"""Text order-book ladder.

Prints best-N bid/ask snapshots with depth bars while random order flow
advances the book. Useful to watch spread, depth imbalance, and price
formation tick by tick.

Run:  python ladder.py [--snapshots N] [--events K] [--depth D] [--seed S]
"""

import argparse
import random

from orderly_chaos import LimitOrderBook, Side, UnknownOrderIdError

BAR_WIDTH = 30


def advance(book, rng, fair, next_id, events):
    for _ in range(events):
        roll = rng.random()
        if roll < 0.75:
            side = rng.choice([Side.BUY, Side.SELL])
            offset = rng.randint(1, 12)
            price = fair - offset if side is Side.BUY else fair + offset
            book.limit(side, next_id(), rng.randint(1, 120), price)
        elif roll < 0.85:
            side = rng.choice([Side.BUY, Side.SELL])
            book.market(side, next_id(), rng.randint(1, 60))
        else:
            victim = rng.randint(1, next_id.counter - 1) if next_id.counter > 1 else None
            if victim is not None and victim in book:
                try:
                    book.cancel(victim)
                except UnknownOrderIdError:
                    pass


class Counter:
    def __init__(self):
        self.counter = 1

    def __call__(self):
        value = self.counter
        self.counter += 1
        return value


def render(book, depth):
    asks = book.depth(Side.SELL, levels=depth)
    bids = book.depth(Side.BUY, levels=depth)
    peak = max([lvl.volume for lvl in asks + bids] or [1])
    lines = []
    for level in reversed(asks):  # worst ask on top, best ask above the spread
        bar = "#" * max(1, round(level.volume / peak * BAR_WIDTH))
        lines.append(f"{level.price:>10} | {level.volume:>7} ({level.count:>3}) | {bar}")
    bid, ask = book.best_buy(), book.best_sell()
    spread = f"{ask - bid}" if bid is not None and ask is not None else "n/a"
    lines.append(f"{'--- spread ' + spread + ' ---':>10} | {'mid ' + str(book.mid_price()):>16} |")
    for level in bids:
        bar = "#" * max(1, round(level.volume / peak * BAR_WIDTH))
        lines.append(f"{level.price:>10} | {level.volume:>7} ({level.count:>3}) | {bar}")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Text order-book ladder on Orderly Chaos.")
    parser.add_argument("--snapshots", type=int, default=4)
    parser.add_argument("--events", type=int, default=600)
    parser.add_argument("--depth", type=int, default=5)
    parser.add_argument("--seed", type=int, default=11)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    next_id = Counter()
    book = LimitOrderBook()
    fair = 10_000
    for number in range(1, args.snapshots + 1):
        fair = max(100, fair + rng.randint(-4, 4))
        advance(book, rng, fair, next_id, args.events)
        print(f"=== snapshot {number}  (orders={len(book)} volume={book.volume()}) ===")
        print(render(book, args.depth))
        print()


if __name__ == "__main__":
    main()
