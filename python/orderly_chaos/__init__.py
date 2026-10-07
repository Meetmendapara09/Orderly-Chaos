"""Orderly Chaos: a fast price-time priority limit order book (matching engine).

Quick start::

    from orderly_chaos import LimitOrderBook, Side

    book = LimitOrderBook(on_trade=print)
    book.limit(Side.SELL, order_id=1, quantity=100, price=10_050)
    book.limit(Side.BUY, order_id=2, quantity=40, price=10_050)  # trades 40
    print(book.best_sell(), book.volume_sell())                 # 10050 60

See https://github.com/Meetmendapara09/Orderly-Chaos for documentation.
"""

from ._library import native_version
from ._version import __version__
from .book import LimitOrderBook, TradeHandler
from .errors import (
    DuplicateOrderIdError,
    InvalidPriceError,
    InvalidQuantityError,
    InvalidSideError,
    LibraryError,
    OrderBookError,
    OrderlyChaosError,
    UnknownOrderIdError,
)
from .types import Order, PriceLevel, Side, Trade

__author__ = "Meet Mendapara"

__all__ = [
    "DuplicateOrderIdError",
    "InvalidPriceError",
    "InvalidQuantityError",
    "InvalidSideError",
    "LibraryError",
    "LimitOrderBook",
    "Order",
    "OrderBookError",
    "OrderlyChaosError",
    "PriceLevel",
    "Side",
    "Trade",
    "TradeHandler",
    "UnknownOrderIdError",
    "__version__",
    "native_version",
]
