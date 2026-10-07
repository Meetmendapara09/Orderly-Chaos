"""Value types used by the Python API."""

import enum
from dataclasses import dataclass
from typing import Union

from .errors import InvalidSideError


class Side(enum.IntEnum):
    """The side of an order. Values match the C API (OC_SIDE_*)."""

    SELL = 0
    BUY = 1

    @property
    def opposite(self) -> "Side":
        """Return the other side."""
        return Side.BUY if self is Side.SELL else Side.SELL

    @classmethod
    def parse(cls, value: "SideLike") -> "Side":
        """Convert a Side, ``"buy"``/``"sell"`` string, or legacy bool to a Side.

        ``True`` means buy and ``False`` means sell, matching versions before
        0.2.0. Integers 0 and 1 are also accepted.

        Raises:
            InvalidSideError: if the value is not a recognised side

        """
        if isinstance(value, Side):
            return value
        if isinstance(value, str):
            text = value.strip().lower()
            if text in ("buy", "bid", "b"):
                return cls.BUY
            if text in ("sell", "ask", "s"):
                return cls.SELL
        elif isinstance(value, (bool, int)) and int(value) in (0, 1):
            return cls(int(value))
        raise InvalidSideError(f"invalid side: {value!r} (expected Side.BUY or Side.SELL)")


#: anything accepted where a side is expected
SideLike = Union[Side, str, bool, int]


@dataclass(frozen=True)
class Order:
    """A snapshot of a resting order."""

    #: the order's ID
    order_id: int
    #: the side of the order
    side: Side
    #: the open (unfilled) quantity
    quantity: int
    #: the limit price in ticks
    price: int


@dataclass(frozen=True)
class Trade:
    """An execution between an incoming (taker) and a resting (maker) order."""

    #: the ID of the incoming order that removed liquidity
    taker_id: int
    #: the ID of the resting order that provided liquidity
    maker_id: int
    #: the side of the incoming order
    taker_side: Side
    #: the execution price (the maker's limit price)
    price: int
    #: the executed quantity
    quantity: int
    #: the maker's open quantity after the trade (0 means fully filled)
    maker_remaining: int


@dataclass(frozen=True)
class PriceLevel:
    """An aggregated price level of the book (see ``LimitOrderBook.depth``)."""

    #: the price of the level
    price: int
    #: the total open quantity at the level
    volume: int
    #: the number of orders at the level
    count: int


__all__ = ["Order", "PriceLevel", "Side", "SideLike", "Trade"]
