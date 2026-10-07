"""The Python interface to the Orderly Chaos limit order book."""

import ctypes
import operator
import weakref
from typing import Callable, List, Optional

from . import _library as native
from .errors import (
    InvalidPriceError,
    InvalidQuantityError,
    OrderlyChaosError,
    error_for_status,
)
from .types import Order, PriceLevel, Side, SideLike, Trade

_UINT32_MAX = 2**32 - 1
_UINT64_MAX = 2**64 - 1

#: a function that receives each trade
TradeHandler = Callable[[Trade], None]


def _integer(value: object, name: str, maximum: int, error: type) -> int:
    """Validate an unsigned integer argument (ctypes would silently wrap it)."""
    if isinstance(value, bool):
        raise TypeError(f"{name} must be an int, not bool")
    try:
        number = operator.index(value)  # type: ignore[arg-type]
    except TypeError:
        raise TypeError(f"{name} must be an int, not {type(value).__name__}") from None
    if not 0 <= number <= maximum:
        raise error(f"{name} must be between 0 and {maximum}, got {number}")
    return number


class LimitOrderBook:
    """A price-time priority limit order book (matching engine).

    Limit orders that cross the spread trade immediately and any remainder
    rests in the book. Market orders trade immediately and any unfilled
    remainder is discarded. Resting orders match best price first, then
    oldest first, and trades execute at the resting order's price.

    Prices are integers in ticks (for example cents); ``0`` is not a valid
    limit price. Order IDs are chosen by the caller and must be unique among
    resting orders.

    Example:
        >>> from orderly_chaos import LimitOrderBook, Side
        >>> book = LimitOrderBook()
        >>> book.limit(Side.SELL, order_id=1, quantity=100, price=10_050)
        0
        >>> book.limit(Side.BUY, order_id=2, quantity=40, price=10_050)
        40
        >>> book.volume_sell()
        60

    A book is safe to share between threads (calls are serialised by the
    GIL), but each operation is atomic only on its own.

    Args:
        on_trade: optional function called with a :class:`Trade` for every
            execution, after the operation has fully updated the book

    """

    def __init__(self, on_trade: Optional[TradeHandler] = None) -> None:
        self._lib = native.library()
        handle = self._lib.oc_book_new()
        if not handle:
            raise MemoryError("failed to allocate an order book")
        self._handle: Optional[int] = handle
        self._on_trade: Optional[TradeHandler] = None
        self._callback = None  # keeps the ctypes callback alive
        self._handler_error: Optional[BaseException] = None
        self.on_trade = on_trade

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def close(self) -> None:
        """Free the native book. The object cannot be used afterwards."""
        handle, self._handle = getattr(self, "_handle", None), None
        if handle:
            self._lib.oc_book_free(handle)

    def __enter__(self) -> "LimitOrderBook":
        return self

    def __exit__(self, *exc_info: object) -> None:
        self.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:  # pragma: no cover - interpreter shutdown
            pass

    @property
    def closed(self) -> bool:
        """True once :meth:`close` has been called."""
        return self._handle is None

    def _book(self) -> int:
        if self._handle is None:
            raise OrderlyChaosError("the order book has been closed")
        return self._handle

    # ------------------------------------------------------------------
    # Trade events
    # ------------------------------------------------------------------

    @property
    def on_trade(self) -> Optional[TradeHandler]:
        """The function receiving each :class:`Trade`, or ``None``.

        The handler runs after the operation has fully updated the book, so
        it may query the book, but it must not modify it. If it raises, the
        remaining trades of that operation are still delivered and the first
        exception is re-raised from the operation that caused the trades.
        """
        return self._on_trade

    @on_trade.setter
    def on_trade(self, handler: Optional[TradeHandler]) -> None:
        if handler is not None and not callable(handler):
            raise TypeError("on_trade must be callable or None")
        book = self._book()
        if handler is None:
            self._check(self._lib.oc_book_set_trade_callback(book, native.TradeCallback(), None), "set trade handler")
            self._on_trade = None
            self._callback = None
            return

        owner = weakref.ref(self)  # avoid a self -> callback -> self cycle

        def deliver(trade_pointer, _user_data):  # type: ignore[no-untyped-def]
            raw = trade_pointer.contents
            trade = Trade(
                taker_id=raw.taker_id,
                maker_id=raw.maker_id,
                taker_side=Side(raw.taker_side),
                price=raw.price,
                quantity=raw.quantity,
                maker_remaining=raw.maker_remaining,
            )
            try:
                handler(trade)
            except BaseException as error:  # re-raised after the C call returns
                book = owner()
                if book is not None and book._handler_error is None:
                    book._handler_error = error

        callback = native.TradeCallback(deliver)
        self._check(self._lib.oc_book_set_trade_callback(book, callback, None), "set trade handler")
        self._on_trade = handler
        self._callback = callback

    def _raise_handler_error(self) -> None:
        error, self._handler_error = self._handler_error, None
        if error is not None:
            raise error

    def _check(self, status: int, action: str) -> None:
        if status != 0:
            message = self._lib.oc_status_string(status).decode("ascii")
            raise error_for_status(status, f"{action}: {message}")

    # ------------------------------------------------------------------
    # Order entry
    # ------------------------------------------------------------------

    def limit(self, side: SideLike, order_id: int, quantity: int, price: int) -> int:
        """Submit a limit order.

        Any part that crosses the spread trades immediately; the remainder
        rests in the book until it is filled, reduced, or cancelled.

        Args:
            side: ``Side.BUY`` or ``Side.SELL`` (also ``"buy"``/``"sell"``)
            order_id: an ID that is not used by any resting order
            quantity: the quantity, at least 1
            price: the limit price in ticks, at least 1

        Returns:
            the quantity filled immediately

        Raises:
            DuplicateOrderIdError: if ``order_id`` is already resting
            InvalidQuantityError: if ``quantity`` is 0 or out of range
            InvalidPriceError: if ``price`` is 0 or out of range

        """
        side = Side.parse(side)
        order_id = _integer(order_id, "order_id", _UINT64_MAX, ValueError)
        quantity = _integer(quantity, "quantity", _UINT32_MAX, InvalidQuantityError)
        price = _integer(price, "price", _UINT64_MAX, InvalidPriceError)
        filled = ctypes.c_uint32(0)
        status = self._lib.oc_book_limit(self._book(), side, order_id, quantity, price, ctypes.byref(filled))
        self._check(status, f"limit order {order_id}")
        self._raise_handler_error()
        return filled.value

    def limit_buy(self, order_id: int, quantity: int, price: int) -> int:
        """Submit a buy limit order. See :meth:`limit`."""
        return self.limit(Side.BUY, order_id, quantity, price)

    def limit_sell(self, order_id: int, quantity: int, price: int) -> int:
        """Submit a sell limit order. See :meth:`limit`."""
        return self.limit(Side.SELL, order_id, quantity, price)

    def market(self, side: SideLike, order_id: int, quantity: int) -> int:
        """Submit a market (immediate-or-cancel) order.

        Market orders never rest; ``order_id`` only labels the resulting
        trades (as ``taker_id``).

        Returns:
            the quantity filled; any unfilled remainder is discarded

        Raises:
            InvalidQuantityError: if ``quantity`` is 0 or out of range

        """
        side = Side.parse(side)
        order_id = _integer(order_id, "order_id", _UINT64_MAX, ValueError)
        quantity = _integer(quantity, "quantity", _UINT32_MAX, InvalidQuantityError)
        filled = ctypes.c_uint32(0)
        status = self._lib.oc_book_market(self._book(), side, order_id, quantity, ctypes.byref(filled))
        self._check(status, f"market order {order_id}")
        self._raise_handler_error()
        return filled.value

    def market_buy(self, order_id: int, quantity: int) -> int:
        """Submit a buy market order. See :meth:`market`."""
        return self.market(Side.BUY, order_id, quantity)

    def market_sell(self, order_id: int, quantity: int) -> int:
        """Submit a sell market order. See :meth:`market`."""
        return self.market(Side.SELL, order_id, quantity)

    def cancel(self, order_id: int) -> None:
        """Cancel a resting order.

        Raises:
            UnknownOrderIdError: if no resting order has this ID

        """
        order_id = _integer(order_id, "order_id", _UINT64_MAX, ValueError)
        self._check(self._lib.oc_book_cancel(self._book(), order_id), f"cancel order {order_id}")

    def reduce(self, order_id: int, quantity: int) -> None:
        """Reduce a resting order's quantity, keeping its place in the queue.

        Reducing by the full open quantity cancels the order.

        Raises:
            UnknownOrderIdError: if no resting order has this ID
            InvalidQuantityError: if ``quantity`` is 0 or exceeds the open quantity

        """
        order_id = _integer(order_id, "order_id", _UINT64_MAX, ValueError)
        quantity = _integer(quantity, "quantity", _UINT32_MAX, InvalidQuantityError)
        self._check(self._lib.oc_book_reduce(self._book(), order_id, quantity), f"reduce order {order_id}")

    def clear(self) -> None:
        """Remove every order from the book."""
        self._check(self._lib.oc_book_clear(self._book()), "clear")

    # ------------------------------------------------------------------
    # Order queries
    # ------------------------------------------------------------------

    def has(self, order_id: int) -> bool:
        """Return True if an order with this ID is resting in the book."""
        order_id = _integer(order_id, "order_id", _UINT64_MAX, ValueError)
        return bool(self._lib.oc_book_has(self._book(), order_id))

    def __contains__(self, order_id: object) -> bool:
        try:
            return self.has(order_id)  # type: ignore[arg-type]
        except (TypeError, ValueError):
            return False

    def get(self, order_id: int) -> Order:
        """Return a snapshot of a resting order.

        Raises:
            UnknownOrderIdError: if no resting order has this ID

        """
        order_id = _integer(order_id, "order_id", _UINT64_MAX, ValueError)
        raw = native.OcOrder()
        self._check(self._lib.oc_book_get(self._book(), order_id, ctypes.byref(raw)), f"get order {order_id}")
        return Order(order_id=raw.order_id, side=Side(raw.side), quantity=raw.quantity, price=raw.price)

    # ------------------------------------------------------------------
    # Prices (None when there is no price)
    # ------------------------------------------------------------------

    def best(self, side: SideLike) -> Optional[int]:
        """Return the best price of a side (highest bid / lowest ask), or None."""
        return self._lib.oc_book_best_price(self._book(), Side.parse(side)) or None

    def best_buy(self) -> Optional[int]:
        """Return the highest bid, or None if there are no buy orders."""
        return self.best(Side.BUY)

    def best_sell(self) -> Optional[int]:
        """Return the lowest ask, or None if there are no sell orders."""
        return self.best(Side.SELL)

    def mid_price(self) -> Optional[int]:
        """Return the midpoint of the best bid and ask (rounded down).

        If only one side has orders, its best price is returned; if the book
        is empty, None.
        """
        return self._lib.oc_book_mid_price(self._book()) or None

    def spread(self) -> Optional[int]:
        """Return best ask minus best bid, or None unless both sides have orders."""
        bid, ask = self.best_buy(), self.best_sell()
        return None if bid is None or ask is None else ask - bid

    # ------------------------------------------------------------------
    # Volumes and counts
    # ------------------------------------------------------------------

    def volume(self, price: Optional[int] = None) -> int:
        """Return the open quantity at ``price``, or in the whole book if None."""
        if price is None:
            return self._lib.oc_book_volume(self._book())
        price = _integer(price, "price", _UINT64_MAX, InvalidPriceError)
        return self._lib.oc_book_volume_at(self._book(), price)

    def volume_buy(self, price: Optional[int] = None) -> int:
        """Return the open buy quantity at ``price``, or in total if None."""
        return self._side_volume(Side.BUY, price)

    def volume_sell(self, price: Optional[int] = None) -> int:
        """Return the open sell quantity at ``price``, or in total if None."""
        return self._side_volume(Side.SELL, price)

    def best_volume(self, side: SideLike) -> int:
        """Return the open quantity at the best price of a side (0 if empty)."""
        return self._lib.oc_book_best_volume(self._book(), Side.parse(side))

    def _side_volume(self, side: Side, price: Optional[int]) -> int:
        if price is None:
            return self._lib.oc_book_side_volume(self._book(), side)
        price = _integer(price, "price", _UINT64_MAX, InvalidPriceError)
        return self._lib.oc_book_side_volume_at(self._book(), side, price)

    def count(self) -> int:
        """Return the number of resting orders."""
        return self._lib.oc_book_count(self._book())

    def __len__(self) -> int:
        return self.count()

    def count_buy(self) -> int:
        """Return the number of resting buy orders."""
        return self._lib.oc_book_side_count(self._book(), Side.BUY)

    def count_sell(self) -> int:
        """Return the number of resting sell orders."""
        return self._lib.oc_book_side_count(self._book(), Side.SELL)

    def count_at(self, price: int) -> int:
        """Return the number of orders resting at a price."""
        price = _integer(price, "price", _UINT64_MAX, InvalidPriceError)
        return self._lib.oc_book_count_at(self._book(), price)

    def depth(self, side: SideLike, levels: int = 10) -> List[PriceLevel]:
        """Return up to ``levels`` aggregated price levels of a side, best first.

        This is a "level 2" market-depth snapshot.
        """
        side = Side.parse(side)
        levels = _integer(levels, "levels", 2**31 - 1, ValueError)
        if levels == 0:
            return []
        buffer = (native.OcLevel * levels)()
        written = self._lib.oc_book_depth(self._book(), side, buffer, levels)
        return [PriceLevel(price=lvl.price, volume=lvl.volume, count=lvl.count) for lvl in buffer[:written]]

    def __repr__(self) -> str:
        if self.closed:
            return "<LimitOrderBook closed>"
        return (
            f"<LimitOrderBook orders={self.count()} "
            f"bid={self.best_buy()} ask={self.best_sell()} volume={self.volume()}>"
        )


__all__ = ["LimitOrderBook", "TradeHandler"]
