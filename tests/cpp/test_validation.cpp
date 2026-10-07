// Orderly Chaos: tests for input validation, return values, and queries.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#include <gtest/gtest.h>
#include <limits>
#include <type_traits>
#include <utility>
#include "orderly_chaos/orderly_chaos.hpp"

using namespace orderly_chaos;

namespace {

/// Assert that `body` throws OrderBookError with the given code.
template<typename Body>
void expect_error(ErrorCode code, Body&& body) {
    try {
        body();
        FAIL() << "expected OrderBookError(" << to_string(code) << ")";
    } catch (const OrderBookError& error) {
        EXPECT_EQ(code, error.code()) << error.what();
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// MARK: Rejections leave the book unchanged
// ---------------------------------------------------------------------------

TEST(Validation, limit_rejects_zero_quantity) {
    LimitOrderBook book;
    expect_error(ErrorCode::InvalidQuantity, [&] { book.limit_buy(1, 0, 100); });
    EXPECT_TRUE(book.empty());
    EXPECT_FALSE(book.has(1));
}

TEST(Validation, limit_rejects_zero_price) {
    LimitOrderBook book;
    book.limit_sell(1, 10, 100);
    // price 0 would otherwise behave like a market order and sweep the book
    expect_error(ErrorCode::InvalidPrice, [&] { book.limit_buy(2, 10, 0); });
    EXPECT_EQ(10u, book.volume_sell());
    EXPECT_FALSE(book.has(2));
    book.check_invariants();
}

TEST(Validation, limit_rejects_duplicate_resting_id) {
    LimitOrderBook book;
    book.limit_buy(7, 10, 100);
    expect_error(ErrorCode::DuplicateOrderId, [&] { book.limit_buy(7, 99, 101); });
    expect_error(ErrorCode::DuplicateOrderId, [&] { book.limit_sell(7, 99, 200); });
    EXPECT_EQ(1u, book.count());
    EXPECT_EQ(10u, book.volume());
    EXPECT_EQ(100u, book.best_buy());
    EXPECT_EQ(10u, book.get(7).quantity);
    book.check_invariants();
}

TEST(Validation, id_can_be_reused_after_the_order_leaves_the_book) {
    LimitOrderBook book;
    book.limit_buy(1, 10, 100);
    book.cancel(1);
    EXPECT_NO_THROW(book.limit_buy(1, 5, 90));
    book.limit_sell(2, 5, 90);  // fills order 1 completely
    EXPECT_FALSE(book.has(1));
    EXPECT_NO_THROW(book.limit_sell(1, 3, 95));
    book.check_invariants();
}

TEST(Validation, cancel_unknown_id_throws) {
    LimitOrderBook book;
    expect_error(ErrorCode::UnknownOrderId, [&] { book.cancel(42); });
    book.limit_buy(1, 10, 100);
    book.cancel(1);
    expect_error(ErrorCode::UnknownOrderId, [&] { book.cancel(1); });
}

TEST(Validation, get_unknown_id_throws) {
    LimitOrderBook book;
    expect_error(ErrorCode::UnknownOrderId, [&] { book.get(42); });
}

TEST(Validation, market_rejects_zero_quantity) {
    LimitOrderBook book;
    book.limit_sell(1, 10, 100);
    expect_error(ErrorCode::InvalidQuantity, [&] { book.market_buy(2, 0); });
    EXPECT_EQ(10u, book.volume_sell());
}

TEST(Validation, error_names_match_the_c_api) {
    // every code maps to the same symbolic name used by oc_status_string
    // callers and the C/Python references, so an error is traceable
    EXPECT_STREQ("OC_ERR_DUPLICATE_ORDER_ID", code_name(ErrorCode::DuplicateOrderId));
    EXPECT_STREQ("OC_ERR_UNKNOWN_ORDER_ID", code_name(ErrorCode::UnknownOrderId));
    EXPECT_STREQ("OC_ERR_INVALID_QUANTITY", code_name(ErrorCode::InvalidQuantity));
    EXPECT_STREQ("OC_ERR_INVALID_PRICE", code_name(ErrorCode::InvalidPrice));
}

TEST(Validation, error_message_is_descriptive) {
    LimitOrderBook book;
    try {
        book.cancel(42);
        FAIL();
    } catch (const OrderBookError& error) {
        EXPECT_NE(std::string::npos, std::string(error.what()).find("42"));
        EXPECT_NE(std::string::npos, std::string(error.what()).find("unknown order id"));
        EXPECT_NE(std::string::npos, std::string(error.what()).find("OC_ERR_UNKNOWN_ORDER_ID"));
    }
}

// ---------------------------------------------------------------------------
// MARK: Reduce
// ---------------------------------------------------------------------------

TEST(Reduce, partial_reduce_keeps_time_priority) {
    LimitOrderBook book;
    book.limit_sell(1, 10, 100);
    book.limit_sell(2, 10, 100);
    book.reduce(1, 4);
    EXPECT_EQ(6u, book.get(1).quantity);
    EXPECT_EQ(16u, book.volume_sell(100));
    EXPECT_EQ(2u, book.count_at(100));
    // order 1 is still first in the queue
    EXPECT_EQ(6u, book.market_buy(9, 6));
    EXPECT_FALSE(book.has(1));
    EXPECT_TRUE(book.has(2));
    book.check_invariants();
}

TEST(Reduce, full_reduce_cancels) {
    LimitOrderBook book;
    book.limit_buy(1, 10, 100);
    book.reduce(1, 10);
    EXPECT_FALSE(book.has(1));
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(0u, book.best_buy());
    book.check_invariants();
}

TEST(Reduce, rejects_invalid_quantities) {
    LimitOrderBook book;
    book.limit_buy(1, 10, 100);
    expect_error(ErrorCode::InvalidQuantity, [&] { book.reduce(1, 0); });
    expect_error(ErrorCode::InvalidQuantity, [&] { book.reduce(1, 11); });
    expect_error(ErrorCode::UnknownOrderId, [&] { book.reduce(2, 1); });
    EXPECT_EQ(10u, book.get(1).quantity);
    book.check_invariants();
}

// ---------------------------------------------------------------------------
// MARK: Filled quantities
// ---------------------------------------------------------------------------

TEST(Filled, resting_limit_returns_zero) {
    LimitOrderBook book;
    EXPECT_EQ(0u, book.limit_buy(1, 10, 100));
    EXPECT_EQ(0u, book.limit_sell(2, 10, 101));
}

TEST(Filled, crossing_limit_returns_filled_and_rests_remainder) {
    LimitOrderBook book;
    book.limit_sell(1, 30, 100);
    book.limit_sell(2, 30, 101);
    EXPECT_EQ(60u, book.limit_buy(3, 100, 101));
    EXPECT_TRUE(book.has(3));
    EXPECT_EQ(40u, book.get(3).quantity);
    EXPECT_EQ(101u, book.best_buy());
    EXPECT_EQ(0u, book.best_sell());
    book.check_invariants();
}

TEST(Filled, limit_does_not_trade_through_its_price) {
    LimitOrderBook book;
    book.limit_sell(1, 30, 100);
    book.limit_sell(2, 30, 105);
    EXPECT_EQ(30u, book.limit_buy(3, 100, 102));
    EXPECT_EQ(105u, book.best_sell());
    EXPECT_EQ(102u, book.best_buy());
    EXPECT_EQ(70u, book.get(3).quantity);
    book.check_invariants();
}

TEST(Filled, fully_filled_limit_does_not_rest) {
    LimitOrderBook book;
    book.limit_buy(1, 50, 100);
    EXPECT_EQ(20u, book.limit_sell(2, 20, 99));
    EXPECT_FALSE(book.has(2));
    EXPECT_EQ(30u, book.get(1).quantity);
    book.check_invariants();
}

TEST(Filled, market_is_immediate_or_cancel) {
    LimitOrderBook book;
    book.limit_sell(1, 30, 100);
    EXPECT_EQ(30u, book.market_buy(2, 100));
    EXPECT_TRUE(book.empty());
    EXPECT_FALSE(book.has(2));
    EXPECT_EQ(0u, book.market_sell(3, 5));  // nothing to trade against
    book.check_invariants();
}

// ---------------------------------------------------------------------------
// MARK: Prices
// ---------------------------------------------------------------------------

TEST(Prices, mid_price_rules) {
    LimitOrderBook book;
    EXPECT_EQ(0u, book.price());
    book.limit_buy(1, 1, 100);
    EXPECT_EQ(100u, book.price());
    book.limit_sell(2, 1, 103);
    EXPECT_EQ(101u, book.price());  // rounds down
    book.cancel(1);
    EXPECT_EQ(103u, book.price());
}

TEST(Prices, mid_price_does_not_overflow) {
    LimitOrderBook book;
    const Price max = std::numeric_limits<Price>::max();
    book.limit_sell(1, 1, max);
    book.limit_buy(2, 1, max - 2);
    EXPECT_EQ(max - 1, book.price());
}

TEST(Prices, last_best_prices_track_changes) {
    LimitOrderBook book;
    book.limit_buy(1, 10, 100);
    book.limit_buy(2, 10, 101);
    EXPECT_EQ(101u, book.last_best_buy());
    book.cancel(2);
    EXPECT_EQ(100u, book.last_best_buy());
    book.cancel(1);
    EXPECT_EQ(100u, book.last_best_buy());  // remembers the last best
    EXPECT_EQ(0u, book.best_buy());
}

// ---------------------------------------------------------------------------
// MARK: Depth
// ---------------------------------------------------------------------------

TEST(Depth, levels_are_best_first_and_aggregated) {
    LimitOrderBook book;
    book.limit_buy(1, 10, 99);
    book.limit_buy(2, 5, 100);
    book.limit_buy(3, 7, 100);
    book.limit_buy(4, 1, 98);
    book.limit_sell(5, 3, 102);
    book.limit_sell(6, 4, 101);

    auto bids = book.depth(Side::Buy, 10);
    ASSERT_EQ(3u, bids.size());
    EXPECT_EQ(100u, bids[0].price);
    EXPECT_EQ(12u, bids[0].volume);
    EXPECT_EQ(2u, bids[0].count);
    EXPECT_EQ(99u, bids[1].price);
    EXPECT_EQ(98u, bids[2].price);

    auto asks = book.depth(Side::Sell, 1);
    ASSERT_EQ(1u, asks.size());
    EXPECT_EQ(101u, asks[0].price);
    EXPECT_EQ(4u, asks[0].volume);

    EXPECT_TRUE(book.depth(Side::Sell, 0).empty());
    EXPECT_TRUE(LimitOrderBook().depth(Side::Buy, 5).empty());
}

// ---------------------------------------------------------------------------
// MARK: Ownership
// ---------------------------------------------------------------------------

TEST(Ownership, book_is_movable_but_not_copyable) {
    static_assert(!std::is_copy_constructible<LimitOrderBook>::value, "book must not be copyable");
    static_assert(std::is_move_constructible<LimitOrderBook>::value, "book must be movable");
    LimitOrderBook source;
    source.limit_buy(1, 10, 100);
    source.limit_sell(2, 10, 105);
    LimitOrderBook moved(std::move(source));
    EXPECT_EQ(2u, moved.count());
    EXPECT_EQ(100u, moved.best_buy());
    moved.check_invariants();
    EXPECT_EQ(10u, moved.limit_sell(3, 10, 100));
    moved.check_invariants();

    LimitOrderBook assigned;
    assigned.limit_buy(9, 1, 1);
    assigned = std::move(moved);
    EXPECT_FALSE(assigned.has(9));
    EXPECT_TRUE(assigned.has(2));
    assigned.check_invariants();
}

TEST(Ownership, clear_empties_everything) {
    LimitOrderBook book;
    for (UID id = 1; id <= 100; ++id)
        book.limit(id % 2 ? Side::Buy : Side::Sell, id, 10, id % 2 ? 100 - id % 50 : 200 + id % 50);
    book.clear();
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(0u, book.volume());
    EXPECT_EQ(0u, book.best_buy());
    EXPECT_EQ(0u, book.best_sell());
    book.check_invariants();
    EXPECT_NO_THROW(book.limit_buy(1, 10, 100));
}

TEST(Compatibility, legacy_namespace_alias) {
    LOB::LimitOrderBook book;
    book.limit(LOB::Side::Buy, 1, 10, 100);
    EXPECT_EQ(100u, book.best(LOB::Side::Buy));
}

TEST(Version, matches_macros) {
    EXPECT_STREQ(ORDERLY_CHAOS_VERSION_STRING, version());
}

// ---------------------------------------------------------------------------
// MARK: Scale
// ---------------------------------------------------------------------------

TEST(Scale, monotonic_prices_stay_fast) {
    // Monotonic prices degrade an unbalanced tree into a linked list
    // (quadratic time). The balanced tree keeps every operation O(log n).
    LimitOrderBook book;
    const UID levels = 200000;
    for (UID id = 1; id <= levels; ++id) book.limit_buy(id, 1, id);
    EXPECT_EQ(levels, book.best_buy());
    for (UID id = levels; id >= 1; --id) book.cancel(id);
    EXPECT_TRUE(book.empty());
    for (UID id = 1; id <= levels; ++id) book.limit_sell(id, 1, levels - id + 1);
    EXPECT_EQ(levels, static_cast<UID>(book.market_buy(0, static_cast<Quantity>(levels))));
    EXPECT_TRUE(book.empty());
}
