# SMA crossover backtest

A trend-following strategy (fast/slow simple moving average of the mid
price) trading through Orderly Chaos instead of assuming fills at the last
price. Every strategy order walks the real queue, so spread costs, partial
fills, and queue priority show up in the P&L.

## When to use it

You are testing an execution idea and want realistic fills: crossing limit
orders that rest when they cannot trade, priority-preserving position
management, and per-trade accounting from trade events.

## Run it

```bash
pip install -r requirements.txt
python backtest.py --steps 2000 --seed 7
```

Options: `--steps N` (market events), `--seed S` (deterministic order flow),
`--csv FILE` (write the summary row for comparing runs).

## Sample output

```text
steps            2000 (seed 7)
strategy trades  131
final inventory  -50
mark price       9922
strategy P&L     6108 ticks
buy-and-hold P&L -4400 ticks
resting orders   519
```

## How it works

- Random participants post passive limit orders around a random-walk fair
  price (`background` flow in `backtest.py`).
- On a fresh SMA(5)/SMA(20) crossover the strategy cancels its resting
  orders and trades toward +50 (long) or -50 (short) units with crossing
  limit orders.
- Fills are tracked from `on_trade` events matched against the strategy's
  own order IDs, so trades with other participants never pollute the P&L.
- Equity is marked to the final mid price and compared against buy-and-hold.

Simplification for clarity: the venue allows self-trades (a strategy sell
can hit its own resting bid). Production venues usually prevent this.
