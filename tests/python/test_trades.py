"""Tests for trade events delivered to the on_trade handler."""

import gc
import unittest
import weakref

from orderly_chaos import LimitOrderBook, Side, Trade


class TradeEventTest(unittest.TestCase):
    def test_trade_fields(self):
        trades = []
        book = LimitOrderBook(on_trade=trades.append)
        book.limit_buy(1, 10, 105)
        book.limit_sell(2, 4, 100)  # aggressive: executes at the resting price
        self.assertEqual(
            [Trade(taker_id=2, maker_id=1, taker_side=Side.SELL, price=105, quantity=4, maker_remaining=6)],
            trades,
        )

    def test_sum_of_trades_equals_filled(self):
        trades = []
        book = LimitOrderBook(on_trade=trades.append)
        for order_id in range(1, 21):
            book.limit_sell(order_id, order_id, 100 + order_id % 5)
        filled = book.limit_buy(100, 150, 103)
        self.assertEqual(filled, sum(t.quantity for t in trades))
        self.assertTrue(all(t.price <= 103 for t in trades))

    def test_handler_can_be_changed_and_removed(self):
        first, second = [], []
        book = LimitOrderBook(on_trade=first.append)
        book.limit_sell(1, 10, 100)
        book.market_buy(2, 1)
        book.on_trade = second.append
        book.market_buy(3, 1)
        book.on_trade = None
        book.market_buy(4, 1)
        self.assertEqual([2], [t.taker_id for t in first])
        self.assertEqual([3], [t.taker_id for t in second])
        self.assertIsNone(book.on_trade)

    def test_handler_may_query_the_book(self):
        seen = []
        book = LimitOrderBook()
        book.on_trade = lambda trade: seen.append((trade.maker_id, book.volume_sell(), book.has(trade.maker_id)))
        book.limit_sell(1, 10, 100)
        book.limit_sell(2, 10, 101)
        book.limit_buy(3, 15, 101)
        # delivered after the operation, so the book is already updated
        self.assertEqual([(1, 5, False), (2, 5, True)], seen)

    def test_handler_exception_is_reraised_after_update(self):
        calls = []

        def failing(trade):
            calls.append(trade)
            raise RuntimeError("boom")

        book = LimitOrderBook(on_trade=failing)
        book.limit_sell(1, 10, 100)
        book.limit_sell(2, 10, 100)
        with self.assertRaisesRegex(RuntimeError, "boom"):
            book.market_buy(3, 20)
        self.assertEqual(2, len(calls))  # every trade was still delivered
        self.assertEqual(0, len(book))  # and the book was fully updated
        book.limit_sell(4, 1, 100)  # the error does not leak into later calls
        self.assertEqual(1, len(book))

    def test_rejects_non_callable(self):
        with self.assertRaises(TypeError):
            LimitOrderBook(on_trade=42)

    def test_book_with_handler_is_freed_promptly(self):
        book = LimitOrderBook(on_trade=lambda trade: None)
        reference = weakref.ref(book)
        gc.disable()  # prove there is no reference cycle to collect
        try:
            del book
            self.assertIsNone(reference())
        finally:
            gc.enable()


if __name__ == "__main__":
    unittest.main()
