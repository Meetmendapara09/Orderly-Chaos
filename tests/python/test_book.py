"""Behavioural tests for orderly_chaos.LimitOrderBook."""

import unittest

from orderly_chaos import LimitOrderBook, Order, PriceLevel, Side


class EmptyBookTest(unittest.TestCase):
    def test_empty_book_has_no_prices_volume_or_orders(self):
        book = LimitOrderBook()
        self.assertIsNone(book.best_sell())
        self.assertIsNone(book.best_buy())
        self.assertIsNone(book.best(Side.SELL))
        self.assertIsNone(book.best(Side.BUY))
        self.assertIsNone(book.mid_price())
        self.assertIsNone(book.spread())
        self.assertEqual(0, book.volume())
        self.assertEqual(0, book.volume(100))
        self.assertEqual(0, book.volume_sell())
        self.assertEqual(0, book.volume_sell(100))
        self.assertEqual(0, book.volume_buy())
        self.assertEqual(0, book.volume_buy(100))
        self.assertEqual(0, book.best_volume(Side.BUY))
        self.assertEqual(0, book.count())
        self.assertEqual(0, book.count_at(100))
        self.assertEqual(0, book.count_sell())
        self.assertEqual(0, book.count_buy())
        self.assertEqual(0, len(book))
        self.assertEqual([], book.depth(Side.BUY))


class LimitOrderTest(unittest.TestCase):
    def assert_single_order(self, book, side, quantity, price):
        other = side.opposite
        self.assertEqual(price, book.best(side))
        self.assertIsNone(book.best(other))
        self.assertEqual(quantity, book.volume())
        self.assertEqual(quantity, book.volume(price))
        self.assertEqual(1, book.count_at(price))
        self.assertEqual(1, book.count())
        if side is Side.SELL:
            self.assertEqual(quantity, book.volume_sell(price))
            self.assertEqual((1, 0), (book.count_sell(), book.count_buy()))
        else:
            self.assertEqual(quantity, book.volume_buy(price))
            self.assertEqual((0, 1), (book.count_sell(), book.count_buy()))

    def test_sell_limit_rests(self):
        book = LimitOrderBook()
        self.assertEqual(0, book.limit_sell(1, 100, 50))
        self.assert_single_order(book, Side.SELL, 100, 50)

    def test_buy_limit_rests(self):
        book = LimitOrderBook()
        self.assertEqual(0, book.limit_buy(1, 100, 50))
        self.assert_single_order(book, Side.BUY, 100, 50)

    def test_side_can_be_given_many_ways(self):
        for side, expected in [
            (Side.SELL, Side.SELL),
            ("sell", Side.SELL),
            ("ASK", Side.SELL),
            (False, Side.SELL),  # legacy bool API: False = sell
            (0, Side.SELL),
            (Side.BUY, Side.BUY),
            ("buy", Side.BUY),
            ("Bid", Side.BUY),
            (True, Side.BUY),  # legacy bool API: True = buy
            (1, Side.BUY),
        ]:
            with self.subTest(side=side):
                book = LimitOrderBook()
                book.limit(side, 1, 100, 50)
                self.assert_single_order(book, expected, 100, 50)

    def test_get_returns_snapshot(self):
        book = LimitOrderBook()
        book.limit_buy(7, 25, 99)
        self.assertEqual(Order(order_id=7, side=Side.BUY, quantity=25, price=99), book.get(7))
        self.assertTrue(book.has(7))
        self.assertIn(7, book)
        self.assertNotIn(8, book)
        self.assertNotIn("not an id", book)

    def test_prices_and_spread(self):
        book = LimitOrderBook()
        book.limit_buy(1, 10, 100)
        self.assertEqual(100, book.mid_price())
        book.limit_sell(2, 10, 105)
        self.assertEqual(102, book.mid_price())  # rounds down
        self.assertEqual(5, book.spread())

    def test_large_prices_are_exact(self):
        book = LimitOrderBook()
        price = 2**64 - 1
        book.limit_sell(1, 2**32 - 1, price)
        self.assertEqual(price, book.best_sell())
        self.assertEqual(2**32 - 1, book.volume_sell(price))


class MatchingTest(unittest.TestCase):
    def test_incoming_buy_fills_resting_sell(self):
        book = LimitOrderBook()
        book.limit_sell(1, 100, 50)
        self.assertEqual(100, book.limit_buy(2, 100, 50))
        self.assertEqual(0, len(book))
        self.assertFalse(book.has(1))
        self.assertFalse(book.has(2))

    def test_incoming_sell_fills_resting_buy(self):
        book = LimitOrderBook()
        book.limit_buy(1, 100, 50)
        self.assertEqual(100, book.limit_sell(2, 100, 49))
        self.assertEqual(0, len(book))

    def test_partial_fill_rests_remainder(self):
        book = LimitOrderBook()
        book.limit_sell(1, 30, 50)
        self.assertEqual(30, book.limit_buy(2, 100, 51))
        self.assertEqual(Order(2, Side.BUY, 70, 51), book.get(2))
        self.assertIsNone(book.best_sell())

    def test_non_crossing_orders_rest(self):
        book = LimitOrderBook()
        book.limit_sell(1, 10, 51)
        self.assertEqual(0, book.limit_buy(2, 10, 50))
        self.assertEqual(2, len(book))
        self.assertEqual(1, book.spread())

    def test_market_orders(self):
        book = LimitOrderBook()
        self.assertEqual(0, book.market_sell(1, 100))  # empty book: nothing happens
        self.assertEqual(0, book.market_buy(2, 100))
        book.limit_sell(3, 60, 50)
        book.limit_sell(4, 60, 51)
        self.assertEqual(100, book.market_buy(5, 100))
        self.assertEqual(20, book.volume_sell())
        self.assertEqual(20, book.market(Side.BUY, 6, 500))  # remainder discarded
        self.assertEqual(0, len(book))
        self.assertFalse(book.has(6))

    def test_price_time_priority(self):
        trades = []
        book = LimitOrderBook(on_trade=trades.append)
        book.limit_sell(1, 10, 101)
        book.limit_sell(2, 10, 100)
        book.limit_sell(3, 10, 100)
        book.market_buy(9, 25)
        self.assertEqual([2, 3, 1], [t.maker_id for t in trades])
        self.assertEqual([100, 100, 101], [t.price for t in trades])


class OrderManagementTest(unittest.TestCase):
    def test_cancel(self):
        for side in Side:
            with self.subTest(side=side):
                book = LimitOrderBook()
                book.limit(side, 1, 100, 50)
                book.cancel(1)
                self.assertEqual(0, len(book))
                self.assertIsNone(book.best(side))

    def test_reduce_keeps_priority(self):
        book = LimitOrderBook()
        book.limit_sell(1, 10, 50)
        book.limit_sell(2, 10, 50)
        book.reduce(1, 4)
        self.assertEqual(6, book.get(1).quantity)
        self.assertEqual(16, book.volume(50))
        book.market_buy(3, 6)
        self.assertFalse(book.has(1))
        self.assertTrue(book.has(2))

    def test_reduce_to_zero_cancels(self):
        book = LimitOrderBook()
        book.limit_buy(1, 10, 50)
        book.reduce(1, 10)
        self.assertFalse(book.has(1))

    def test_clear(self):
        book = LimitOrderBook()
        for order_id in range(1, 101):
            book.limit_sell(order_id, 10, 100 + order_id % 10)
            book.limit_buy(1000 + order_id, 10, 50 + order_id % 10)
        self.assertEqual(200, len(book))
        book.clear()
        self.assertEqual(0, len(book))
        self.assertEqual(0, book.volume())
        book.limit_buy(1, 1, 1)  # IDs are free again
        self.assertTrue(book.has(1))

    def test_depth(self):
        book = LimitOrderBook()
        book.limit_buy(1, 10, 99)
        book.limit_buy(2, 5, 100)
        book.limit_buy(3, 7, 100)
        self.assertEqual(
            [PriceLevel(100, 12, 2), PriceLevel(99, 10, 1)],
            book.depth(Side.BUY),
        )
        self.assertEqual([PriceLevel(100, 12, 2)], book.depth("buy", levels=1))
        self.assertEqual([], book.depth(Side.BUY, levels=0))


class LifecycleTest(unittest.TestCase):
    def test_context_manager_closes(self):
        with LimitOrderBook() as book:
            book.limit_buy(1, 1, 1)
        self.assertTrue(book.closed)
        self.assertEqual("<LimitOrderBook closed>", repr(book))
        with self.assertRaises(Exception):
            book.limit_buy(2, 1, 1)
        book.close()  # idempotent

    def test_repr(self):
        book = LimitOrderBook()
        book.limit_buy(1, 10, 99)
        self.assertEqual("<LimitOrderBook orders=1 bid=99 ask=None volume=10>", repr(book))

    def test_many_books_are_independent(self):
        books = [LimitOrderBook() for _ in range(50)]
        for index, book in enumerate(books):
            book.limit_buy(1, index + 1, 100)
        self.assertEqual(list(range(1, 51)), [b.volume() for b in books])


if __name__ == "__main__":
    unittest.main()
