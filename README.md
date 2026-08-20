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

You'll need to install the Visual-Studio 17.0 tools for Windows installation. The [Visual Studio Community](https://visualstudio.microsoft.com/downloads/) package provides these tools for free.

## Development

### Testing

To run all the unit-test suites, run:

```shell
make test
```

#### C++

To run the C++ unit-test suite, run:

```shell
scons test
```

#### Python

To run the Python unit-test suite, run:

```shell
python -m unittest discover .
```

### Benchmarking

#### C++

To run the C++ benchmark code, run:

```shell
scons benchmark
```