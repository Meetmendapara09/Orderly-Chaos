// Orderly Chaos: tests for trade events and matching priority.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "orderly_chaos/limit_order_book.hpp"

using namespace orderly_chaos;

namespace {

/// A book that records every trade it produces.
struct RecordingBook {
    LimitOrderBook book;
    std::vector<Trade> trades;
    RecordingBook() {
        book.set_trade_handler([this](const Trade& trade) { trades.push_back(trade); });
    }
};

void expect_trade(const Trade& trade, UID taker, UID maker, Side taker_side,
                  Price price, Quantity quantity, Quantity maker_remaining) {
    EXPECT_EQ(taker, trade.taker_id);
    EXPECT_EQ(maker, trade.maker_id);
    EXPECT_EQ(taker_side, trade.taker_side);
    EXPECT_EQ(price, trade.price);
    EXPECT_EQ(quantity, trade.quantity);
    EXPECT_EQ(maker_remaining, trade.maker_remaining);
}

}  // namespace

TEST(Trades, no_handler_means_no_events) {
    LimitOrderBook book;
    book.limit_sell(1, 10, 100);
    EXPECT_EQ(10u, book.market_buy(2, 10));
}

TEST(Trades, resting_orders_produce_no_trades) {
    RecordingBook r;
    r.book.limit_buy(1, 10, 99);
    r.book.limit_sell(2, 10, 101);
    EXPECT_TRUE(r.trades.empty());
}

TEST(Trades, price_priority_then_time_priority) {
    RecordingBook r;
    r.book.limit_sell(1, 10, 101);
    r.book.limit_sell(2, 10, 100);  // better price, later
    r.book.limit_sell(3, 10, 100);  // same price, even later
    EXPECT_EQ(25u, r.book.market_buy(9, 25));
    ASSERT_EQ(3u, r.trades.size());
    expect_trade(r.trades[0], 9, 2, Side::Buy, 100, 10, 0);
    expect_trade(r.trades[1], 9, 3, Side::Buy, 100, 10, 0);
    expect_trade(r.trades[2], 9, 1, Side::Buy, 101, 5, 5);
}

TEST(Trades, execute_at_maker_price) {
    RecordingBook r;
    r.book.limit_buy(1, 10, 105);
    r.book.limit_sell(2, 4, 100);  // aggressive sell, trades at the bid
    ASSERT_EQ(1u, r.trades.size());
    expect_trade(r.trades[0], 2, 1, Side::Sell, 105, 4, 6);
}

TEST(Trades, handler_sees_a_consistent_book) {
    LimitOrderBook book;
    std::vector<Volume> volumes;
    book.set_trade_handler([&](const Trade&) {
        book.check_invariants();
        volumes.push_back(book.volume_sell());
    });
    book.limit_sell(1, 10, 100);
    book.limit_sell(2, 10, 101);
    book.limit_buy(3, 15, 101);
    // trades are delivered after the operation completes
    ASSERT_EQ(2u, volumes.size());
    EXPECT_EQ(5u, volumes[0]);
    EXPECT_EQ(5u, volumes[1]);
}

TEST(Trades, throwing_handler_delivers_all_then_rethrows) {
    LimitOrderBook book;
    int calls = 0;
    book.set_trade_handler([&](const Trade&) {
        ++calls;
        throw std::runtime_error("handler failed");
    });
    book.limit_sell(1, 10, 100);
    book.limit_sell(2, 10, 100);
    EXPECT_THROW(book.market_buy(3, 20), std::runtime_error);
    EXPECT_EQ(2, calls);
    EXPECT_TRUE(book.empty());  // the book was fully updated anyway
    book.check_invariants();
}

TEST(Trades, handler_can_be_removed) {
    RecordingBook r;
    r.book.set_trade_handler(nullptr);
    r.book.limit_sell(1, 10, 100);
    r.book.market_buy(2, 10);
    EXPECT_TRUE(r.trades.empty());
}

TEST(Trades, sum_of_trades_equals_filled_quantity) {
    RecordingBook r;
    for (UID id = 1; id <= 20; ++id) r.book.limit_sell(id, static_cast<Quantity>(id), 100 + id % 5);
    const Quantity filled = r.book.limit_buy(100, 150, 103);
    Quantity traded = 0;
    for (const auto& trade : r.trades) {
        traded += trade.quantity;
        EXPECT_LE(trade.price, 103u);
    }
    EXPECT_EQ(filled, traded);
    r.book.check_invariants();
}
