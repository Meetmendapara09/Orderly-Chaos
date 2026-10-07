"""Replay an order-event log through the book (backtesting / market-data replay).

A common use of a limit order book is to rebuild the market from a stream of
order events (from an exchange feed or a strategy's simulated orders) and to
observe the resulting trades and top-of-book. This example replays a small
CSV event log; point it at your own file with:

    python examples/python/replay_events.py events.csv

Each CSV row is ``action,order_id,side,quantity,price`` where ``action`` is
``limit``, ``market``, ``cancel``, or ``reduce`` (unused fields may be blank).
"""

import csv
import io
import sys
from typing import Iterable, List

from orderly_chaos import LimitOrderBook, OrderBookError, Trade

SAMPLE_EVENTS = """\
action,order_id,side,quantity,price
limit,1,sell,100,10050
limit,2,sell,50,10060
limit,3,buy,80,10000
limit,4,buy,120,9990
limit,5,buy,60,10050
reduce,2,,20,
market,6,sell,150,
cancel,4,,,
cancel,4,,,
limit,7,sell,30,9980
"""


def replay(rows: Iterable[dict]) -> List[Trade]:
    """Apply every event to a fresh book, printing the book after each one."""
    trades: List[Trade] = []
    book = LimitOrderBook(on_trade=trades.append)
    for number, row in enumerate(rows, start=1):
        action = row["action"].strip().lower()
        order_id = int(row["order_id"])
        before = len(trades)
        try:
            if action == "limit":
                book.limit(row["side"], order_id, int(row["quantity"]), int(row["price"]))
            elif action == "market":
                book.market(row["side"], order_id, int(row["quantity"]))
            elif action == "cancel":
                book.cancel(order_id)
            elif action == "reduce":
                book.reduce(order_id, int(row["quantity"]))
            else:
                raise ValueError(f"unknown action {action!r}")
        except OrderBookError as error:
            # a real feed handler would log and continue; the book is unchanged
            print(f"{number:>3} {action:<6} #{order_id:<3} REJECTED: {error}")
            continue
        fills = ", ".join(f"{t.quantity}@{t.price} vs #{t.maker_id}" for t in trades[before:])
        print(
            f"{number:>3} {action:<6} #{order_id:<3} bid={book.best_buy()} ask={book.best_sell()} "
            f"orders={len(book)}" + (f"  fills: {fills}" if fills else "")
        )
    return trades


def main(argv: List[str]) -> None:
    if len(argv) > 1:
        with open(argv[1], newline="") as handle:
            trades = replay(csv.DictReader(handle))
    else:
        trades = replay(csv.DictReader(io.StringIO(SAMPLE_EVENTS)))
    volume = sum(t.quantity for t in trades)
    vwap = sum(t.price * t.quantity for t in trades) / volume if volume else 0
    print(f"\n{len(trades)} trades, volume {volume}, VWAP {vwap:.2f}")


if __name__ == "__main__":
    main(sys.argv)
