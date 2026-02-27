# Bookbench: Writing Your Own Order Book
> “The order book is the heart of any trading system.”
> 
> Larry Harris
## Introduction

An extremely common interview question across HFT firms is the implementation of an order book. At its core, an order book maintains a record of all outstanding buy and sell orders for a given instrument: price levels, bids and asks at each level, the best bid/offer spread, and order quantities. The order book is undoubtedly one of the most valuable assets to a firm.

However, perhaps unlike these interviews, what we've done here is given you the full solution. Inside you will find a fully implemented and functional order book using the C++ standard library. It is somewhat "optimal" in principle and more or less the kind of answer you'd may be expected to produce.

Practically, the implementation is not optimal. There are both immediately obvious and niche, obscure methods that can offer significant computational improvements over what is given, and your task is to find and exploit them in whatever manner you wish.

Your goal is simple, make it faster.

---

## Task Details

Your implementation will live in `include/OrderBook.hpp` and `src/OrderBook.cpp`. These these are the only files you will need to modify. 

You will not need to modify:
- `Engine.hpp/cpp`, which handles event processing logic for the book.
- `tools/`, which contains the statistical profiling code.

Accuracy is just as important as speed. Use the test suite to check the state of your book. You can add your own tests to the `test/` directory, and they'll be automatically picked up when you rebuild the project. A default starter file has already been included for you.

Once your implementation is correct, you can use `python -m tools.run` or `python -m tools.run --no-plot` to benchmark it.

---

## Getting Started

### Requirements

Install Python dependencies:

```bash
pip install -r requirements.txt
```

Build the project with CMake:

```bash
cmake -S . -B build
cmake --build build
```

### Statistical Profiling

To benchmark your implementation, run:

```bash
python -m tools.run
```

This profiles your order book **against a real trading day of events** and reports nanosecond per-event latencies. To suppress the plot output (which might make repeated runs easier), pass the `--no-plot` flag:

```bash
python -m tools.run --no-plot
```

### Testing

To run the order book test suite:

```bash
ctest --test-dir build
```

---

## Final Notes

This task is not intended to be a test! The problem you're looking at is quite a real one across HFTs, and knowledge of how to build an effective one is what sets firms apart. We want think through every approach brought to the table with you — there is no single "correct" answer.

Please be aware that we want to be available to you throughout. If something is unclear, if you want to think through an approach on a whiteboard, or if you just want to chat and learn more about the company, don't hesistate to ask. This is meant to be a collaborative effort.

The only thing we ask is that you have fun with it.
