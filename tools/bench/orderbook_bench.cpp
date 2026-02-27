/**
 * orderbook_bench.cpp
 *
 * Google Benchmark + RDTSC per-call latency sampler for OrderBook::process_event.
 *
 * Custom flags (parsed before benchmark::Initialize):
 *   --bench_events=<path>   Path to events.bin dumped by tools/bench/dump_events.py
 *                           (default: events.bin)
 *   --bench_samples=<path>  Output path for per-call RDTSC samples in samples.bin
 *                           (default: samples.bin)
 *
 * Build:
 *   cmake -DBUILD_BENCHMARKS=ON -B build && cmake --build build --target orderbook_bench
 *
 * Run:
 *   ./build/bench/orderbook_bench \
 *       --bench_events=events.bin \
 *       --bench_samples=samples.bin \
 *       [--benchmark_repetitions=3] [--benchmark_filter=BM_ProcessEvent]
 */

#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "OrderBook.hpp"
#include "Types.hpp"

// ---------------------------------------------------------------------------
// RDTSC helpers
// ---------------------------------------------------------------------------

static inline uint64_t rdtsc_begin() {
    uint32_t lo, hi;
    // CPUID serialises the pipeline before the read.
    __asm__ volatile(
        "cpuid\n\t"
        "rdtsc\n\t"
        : "=a"(lo), "=d"(hi)
        : "a"(0)
        : "rbx", "rcx");
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

static inline uint64_t rdtsc_end() {
    uint32_t lo, hi;
    // RDTSCP waits for prior instructions to retire before reading.
    __asm__ volatile(
        "rdtscp\n\t"
        : "=a"(lo), "=d"(hi)
        :
        : "rcx");
    // Second CPUID prevents later instructions from being measured.
    __asm__ volatile("cpuid\n\t" : : "a"(0) : "rbx", "rcx", "rdx");
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

/**
 * Calibrate TSC ticks per nanosecond using CLOCK_MONOTONIC over a ~200 ms sleep.
 */
static double calibrate_tsc_ns() {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    uint64_t r0 = rdtsc_begin();

    // Busy-spin for ~200 ms to avoid sleep granularity artifacts.
    struct timespec now;
    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
    } while ((now.tv_sec - t0.tv_sec) * 1'000'000'000LL + (now.tv_nsec - t0.tv_nsec) < 200'000'000LL);

    uint64_t r1 = rdtsc_end();
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed_ns =
        static_cast<double>(t1.tv_sec - t0.tv_sec) * 1e9 +
        static_cast<double>(t1.tv_nsec - t0.tv_nsec);
    double ticks = static_cast<double>(r1 - r0);
    return ticks / elapsed_ns;  // ticks per ns
}

// ---------------------------------------------------------------------------
// events.bin format
// ---------------------------------------------------------------------------

static constexpr char EVENTS_MAGIC[8] = {'E','V','T','D','U','M','P','\0'};

struct EventsBin {
    std::vector<int8_t>   actions;
    std::vector<int8_t>   sides;
    std::vector<int64_t>  prices;
    std::vector<int64_t>  sizes;
    std::vector<uint64_t> order_ids;
    std::vector<uint64_t> ts_recvs;
    std::vector<uint8_t>  flags;
    int64_t               n_events = 0;
};

static EventsBin load_events(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open events file: " + path);

    char magic[8];
    f.read(magic, 8);
    if (std::memcmp(magic, EVENTS_MAGIC, 8) != 0)
        throw std::runtime_error("Bad magic in events file: " + path);

    EventsBin eb;
    f.read(reinterpret_cast<char*>(&eb.n_events), 8);
    int64_t n = eb.n_events;

    auto read_vec = [&](auto& vec, size_t elem_size) {
        vec.resize(n);
        f.read(reinterpret_cast<char*>(vec.data()), n * elem_size);
    };

    read_vec(eb.actions,   1);
    read_vec(eb.sides,     1);
    read_vec(eb.prices,    8);
    read_vec(eb.sizes,     8);
    read_vec(eb.order_ids, 8);
    read_vec(eb.ts_recvs,  8);
    read_vec(eb.flags,     1);

    if (!f) throw std::runtime_error("Truncated events file: " + path);
    return eb;
}

// ---------------------------------------------------------------------------
// samples.bin format
// ---------------------------------------------------------------------------

static constexpr char SAMPLES_MAGIC[8] = {'B','S','A','M','P','L','E','S'};

static void write_samples(const std::string& path, const std::vector<double>& samples) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open samples output: " + path);

    f.write(SAMPLES_MAGIC, 8);
    int64_t n = static_cast<int64_t>(samples.size());
    f.write(reinterpret_cast<const char*>(&n), 8);
    f.write(reinterpret_cast<const char*>(samples.data()), n * 8);
}

// ---------------------------------------------------------------------------
// Globals (set before benchmark::Initialize)
// ---------------------------------------------------------------------------

static std::string g_events_path  = "events.bin";
static std::string g_samples_path = "samples.bin";
static EventsBin   g_events;
static double      g_ticks_per_ns = 1.0;

// ---------------------------------------------------------------------------
// RDTSC sample collection (runs BEFORE Google Benchmark iterations)
// ---------------------------------------------------------------------------

static std::vector<double> g_samples;

static void collect_rdtsc_samples() {
    int64_t n = g_events.n_events;
    if (n == 0) return;

    g_samples.reserve(n);

    // Warm-up pass (one full replay, results discarded)
    {
        OrderBook book;
        for (int64_t i = 0; i < n; ++i) {
            trading::Event e = {
                static_cast<trading::Action>(g_events.actions[i]),
                static_cast<trading::Side>(g_events.sides[i]),
                g_events.prices[i],
                g_events.sizes[i],
                g_events.order_ids[i],
            };
            book.process_event(e);
        }
    }

    // Measurement pass
    {
        OrderBook book;
        for (int64_t i = 0; i < n; ++i) {
            trading::Event e = {
                static_cast<trading::Action>(g_events.actions[i]),
                static_cast<trading::Side>(g_events.sides[i]),
                g_events.prices[i],
                g_events.sizes[i],
                g_events.order_ids[i],
            };

            uint64_t t0 = rdtsc_begin();
            book.process_event(e);
            uint64_t t1 = rdtsc_end();

            g_samples.push_back(static_cast<double>(t1 - t0) / g_ticks_per_ns);
        }
    }

    benchmark::ClobberMemory();
}

// ---------------------------------------------------------------------------
// Google Benchmark fixture (throughput + variance across repetitions)
// ---------------------------------------------------------------------------

static void BM_ProcessEvent(benchmark::State& state) {
    int64_t n = g_events.n_events;
    if (n == 0) {
        state.SkipWithError("No events loaded");
        return;
    }

    for (auto _ : state) {
        OrderBook book;
        for (int64_t i = 0; i < n; ++i) {
            trading::Event e = {
                static_cast<trading::Action>(g_events.actions[i]),
                static_cast<trading::Side>(g_events.sides[i]),
                g_events.prices[i],
                g_events.sizes[i],
                g_events.order_ids[i],
            };
            book.process_event(e);
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_ProcessEvent)->Unit(benchmark::kNanosecond)->MinTime(2.0);

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    // Strip our custom flags before handing argv to Google Benchmark
    // (GBM rejects unknown flags).
    std::vector<char*> filtered_argv;
    filtered_argv.push_back(argv[0]);

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg.rfind("--bench_events=", 0) == 0) {
            g_events_path = arg.substr(15);
        } else if (arg.rfind("--bench_samples=", 0) == 0) {
            g_samples_path = arg.substr(16);
        } else if (arg.rfind("--bench_features=", 0) == 0) {
            // ignored by orderbook_bench; consumed here so it doesn't reach GBM
        } else {
            filtered_argv.push_back(argv[i]);
        }
    }

    int filtered_argc = static_cast<int>(filtered_argv.size());

    // Load data
    fprintf(stderr, "[bench] Loading events from %s ...\n", g_events_path.c_str());
    try {
        g_events = load_events(g_events_path);
    } catch (const std::exception& e) {
        fprintf(stderr, "[bench] ERROR: %s\n", e.what());
        return 1;
    }
    fprintf(stderr, "[bench] Loaded %lld events\n", (long long)g_events.n_events);

    // Calibrate TSC
    fprintf(stderr, "[bench] Calibrating TSC ...\n");
    g_ticks_per_ns = calibrate_tsc_ns();
    fprintf(stderr, "[bench] TSC = %.3f ticks/ns\n", g_ticks_per_ns);

    // RDTSC sample collection (warm-up + measure)
    fprintf(stderr, "[bench] Collecting %lld RDTSC samples ...\n", (long long)g_events.n_events);
    collect_rdtsc_samples();

    // Write samples.bin
    try {
        write_samples(g_samples_path, g_samples);
        fprintf(stderr, "[bench] Wrote %zu samples → %s\n", g_samples.size(), g_samples_path.c_str());
    } catch (const std::exception& e) {
        fprintf(stderr, "[bench] WARNING: failed to write samples: %s\n", e.what());
    }

    // Google Benchmark (throughput / wall-clock aggregates)
    benchmark::Initialize(&filtered_argc, filtered_argv.data());
    if (benchmark::ReportUnrecognizedArguments(filtered_argc, filtered_argv.data()))
        return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
