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

## Demo projects

Larger, self-contained projects built only on the published
`orderly-chaos` package (`pip install -r requirements.txt` in each
directory). Each has its own README and is executed by the test suite
with a small workload.

| Demo | Use case | Run |
|------|----------|-----|
| [sma-backtest/](sma-backtest/) | Trend-following strategy with realistic queue fills and P&L vs buy-and-hold | `python examples/sma-backtest/backtest.py --steps 2000` |
| [twap-executor/](twap-executor/) | Work a large parent order in time slices; VWAP and slippage vs arrival | `python examples/twap-executor/twap.py --quantity 5000 --slices 10` |
| [book-ladder/](book-ladder/) | Text depth ladder: watch spread, imbalance, and price formation | `python examples/book-ladder/ladder.py --snapshots 4` |

The Python examples need the package installed (`pip install -e .`) or the
library built with `make python-lib` and `PYTHONPATH=python`.

Annotated walkthroughs are on the [examples page](../docs/examples/index.html).
