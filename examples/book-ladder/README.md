# Order-book ladder

Prints best-N bid/ask snapshots with depth bars while random order flow
advances the book. Watch the spread open and close, depth stack up, and the
mid price drift tick by tick.

## When to use it

You want to see what the matching engine is doing: inspect depth imbalance,
verify that cancels and market orders remove the right quantities, or demo
the book interactively.

## Run it

```bash
pip install -r requirements.txt
python ladder.py --snapshots 4 --events 600 --depth 5 --seed 11
```

Options: `--snapshots N`, `--events K` per snapshot, `--depth D` levels per
side, `--seed S`.

## Sample output

```text
=== snapshot 1  (orders=369 volume=23169) ===
     10008 |    1692 ( 24) | ##############################
     10007 |     986 ( 18) | #################
     ...
--- spread 2 --- |        mid 10003 |
     10002 |     317 (  4) | ######
     10001 |    1310 ( 20) | #######################
     ...
```

Asks print worst-on-top so the best ask sits directly above the spread
line; bids print best-first below it.

## How it works

- Each snapshot advances `events` random limit/market/cancel operations,
  then reads `depth()` per side and scales bars to the largest level.
