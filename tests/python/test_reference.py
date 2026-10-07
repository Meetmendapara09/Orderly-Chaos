"""Randomized differential test of the Python API against a pure-Python model."""

import random
import unittest
from collections import deque

from orderly_chaos import LimitOrderBook, OrderBookError, PriceLevel, Side, Trade


class ReferenceBook:
    """A deliberately simple price-time priority order book."""

    def __init__(self):
        self.levels = {Side.BUY: {}, Side.SELL: {}}  # side -> price -> deque[[id, qty]]
        self.where = {}  # order id -> (side, price)
        self.trades = []

    def limit(self, side, order_id, quantity, price):
        if quantity == 0:
            raise ValueError("InvalidQuantityError")
        if price == 0:
            raise ValueError("InvalidPriceError")
        if order_id in self.where:
            raise ValueError("DuplicateOrderIdError")
        remaining = self._match(side, order_id, quantity, price)
        if remaining:
            self.levels[side].setdefault(price, deque()).append([order_id, remaining])
            self.where[order_id] = (side, price)
        return quantity - remaining

    def market(self, side, order_id, quantity):
        if quantity == 0:
            raise ValueError("InvalidQuantityError")
        return quantity - self._match(side, order_id, quantity, None)

    def cancel(self, order_id):
        if order_id not in self.where:
            raise ValueError("UnknownOrderIdError")
        side, price = self.where.pop(order_id)
        queue = self.levels[side][price]
        queue.remove(next(entry for entry in queue if entry[0] == order_id))
        if not queue:
            del self.levels[side][price]

    def reduce(self, order_id, quantity):
        if order_id not in self.where:
            raise ValueError("UnknownOrderIdError")
        side, price = self.where[order_id]
        entry = next(e for e in self.levels[side][price] if e[0] == order_id)
        if quantity == 0 or quantity > entry[1]:
            raise ValueError("InvalidQuantityError")
        entry[1] -= quantity
        if entry[1] == 0:
            self.cancel(order_id)

    def _match(self, side, order_id, quantity, limit):
        opposite = self.levels[side.opposite]
        while quantity and opposite:
            price = min(opposite) if side is Side.BUY else max(opposite)
            if limit is not None and (price > limit if side is Side.BUY else price < limit):
                break
            queue = opposite[price]
            maker = queue[0]
            traded = min(quantity, maker[1])
            quantity -= traded
            maker[1] -= traded
            self.trades.append(Trade(order_id, maker[0], side, price, traded, maker[1]))
            if maker[1] == 0:
                queue.popleft()
                del self.where[maker[0]]
                if not queue:
                    del opposite[price]
        return quantity

    def best(self, side):
        prices = self.levels[side]
        if not prices:
            return None
        return max(prices) if side is Side.BUY else min(prices)

    def depth(self, side):
        prices = sorted(self.levels[side], reverse=side is Side.BUY)
        return [
            PriceLevel(p, sum(e[1] for e in self.levels[side][p]), len(self.levels[side][p])) for p in prices
        ]


def outcome(action):
    """Run an action, returning (result, error class name or None)."""
    try:
        return action(), None
    except OrderBookError as error:
        return None, type(error).__name__
    except ValueError as error:
        return None, str(error)


class DifferentialTest(unittest.TestCase):
    def run_scenario(self, seed, steps, mid, spread):
        rng = random.Random(seed)
        reference = ReferenceBook()
        trades = []
        book = LimitOrderBook(on_trade=trades.append)
        next_id = 1
        for step in range(steps):
            reference.trades.clear()
            trades.clear()
            side = rng.choice([Side.BUY, Side.SELL])
            resting = list(reference.where)
            existing = rng.choice(resting) if resting else None
            quantity = 0 if rng.random() < 0.02 else rng.randint(1, 100)
            price = 0 if rng.random() < 0.02 else rng.randint(mid - spread, mid + spread)
            action = rng.random()
            if action < 0.55:
                order_id = existing if existing is not None and rng.random() < 0.03 else next_id
                next_id += 1
                expected = outcome(lambda: reference.limit(side, order_id, quantity, price))
                actual = outcome(lambda: book.limit(side, order_id, quantity, price))
            elif action < 0.70:
                order_id, next_id = next_id, next_id + 1
                expected = outcome(lambda: reference.market(side, order_id, quantity))
                actual = outcome(lambda: book.market(side, order_id, quantity))
            elif action < 0.88:
                order_id = existing if existing is not None and rng.random() > 0.1 else next_id + 1000
                expected = outcome(lambda: reference.cancel(order_id))
                actual = outcome(lambda: book.cancel(order_id))
            else:
                order_id = existing if existing is not None else next_id + 1000
                expected = outcome(lambda: reference.reduce(order_id, quantity % 60))
                actual = outcome(lambda: book.reduce(order_id, quantity % 60))

            context = f"seed={seed} step={step}"
            self.assertEqual(expected, actual, context)
            self.assertEqual(reference.trades, trades, context)
            for s in Side:
                self.assertEqual(reference.best(s), book.best(s), context)
                self.assertEqual(reference.depth(s), book.depth(s, levels=1000), context)
            self.assertEqual(len(reference.where), len(book), context)

    def test_tight_spread(self):
        for seed in range(5):
            self.run_scenario(seed, 600, mid=1000, spread=5)

    def test_wide_spread(self):
        for seed in range(100, 103):
            self.run_scenario(seed, 800, mid=100_000, spread=300)


if __name__ == "__main__":
    unittest.main()
