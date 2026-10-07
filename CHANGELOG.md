# Changelog

All notable changes to this project are documented in this file. The format
follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the
project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Symbolic error names shared across languages: C++ `code_name()` and Python
  `OrderBookError.code_name` return the `OC_ERR_*` status name, and error
  messages include it (for example `unknown order id
  [OC_ERR_UNKNOWN_ORDER_ID]`), so a failure seen in one language can be
  looked up in any API reference.
- Documentation: an error "Fix" column in the overview, a proper favicon set
  (SVG, PNG, Apple touch icon, web manifest), and a simplified layers diagram.

### Changed

- Landing page: removed the decorative version pill and background blobs.
- Error messages now state the failed operation, the reason, and the symbolic
  status code.

## [0.2.0] - 2026-10-07

This release makes the engine production ready. It contains breaking changes
to the C API, the Python API, and the include paths.

### Added

- Trade events: `set_trade_handler` (C++), `oc_book_set_trade_callback` (C),
  and `on_trade` (Python) report taker, maker, price, quantity, and the
  maker's remaining quantity for every execution.
- Limit and market orders return the quantity filled immediately.
- `depth(side, levels)` returns aggregated price levels, best first.
- Python: `get()`, `reduce()`, `mid_price()`, `spread()`, `best_volume()`,
  `Side`, `Order`, `Trade`, and `PriceLevel` types, typed exceptions, context
  manager support, `len()` and `in`, and type hints (`py.typed`).
- C API: a real C header (`orderly_chaos.h`) with `oc_`-prefixed functions,
  status codes, `oc_book_get`, `oc_book_reduce`, `oc_book_depth`, and
  `oc_version`.
- C++: `OrderBookError` with machine-readable `ErrorCode`, `empty()`,
  `volume_best()`, `check_invariants()`, and version macros.
- `ORDERLY_CHAOS_LIBRARY` environment variable to choose the native library.
- Platform-specific shared library targets (`.so`, `.dylib`, `.dll`).
- Test suites for validation, trade events, the C API, library loading,
  threading, examples, and randomized differential tests against reference
  models in C++ and Python.
- Runnable examples for Python, C++, and C; a benchmark harness.
- Documentation: overview, getting started, architecture, complete API
  references, examples, and benchmarks, with Mermaid diagrams.
- `pyproject.toml`, `LICENSE`, `THIRD_PARTY_NOTICES.md`, `CONTRIBUTING.md`,
  `SECURITY.md`, and a committed cross-platform `.bazelrc`.

### Changed

- Price levels are stored in a balanced tree (`std::map`), guaranteeing
  O(log L) level changes. With 100,000 rising prices, inserts are 447 times
  faster than in 0.1.
- Headers moved to `include/orderly_chaos/`; include
  `<orderly_chaos/orderly_chaos.hpp>`. The namespace is now `orderly_chaos`
  (the `LOB` alias remains for compatibility).
- The Python package moved to `python/orderly_chaos/`; empty-side prices
  return `None` instead of `0`.
- Volumes and counts are 64-bit everywhere.
- The Python package loads the library with `ctypes.PyDLL`, making shared use
  from multiple threads memory-safe.
- `setup.py` no longer compiles with `-march=native`, so built wheels run on
  any CPU of the target architecture.

### Fixed

- Submitting a limit order with an ID that is already resting corrupted the
  book; it is now rejected.
- A limit price of 0 behaved like a market order and could sweep the book; it
  is now rejected.
- A quantity of 0 was accepted; it is now rejected.
- Cancelling or looking up an unknown ID terminated the process from Python
  (an uncaught C++ exception crossed the C boundary).
- Python read 64-bit volumes as 32-bit integers, silently truncating them.
- Python silently truncated out-of-range integers passed to the C library.
- Destroying a book leaked all of its price levels.
- Copying a book caused a double free; books are now movable but not copyable.
- The mid price overflowed for prices near the 64-bit limit.
- On Windows, a `pip install` build could not be found by the loader (`.pyd`).
- The Python package on Linux and macOS required a manual rename of the
  library built by Bazel.

### Removed

- Unused vendored libraries (`hopscotch_map`, `sparse_map`, `cpptqdm`, and
  the old tree and list libraries now replaced by standard containers).
- The committed prebuilt Windows DLL (it was built from the old ABI).
- Obsolete Travis CI and SCons configuration and third-party notes.

## [0.1.0] - 2026-08-20

- Initial release of Orderly Chaos: a price-time priority limit order book
  with C++, C, and Python interfaces, a Bazel build, and a documentation site.

[Unreleased]: https://github.com/Meetmendapara09/Orderly-Chaos/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/Meetmendapara09/Orderly-Chaos/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Meetmendapara09/Orderly-Chaos/releases/tag/v0.1.0
