"""Simulate a simple market maker trading against random order flow.

This shows the book used as an exchange simulator for strategy research:
a market maker keeps one bid and one ask around the mid price, re-quotes
after every fill, and tracks its inventory and cash, while random traders
send market and limit orders.

Run:  python examples/python/market_maker.py [steps]
"""

import random
import sys
from itertools import count

from orderly_chaos import LimitOrderBook, Side, Trade

MAKER_SIZE = 50
HALF_SPREAD = 3  # ticks either side of the reference price


def main(steps: int = 5000, seed: int = 7) -> None:
    rng = random.Random(seed)
    ids = count(1)
    maker_orders = {}  # order id -> side, for the market maker's quotes
    position = {"inventory": 0, "cash": 0}

    def on_trade(trade: Trade) -> None:
        side = maker_orders.get(trade.maker_id)
        if side is None:
            return  # a trade between other participants
        if side is Side.BUY:
            position["inventory"] += trade.quantity
            position["cash"] -= trade.quantity * trade.price
        else:
            position["inventory"] -= trade.quantity
            position["cash"] += trade.quantity * trade.price

    book = LimitOrderBook(on_trade=on_trade)
    fair = 10_000

    def requote() -> None:
        # cancel the old quotes and quote around the mid (skewed by inventory)
        for order_id in list(maker_orders):
            if order_id in book:
                book.cancel(order_id)
        maker_orders.clear()
        reference = book.mid_price() or fair
        skew = position["inventory"] // 100  # lean against inventory
        for side, price in ((Side.BUY, reference - HALF_SPREAD - skew), (Side.SELL, reference + HALF_SPREAD - skew)):
            order_id = next(ids)
            maker_orders[order_id] = side
            book.limit(side, order_id, MAKER_SIZE, max(1, price))

    requote()
    for _ in range(steps):
        fair += rng.choice((-1, 0, 1))
        side = rng.choice((Side.BUY, Side.SELL))
        if rng.random() < 0.3:
            book.market(side, next(ids), rng.randint(1, 40))
        else:  # other participants post liquidity away from the touch
            offset = rng.randint(4, 15)
            price = fair - offset if side is Side.BUY else fair + offset
            book.limit(side, next(ids), rng.randint(1, 100), max(1, price))
        if not all(order_id in book for order_id in maker_orders):
            requote()  # one of our quotes was completely filled

    mark = book.mid_price() or fair
    pnl = position["cash"] + position["inventory"] * mark
    print(f"steps            {steps}")
    print(f"resting orders   {len(book)}")
    print(f"inventory        {position['inventory']}")
    print(f"mark price       {mark}")
    print(f"P&L (ticks)      {pnl}")


if __name__ == "__main__":
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 5000)
