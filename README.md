# Mandelbrot set with C++ multithreading

This project compares sequential and multithreaded C++17 implementations of
Mandelbrot-set image generation. The parallel executable supports two row
scheduling strategies:

- **static:** each worker receives one fixed contiguous block of rows;
- **dynamic:** workers acquire rows from a shared atomic counter as they
  finish earlier work.

Because pixels near the Mandelbrot boundary require more iterations, dynamic
scheduling can reduce load imbalance at the cost of additional scheduling
overhead. Threads write to disjoint image rows, and each thread accumulates a
local membership count before a final reduction.

## Requirements

- A C++17 compiler;
- POSIX-compatible C++ threads;
- GNU Make for the provided build targets.

## Build and test

```bash
make
make test
```

The test target renders a small image sequentially and with both parallel
schedulers. Each parallel result is compared pixel by pixel with the
sequential reference.

## Run

Sequential:

```bash
./mandelbrot --height 720 --width 960 --output mandelbrot.ppm
```

Parallel with static row allocation:

```bash
./mandelbrot-parallel \
  --height 720 \
  --width 960 \
  --num-threads 8 \
  --work-allocation static \
  --output mandelbrot-static.ppm
```

Parallel with dynamic row allocation and verification:

```bash
./mandelbrot-parallel \
  --height 720 \
  --width 960 \
  --num-threads 8 \
  --work-allocation dynamic \
  --verify \
  --output mandelbrot-dynamic.ppm
```

The generated image is a binary PPM file. Use `--no-output` for timing runs so
file writing does not produce unnecessary output artifacts. Image writing is
excluded from the reported render time.

## Benchmark

Run the benchmark script after building:

```bash
scripts/benchmark.sh
```

It tests static and dynamic scheduling with `1 2 4 8 16 32` threads and writes
CSV output to `results/mandelbrot.csv`. Override the defaults with
`THREAD_COUNTS`, `ALLOCATIONS`, or the positional arguments documented by
`scripts/benchmark.sh`. On Slurm, a launcher can be supplied explicitly:

```bash
LAUNCHER="srun --nodes=1" scripts/benchmark.sh
```

Benchmark results from the earlier version are not retained because its
so-called dynamic scheduler performed the same cyclic row assignment as the
static scheduler, and its reported "Mandelbrot pixels" counted points outside
the set. Results should be regenerated with the corrected implementation.
