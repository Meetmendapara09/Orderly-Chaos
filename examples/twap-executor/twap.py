"""TWAP execution against simulated market flow.

Buys a large parent quantity in equal slices over time (time-weighted
average price execution) instead of sweeping the book at once, then
reports the average fill price and slippage versus the arrival mid price.

Run:  python twap.py [--quantity Q] [--slices N] [--seed S]
"""

import argparse
import random

from orderly_chaos import LimitOrderBook, Side, UnknownOrderIdError


def background_flow(book, rng, fair, next_id, steps):
    """Advance other participants' random limit/market flow; return volume traded."""
    volume = 0
    seen = []
    previous = book.on_trade
    book.on_trade = lambda t: seen.append(t.quantity) or (previous(t) if previous else None)
    for _ in range(steps):
        roll = rng.random()
        if roll < 0.7:
            side = rng.choice([Side.BUY, Side.SELL])
            offset = rng.randint(1, 10)
            price = fair - offset if side is Side.BUY else fair + offset
            book.limit(side, next_id(), rng.randint(1, 80), price)
        elif roll < 0.8:
            side = rng.choice([Side.BUY, Side.SELL])
            book.market(side, next_id(), rng.randint(1, 40))
        else:
            victim = rng.randint(1, next_id.counter - 1) if next_id.counter > 1 else None
            if victim is not None and victim in book:
                try:
                    book.cancel(victim)
                except UnknownOrderIdError:
                    pass
    book.on_trade = previous
    return sum(seen)


class Counter:
    def __init__(self):
        self.counter = 1

    def __call__(self):
        value = self.counter
        self.counter += 1
        return value


def run(quantity=5000, slices=10, seed=3, warmup=2000, flow_per_slice=200):
    rng = random.Random(seed)
    next_id = Counter()
    book = LimitOrderBook()

    fair = 10_000
    for _ in range(warmup):  # let other participants build the book first
        fair = max(100, fair + rng.randint(-2, 2))
        background_flow(book, rng, fair, next_id, 1)

    arrival = book.mid_price() or fair
    slice_quantity = quantity // slices
    bought, spent, market_volume = 0, 0, 0
    shortfall = 0
    per_slice = []
    for number in range(1, slices + 1):
        fair = max(100, fair + rng.randint(-2, 2))
        market_volume += background_flow(book, rng, fair, next_id, flow_per_slice)
        ask = book.best_sell()
        if ask is None:  # nothing offered; the slice waits out this interval
            shortfall += slice_quantity
            per_slice.append((number, 0, None))
            continue
        fills = []
        book.on_trade = lambda t: fills.append(t)  # noqa: E731
        order_id = next_id()
        filled = book.limit(Side.BUY, order_id, slice_quantity, ask)
        book.on_trade = None
        if order_id in book:
            book.cancel(order_id)  # slices are immediate-or-cancel; never leave a remainder
        bought += filled
        spent += sum(t.price * t.quantity for t in fills)
        shortfall += slice_quantity - filled
        per_slice.append((number, filled, (sum(t.price * t.quantity for t in fills) / filled) if filled else None))

    vwap = spent / bought if bought else None
    slippage_bps = (vwap - arrival) / arrival * 10_000 if vwap else None
    return {
        "quantity": quantity,
        "slices": slices,
        "arrival": arrival,
        "bought": bought,
        "shortfall": shortfall,
        "vwap": vwap,
        "slippage_bps": slippage_bps,
        "market_volume": market_volume,
        "per_slice": per_slice,
    }


def main():
    parser = argparse.ArgumentParser(description="TWAP execution demo on Orderly Chaos.")
    parser.add_argument("--quantity", type=int, default=5000)
    parser.add_argument("--slices", type=int, default=10)
    parser.add_argument("--seed", type=int, default=3)
    args = parser.parse_args()
    result = run(quantity=args.quantity, slices=args.slices, seed=args.seed)
    print(f"parent order      buy {result['quantity']} in {result['slices']} slices")
    print(f"arrival mid       {result['arrival']}")
    print(f"bought            {result['bought']} (shortfall {result['shortfall']})")
    print(f"average price     {result['vwap']:.2f}" if result["vwap"] else "average price     n/a")
    print(f"slippage vs arrival {result['slippage_bps']:.1f} bps" if result["slippage_bps"] is not None else "slippage          n/a")
    print(f"other volume      {result['market_volume']}")
    print("slice  filled  avg price")
    for number, filled, average in result["per_slice"]:
        print(f"{number:>5}  {filled:>6}  {average:.2f}" if average else f"{number:>5}  {filled:>6}  n/a")


if __name__ == "__main__":
    main()
