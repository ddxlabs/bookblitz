"""
Entry point for the orderbook benchmark tooling.

Usage (from engine/):

  # 1. Dump events from a DBN file
  python -m tools.bench dump <data.dbn> [--out events.bin] [--n-events N]

  # 2. Run the C++ benchmark (must be built first)
  #    This writes samples.bin alongside the binary.
  ./build/bench/orderbook_bench --bench_events=events.bin --bench_samples=samples.bin

  # 3. Plot the samples
  python -m tools.bench plot [samples.bin] [--title "My benchmark"]

  # Or do dump + plot in one shot (benchmark binary must exist):
  python -m tools.bench run <data.dbn> --bench ./build/bench/orderbook_bench
"""

import argparse
import logging
import os
import subprocess
import sys

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")


def profile(args):
    tmp = os.path.abspath(args.tmp_dir)
    os.makedirs(tmp, exist_ok=True)
    events_bin  = os.path.join(tmp, "events.bin")
    samples_bin = os.path.join(tmp, "samples.bin")

    # 2. run C++ benchmark
    bench_args = [
        args.bench,
        f"--bench_events={events_bin}",
        f"--bench_samples={samples_bin}",
    ]

    print(f"Running: {' '.join(bench_args)}")
    ret = subprocess.run(bench_args)
    if ret.returncode != 0:
        sys.exit(ret.returncode)

    # 3. plot
    from .plot_samples import load_samples, plot_samples, print_summary
    samples = load_samples(samples_bin)
    print_summary(samples)
    if not args.no_plot:
        plot_samples(samples_bin, title=f"process_event()")


def main():
    parser = argparse.ArgumentParser(
        description="OrderBook benchmark tools",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # --- run ---
    p_run = sub.add_parser("run", help="Dump + benchmark + plot in one step")
    p_run.add_argument("--bench", default="build/bench/orderbook_bench",
                       help="Path to benchmark binary")
    p_run.add_argument("--tmp-dir", default="tools/bench/", metavar="DIR",
                       help="Directory for intermediate bin files")
    p_run.add_argument("--n-events", type=int, default=None, metavar="N",
                       help="Limit to first N events")
    p_run.add_argument("--no-plot", action="store_true",
                       help="Skip matplotlib window after benchmark")

    args = parser.parse_args()

    profile(args)


if __name__ == "__main__":
    main()
