# TWAP execution

Buys a large parent quantity in equal slices over time (time-weighted
average price) against live simulated flow, then reports the average fill
price and slippage versus the arrival mid price.

## When to use it

You need to work a large order without sweeping the book: compare patient
sliced execution against an aggressive single fill, or measure how book
depth and spread translate into implementation shortfall.

## Run it

```bash
pip install -r requirements.txt
python twap.py --quantity 5000 --slices 10 --seed 3
```

Options: `--quantity Q` (parent size), `--slices N`, `--seed S`.

## Sample output

```text
parent order      buy 5000 in 10 slices
arrival mid       10011
bought            2663 (shortfall 2337)
average price     10022.68
slippage vs arrival 11.7 bps
other volume      6323
slice  filled  avg price
    1     207  10021.00
    2     148  10021.00
    ...
```

The shortfall is the point: when the book is thin, slices cannot fill and
the remainder is cancelled (slices are immediate-or-cancel). Increase
`--slices` or trade when `other volume` is high to reduce it.

## How it works

- Other participants build the book (`warmup` steps), then each slice
  interval advances more random limit/market/cancel flow.
- Each slice takes liquidity at the best ask and cancels any remainder.
- Per-slice fills come from trade events, so the VWAP is exact.
