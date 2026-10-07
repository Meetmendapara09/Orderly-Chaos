# Examples

Runnable programs that show Orderly Chaos in realistic situations. Each one is
executed by the test suite, so they always match the current API.

| Example | Use case | Run |
|---------|----------|-----|
| [python/quickstart.py](python/quickstart.py) | Every core operation in one script | `python examples/python/quickstart.py` |
| [python/replay_events.py](python/replay_events.py) | Rebuild a book from a CSV event log (market data replay, backtesting) | `python examples/python/replay_events.py [events.csv]` |
| [python/market_maker.py](python/market_maker.py) | A quoting strategy against random order flow (exchange simulation) | `python examples/python/market_maker.py [steps]` |
| [cpp/quickstart.cpp](cpp/quickstart.cpp) | The C++ API end to end | `bazel run //examples/cpp:quickstart` |
| [cpp/market_simulation.cpp](cpp/market_simulation.cpp) | High-volume random order flow with throughput statistics | `bazel run -c opt //examples/cpp:market_simulation -- 1000000` |
| [c/quickstart.c](c/quickstart.c) | The C API with status codes and a trade callback | `bazel run //examples/c:quickstart` |

The Python examples need the package installed (`pip install -e .`) or the
library built with `make python-lib` and `PYTHONPATH=python`.

Annotated walkthroughs are on the [examples page](../docs/examples/index.html).
