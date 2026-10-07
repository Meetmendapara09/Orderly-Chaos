"""Tests for input validation and error reporting."""

import unittest

from orderly_chaos import (
    DuplicateOrderIdError,
    InvalidPriceError,
    InvalidQuantityError,
    InvalidSideError,
    LimitOrderBook,
    OrderBookError,
    OrderlyChaosError,
    Side,
    UnknownOrderIdError,
)


class RejectionTest(unittest.TestCase):
    def setUp(self):
        self.book = LimitOrderBook()
        self.book.limit_buy(1, 10, 100)
        self.book.limit_sell(2, 10, 110)

    def assert_unchanged(self):
        self.assertEqual(2, len(self.book))
        self.assertEqual(20, self.book.volume())
        self.assertEqual((100, 110), (self.book.best_buy(), self.book.best_sell()))

    def test_duplicate_id(self):
        with self.assertRaises(DuplicateOrderIdError) as context:
            self.book.limit_buy(1, 5, 101)
        self.assertEqual(1, context.exception.status)
        self.assertIn("duplicate order id", str(context.exception))
        self.assert_unchanged()

    def test_zero_quantity(self):
        with self.assertRaises(InvalidQuantityError):
            self.book.limit_buy(3, 0, 100)
        with self.assertRaises(InvalidQuantityError):
            self.book.market_buy(3, 0)
        self.assert_unchanged()

    def test_zero_price(self):
        # price 0 would otherwise act like a market order
        with self.assertRaises(InvalidPriceError):
            self.book.limit_buy(3, 10, 0)
        self.assert_unchanged()

    def test_unknown_id(self):
        with self.assertRaises(UnknownOrderIdError) as context:
            self.book.cancel(99)
        self.assertIn("unknown order id", str(context.exception))
        with self.assertRaises(UnknownOrderIdError):
            self.book.get(99)
        with self.assertRaises(UnknownOrderIdError):
            self.book.reduce(99, 1)
        # UnknownOrderIdError is also a KeyError for idiomatic handling
        with self.assertRaises(KeyError):
            self.book.cancel(99)
        self.assert_unchanged()

    def test_reduce_too_much(self):
        with self.assertRaises(InvalidQuantityError):
            self.book.reduce(1, 11)
        with self.assertRaises(InvalidQuantityError):
            self.book.reduce(1, 0)
        self.assert_unchanged()

    def test_code_names_match_the_c_api(self):
        cases = [
            (DuplicateOrderIdError, 1, "OC_ERR_DUPLICATE_ORDER_ID"),
            (UnknownOrderIdError, 2, "OC_ERR_UNKNOWN_ORDER_ID"),
            (InvalidQuantityError, 3, "OC_ERR_INVALID_QUANTITY"),
            (InvalidPriceError, 4, "OC_ERR_INVALID_PRICE"),
            (InvalidSideError, 5, "OC_ERR_INVALID_SIDE"),
        ]
        for cls, status, name in cases:
            with self.subTest(cls=cls):
                self.assertEqual(status, cls.status_code)
                self.assertEqual(name, cls.code_name)

    def test_message_carries_operation_and_code_name(self):
        with self.assertRaises(UnknownOrderIdError) as context:
            self.book.cancel(99)
        message = str(context.exception)
        self.assertIn("cancel order 99", message)
        self.assertIn("unknown order id", message)
        self.assertIn("OC_ERR_UNKNOWN_ORDER_ID", message)

    def test_hierarchy(self):
        for error in (DuplicateOrderIdError, InvalidQuantityError, InvalidPriceError, InvalidSideError):
            self.assertTrue(issubclass(error, ValueError))
            self.assertTrue(issubclass(error, OrderBookError))
            self.assertTrue(issubclass(error, OrderlyChaosError))
        self.assertTrue(issubclass(UnknownOrderIdError, KeyError))


class RangeTest(unittest.TestCase):
    """Values that would be silently truncated by ctypes must be rejected."""

    def setUp(self):
        self.book = LimitOrderBook()

    def test_quantity_out_of_range(self):
        for quantity in (-1, 2**32):
            with self.subTest(quantity=quantity), self.assertRaises(InvalidQuantityError):
                self.book.limit_buy(1, quantity, 100)
        self.assertEqual(0, len(self.book))

    def test_price_out_of_range(self):
        for price in (-1, 2**64):
            with self.subTest(price=price), self.assertRaises(InvalidPriceError):
                self.book.limit_buy(1, 1, price)

    def test_order_id_out_of_range(self):
        for order_id in (-1, 2**64):
            with self.subTest(order_id=order_id), self.assertRaises(ValueError):
                self.book.limit_buy(order_id, 1, 100)
        self.book.limit_buy(2**64 - 1, 1, 100)  # the maximum is fine
        self.assertTrue(self.book.has(2**64 - 1))

    def test_wrong_types(self):
        for bad in (1.5, "10", None, True):
            with self.subTest(value=bad), self.assertRaises(TypeError):
                self.book.limit_buy(1, bad, 100)

    def test_invalid_side(self):
        for side in ("long", 2, -1, None, 0.5):
            with self.subTest(side=side), self.assertRaises(InvalidSideError):
                self.book.limit(side, 1, 1, 100)
        with self.assertRaises(InvalidSideError):
            Side.parse("short")


if __name__ == "__main__":
    unittest.main()
