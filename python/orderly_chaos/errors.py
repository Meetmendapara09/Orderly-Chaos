"""Exceptions raised by Orderly Chaos."""

from typing import Dict, Optional, Type


class OrderlyChaosError(Exception):
    """Base class for every exception raised by Orderly Chaos."""


class LibraryError(OrderlyChaosError, OSError):
    """The native shared library is missing, incompatible, or failed to load."""


class OrderBookError(OrderlyChaosError):
    """An order book operation was rejected. The book was not modified.

    Attributes:
        status: the numeric status code returned by the C API

    """

    #: the C API status code for this error type
    status_code: int = 0
    #: the symbolic C API status name, shared across all three APIs
    code_name: str = "OC_ERR_INTERNAL"

    def __init__(self, message: str, status: Optional[int] = None) -> None:
        super().__init__(message)
        self.status = self.status_code if status is None else status

    def __str__(self) -> str:
        # KeyError subclasses would otherwise repr() the message
        return str(self.args[0]) if self.args else ""


class DuplicateOrderIdError(OrderBookError, ValueError):
    """A limit order used an ID that is already resting in the book."""

    status_code = 1
    code_name = "OC_ERR_DUPLICATE_ORDER_ID"


class UnknownOrderIdError(OrderBookError, KeyError):
    """No resting order has the given ID."""

    status_code = 2
    code_name = "OC_ERR_UNKNOWN_ORDER_ID"


class InvalidQuantityError(OrderBookError, ValueError):
    """The quantity is zero, out of range, or exceeds the open quantity."""

    status_code = 3
    code_name = "OC_ERR_INVALID_QUANTITY"


class InvalidPriceError(OrderBookError, ValueError):
    """The limit price is zero or out of range."""

    status_code = 4
    code_name = "OC_ERR_INVALID_PRICE"


class InvalidSideError(OrderBookError, ValueError):
    """The side is not a valid Side value."""

    status_code = 5
    code_name = "OC_ERR_INVALID_SIDE"


_BY_STATUS: Dict[int, Type[OrderBookError]] = {
    cls.status_code: cls
    for cls in (
        DuplicateOrderIdError,
        UnknownOrderIdError,
        InvalidQuantityError,
        InvalidPriceError,
        InvalidSideError,
    )
}


def error_for_status(status: int, message: str) -> OrderlyChaosError:
    """Return the exception instance matching a C API status code.

    The message carries the symbolic status name (for example
    ``OC_ERR_UNKNOWN_ORDER_ID``) so an error seen in Python can be looked
    up in the C and C++ references too.
    """
    cls = _BY_STATUS.get(status)
    if cls is not None:
        return cls(f"{message} [{cls.code_name}]", status)
    if status == 7:
        return OrderlyChaosError(f"{message} (out of memory)")
    return OrderlyChaosError(f"{message} (internal error, status {status})")


__all__ = [
    "DuplicateOrderIdError",
    "InvalidPriceError",
    "InvalidQuantityError",
    "InvalidSideError",
    "LibraryError",
    "OrderBookError",
    "OrderlyChaosError",
    "UnknownOrderIdError",
]
