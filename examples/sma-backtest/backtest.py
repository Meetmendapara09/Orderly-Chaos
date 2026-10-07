"""SMA crossover backtest on a simulated limit order book.

A classic trend-following strategy (fast/slow simple moving average of the
mid price) trades a single instrument through Orderly Chaos instead of
assuming fills at the last price. Every strategy order walks the real queue,
so spread costs, partial fills, and queue priority are reflected in the P&L.

Run:  python backtest.py [--steps N] [--seed S] [--csv FILE]
"""

import argparse
import csv
import random
from collections import deque

from orderly_chaos import LimitOrderBook, Side, UnknownOrderIdError

SIZE = 50  # units per position step; position is always -SIZE, 0, or +SIZE
FAST = 5
SLOW = 20


class Strategy:
    """Goes long/short SIZE units on SMA crosses, flat otherwise."""

    def __init__(self, book):
        self.book = book
        self.own_ids = set()
        self.inventory = 0
        self.cash = 0
        self.trades = 0
        book.on_trade = self.on_trade

    def on_trade(self, trade):
        ours_taker = trade.taker_id in self.own_ids
        ours_maker = trade.maker_id in self.own_ids
        if not (ours_taker or ours_maker):
            return  # other participants trading among themselves
        side = trade.taker_side if ours_taker else trade.taker_side.opposite
        signed = trade.quantity if side is Side.BUY else -trade.quantity
        self.inventory += signed
        self.cash -= signed * trade.price
        self.trades += 1

    def cancel_resting(self):
        for order_id in list(self.own_ids):
            if order_id in self.book:
                try:
                    self.book.cancel(order_id)
                except UnknownOrderIdError:
                    pass  # filled between the check and the cancel
            else:
                self.own_ids.discard(order_id)

    def trade_to(self, target, reference, next_id):
        """Move inventory toward target using crossing limit orders."""
        while self.inventory != target:
            delta = target - self.inventory
            side = Side.BUY if delta > 0 else Side.SELL
            quantity = min(abs(delta), SIZE)
            anchor = self.book.best_sell() if side is Side.BUY else self.book.best_buy()
            price = max(1, anchor if anchor is not None else reference)
            order_id = next_id()
            self.own_ids.add(order_id)
            if self.book.limit(side, order_id, quantity, price) == 0:
                break  # no opposite liquidity; the remainder rests for later
            if order_id not in self.book:
                self.own_ids.discard(order_id)  # fully filled immediately


def run(steps=2000, seed=7):
    rng = random.Random(seed)
    book = LimitOrderBook()
    strategy = Strategy(book)
    ids = iter(range(1, 10**9))
    next_id = lambda: next(ids)  # noqa: E731
    noise_ids = deque()

    fair = 10_000
    mids = []
    first_mid = None
    for _ in range(steps):
        fair = max(100, fair + rng.randint(-3, 3))
        # other participants post passive liquidity around the fair price
        for _ in range(rng.randint(1, 3)):
            side = rng.choice([Side.BUY, Side.SELL])
            offset = rng.randint(1, 12)
            price = fair - offset if side is Side.BUY else fair + offset
            order_id = next_id()
            book.limit(side, order_id, rng.randint(1, 60), price)
            noise_ids.append(order_id)
        while len(noise_ids) > 1500:  # keep the book bounded
            victim = noise_ids.popleft()
            if victim in book:
                try:
                    book.cancel(victim)
                except UnknownOrderIdError:
                    pass
        mid = book.mid_price() or fair
        mids.append(mid)
        if first_mid is None:
            first_mid = mid
        # rebalance on a fresh crossover signal only
        if len(mids) >= SLOW:
            fast = sum(mids[-FAST:]) / FAST
            slow = sum(mids[-SLOW:]) / SLOW
            prev_fast = sum(mids[-FAST - 1:-1]) / FAST
            prev_slow = sum(mids[-SLOW - 1:-1]) / SLOW
            crossed_up = prev_fast <= prev_slow and fast > slow
            crossed_down = prev_fast >= prev_slow and fast < slow
            if crossed_up or crossed_down:
                strategy.cancel_resting()
                strategy.trade_to(SIZE if crossed_up else -SIZE, mid, next_id)

    mark = book.mid_price() or fair
    equity = strategy.cash + strategy.inventory * mark
    buy_hold = SIZE * (mark - first_mid)  # long SIZE from the first mid
    return {
        "steps": steps,
        "seed": seed,
        "strategy_trades": strategy.trades,
        "final_inventory": strategy.inventory,
        "cash": strategy.cash,
        "mark": mark,
        "equity_ticks": equity,
        "buy_hold_ticks": buy_hold,
        "resting_orders": len(book),
    }


def main():
    parser = argparse.ArgumentParser(description="SMA crossover backtest on Orderly Chaos.")
    parser.add_argument("--steps", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=7)
    parser.add_argument("--csv", default=None, help="write the summary row to this CSV file")
    args = parser.parse_args()
    result = run(steps=args.steps, seed=args.seed)
    print(f"steps            {result['steps']} (seed {result['seed']})")
    print(f"strategy trades  {result['strategy_trades']}")
    print(f"final inventory  {result['final_inventory']}")
    print(f"mark price       {result['mark']}")
    print(f"strategy P&L     {result['equity_ticks']} ticks")
    print(f"buy-and-hold P&L {result['buy_hold_ticks']} ticks")
    print(f"resting orders   {result['resting_orders']}")
    if args.csv:
        with open(args.csv, "w", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=sorted(result))
            writer.writeheader()
            writer.writerow(result)
        print(f"wrote {args.csv}")


if __name__ == "__main__":
    main()
