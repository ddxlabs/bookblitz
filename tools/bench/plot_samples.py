"""
Read a samples.bin file produced by orderbook_bench and plot a latency histogram.

Binary format (samples.bin):
  [8  bytes]           magic: b"BSAMPLES"
  [8  bytes int64 LE]  n_samples
  [n × float64 LE]     nanoseconds per process_event call
"""

import struct
import sys
import numpy as np

MAGIC = b"BSAMPLES"


def load_samples(path: str) -> np.ndarray:
    with open(path, "rb") as f:
        magic = f.read(8)
        if magic != MAGIC:
            raise ValueError(f"Bad magic in {path}: {magic!r}")
        (n,) = struct.unpack("<q", f.read(8))
        samples = np.frombuffer(f.read(n * 8), dtype=np.float64).copy()
    return samples


def plot_samples(samples_path: str, title: str = "latency profile for process_event") -> None:
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker

    samples = load_samples(samples_path)
    n = len(samples)

    p50  = np.percentile(samples, 50)
    p90  = np.percentile(samples, 90)
    p95  = np.percentile(samples, 95)
    p99  = np.percentile(samples, 99)

    clipped = samples[samples <= p95]

    import matplotlib.lines as mlines

    plt.rcParams.update({
        "font.family": "sans-serif",
        "font.size": 13,
        "axes.titlesize": 15,
        "axes.labelsize": 13,
        "axes.titleweight": "bold",
        "legend.fontsize": 12,
        "xtick.labelsize": 11,
        "ytick.labelsize": 11,
        "axes.facecolor": "#F7F9FC",
        "figure.facecolor": "#FFFFFF",
        "axes.grid": True,
        "grid.color": "#DADDE1",
        "grid.linewidth": 0.7,
        "grid.linestyle": "-",
    })

    fig, ax = plt.subplots(figsize=(12, 5))

    counts, edges = np.histogram(clipped, bins=150)
    centers = (edges[:-1] + edges[1:]) / 2
    mask = counts > 0
    ax.vlines(centers[mask], 0, counts[mask], color="#4C8EDA", linewidth=2.5, zorder=2)
    ax.plot(centers[mask], counts[mask], "o", color="#4C8EDA", markersize=7, zorder=3)
    ax.set_yscale("log")

    # Y-axis: major ticks with comma-formatted integers, minor ticks unlabelled
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(
        lambda x, _: f"{int(x):,}" if x >= 1 else f"{x:.1f}"
    ))
    ax.yaxis.set_major_locator(ticker.LogLocator(base=10, subs=[1, 2, 5], numticks=20))
    ax.yaxis.set_minor_locator(ticker.LogLocator(base=10, subs="all", numticks=100))
    ax.yaxis.set_minor_formatter(ticker.NullFormatter())

    # X-axis: auto minor ticks between major ticks
    ax.xaxis.set_minor_locator(ticker.AutoMinorLocator(5))
    ax.tick_params(axis="x", which="minor", length=3, color="#AAAAAA")
    ax.tick_params(axis="both", which="major", length=5)

    # Grid on major y ticks only
    ax.yaxis.grid(True, which="major", zorder=0)
    ax.xaxis.grid(False)

    # p50 vertical line
    ax.axvline(p50, linestyle="--", color="#3DAA5C", linewidth=1.8, zorder=3)

    # Legend entries for all three percentiles (only p50 drawn)
    for pct, val, col in [
        ("p50", p50, "#3DAA5C"),
        ("p90", p90, "#E8883A"),
        ("p99", p99, "#E05A4E"),
    ]:
        ax.add_artist(mlines.Line2D([], [], color=col, linestyle="--",
                                    linewidth=1.8, label=f"{pct}  {val:.0f} ns"))

    ax.set_xlabel("Latency (ns)", labelpad=8)
    ax.set_ylabel("Count  (log scale)", labelpad=8)
    ax.set_title(f"{title}  —  n = {n:,}  ·  (clipped at p95 = {p95:.0f} ns)", pad=10)

    ax.legend(framealpha=0.9, edgecolor="#CCCCCC", loc="upper right")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_color("#CCCCCC")
    ax.spines["bottom"].set_color("#CCCCCC")

    plt.tight_layout()
    plt.show()


def print_summary(samples: np.ndarray) -> None:
    print(f"  n        = {len(samples):,}")
    print(f"  mean     = {np.mean(samples):.1f} ns")
    print(f"  median   = {np.median(samples):.1f} ns")
    print(f"  p90      = {np.percentile(samples, 90):.1f} ns")
    print(f"  p99      = {np.percentile(samples, 99):.1f} ns")
    print(f"  p99.9    = {np.percentile(samples, 99.9):.1f} ns")
    print(f"  max      = {np.max(samples):.1f} ns")
