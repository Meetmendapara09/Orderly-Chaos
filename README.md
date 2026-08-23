# Orderly Chaos

**Orderly Chaos: A Non-Consensual Matching Engine for Unruly Capital**

This is an implementation of the limit order book with a price-time priority matching policy for reception of market data streams. There are APIs for C++, C, and Python.

## Usage

### C++

Simply add [include/*.hpp](include) to your C++ project either by copying directly or using git submodules.

### C

A C-level API is defined in [include/lib_lob.hpp](include/lib_lob.hpp).

### Python

The preferred Python installation of `orderly-chaos` is from `pip`:

```shell
pip install orderly-chaos
```

## Usage Caveats

### Windows

Building on Windows uses the MinGW-w64 toolchain (e.g. the one bundled with [Strawberry Perl](https://strawberryperl.com/)) via a custom Bazel toolchain registered in [toolchain/BUILD.bazel](toolchain/BUILD.bazel). Adjust `STRAWBERRY_BIN` there if your g++ lives elsewhere.

## Development

### Testing

To run all the unit-test suites, run:

```shell
make test
```

#### C++

To run the C++ unit-test suite (Bazel + Google Test), run:

```shell
bazel test //test:all
```

#### Python

To build the shared library and run the Python unit-test suite, run:

```shell
make test
```

or manually:

```shell
bazel build //orderly_chaos:copy_dll
cp bazel-bin/orderly_chaos/lib_orderly_chaos.dll orderly_chaos/
python -m unittest discover .
```

### Benchmarking

#### C++

To run the C++ benchmark suite, run:

```shell
bazel test //benchmark:benchmark_lob_test --test_output=streamed
```
