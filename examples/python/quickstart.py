"""Orderly Chaos: Python quick start.

Run:  python examples/python/quickstart.py
"""

from orderly_chaos import LimitOrderBook, Side, UnknownOrderIdError


def main() -> None:
    # 1. Print every execution as it happens.
    book = LimitOrderBook(on_trade=lambda t: print(f"TRADE taker={t.taker_id} maker={t.maker_id} qty={t.quantity} @ {t.price}"))

    # 2. Build a book. Prices are integer ticks (here: cents).
    book.limit(Side.SELL, order_id=1, quantity=100, price=10_050)  # ask 100 @ 100.50
    book.limit(Side.SELL, order_id=2, quantity=200, price=10_075)  # ask 200 @ 100.75
    book.limit(Side.BUY, order_id=3, quantity=150, price=10_000)  # bid 150 @ 100.00
    print(f"bid {book.best_buy()} / ask {book.best_sell()}  mid {book.mid_price()}  spread {book.spread()}")

    # 3. An aggressive limit order trades up to its price; the rest rests.
    filled = book.limit(Side.BUY, order_id=4, quantity=250, price=10_050)
    print(f"order 4 filled {filled}, resting {book.get(4).quantity}")

    # 4. A market order sweeps the book; any remainder is discarded.
    print(f"market buy filled {book.market(Side.BUY, order_id=5, quantity=500)} of 500")

    # 5. Depth snapshot (best first).
    for level in book.depth(Side.BUY, levels=5):
        print(f"  bid level {level.price} x {level.volume} ({level.count} orders)")

    # 6. Errors raise typed exceptions and never modify the book.
    try:
        book.cancel(999)
    except UnknownOrderIdError as error:
        print(f"rejected: {error}")


if __name__ == "__main__":
    main()
