// Orderly Chaos: tests for the C API (orderly_chaos.h).
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <vector>
#include "orderly_chaos/orderly_chaos.h"
#include "orderly_chaos/version.hpp"

namespace {

/// RAII owner for an oc_book in tests.
struct BookDeleter { void operator()(oc_book* book) const { oc_book_free(book); } };
typedef std::unique_ptr<oc_book, BookDeleter> Book;

Book make_book() {
    Book book(oc_book_new());
    EXPECT_NE(nullptr, book.get());
    return book;
}

void collect(const oc_trade* trade, void* user_data) {
    static_cast<std::vector<oc_trade>*>(user_data)->push_back(*trade);
}

}  // namespace

TEST(CApi, version_and_status_strings) {
    EXPECT_STREQ(ORDERLY_CHAOS_VERSION_STRING, oc_version());
    EXPECT_STREQ("ok", oc_status_string(OC_OK));
    EXPECT_STREQ("duplicate order id", oc_status_string(OC_ERR_DUPLICATE_ORDER_ID));
    EXPECT_STREQ("unknown order id", oc_status_string(OC_ERR_UNKNOWN_ORDER_ID));
    EXPECT_STREQ("invalid quantity", oc_status_string(OC_ERR_INVALID_QUANTITY));
    EXPECT_STREQ("invalid price", oc_status_string(OC_ERR_INVALID_PRICE));
    EXPECT_STREQ("invalid side", oc_status_string(OC_ERR_INVALID_SIDE));
    EXPECT_STREQ("null argument", oc_status_string(OC_ERR_NULL_ARGUMENT));
    EXPECT_STREQ("out of memory", oc_status_string(OC_ERR_OUT_OF_MEMORY));
    EXPECT_STREQ("internal error", oc_status_string(OC_ERR_INTERNAL));
    EXPECT_STREQ("unknown status", oc_status_string(12345));
}

TEST(CApi, struct_layouts_are_stable) {
    // these sizes are relied upon by FFI bindings (e.g. Python ctypes)
    EXPECT_EQ(24u, sizeof(oc_order));
    EXPECT_EQ(40u, sizeof(oc_trade));
    EXPECT_EQ(24u, sizeof(oc_level));
}

TEST(CApi, null_book_is_handled) {
    uint32_t filled = 99;
    oc_order order;
    oc_level level;
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_clear(nullptr));
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_set_trade_callback(nullptr, nullptr, nullptr));
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_limit(nullptr, OC_SIDE_BUY, 1, 1, 1, &filled));
    EXPECT_EQ(0u, filled);
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_market(nullptr, OC_SIDE_BUY, 1, 1, nullptr));
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_cancel(nullptr, 1));
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_reduce(nullptr, 1, 1));
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_get(nullptr, 1, &order));
    EXPECT_EQ(0, oc_book_has(nullptr, 1));
    EXPECT_EQ(0u, oc_book_best_price(nullptr, OC_SIDE_BUY));
    EXPECT_EQ(0u, oc_book_mid_price(nullptr));
    EXPECT_EQ(0u, oc_book_volume(nullptr));
    EXPECT_EQ(0u, oc_book_count(nullptr));
    EXPECT_EQ(0u, oc_book_depth(nullptr, OC_SIDE_BUY, &level, 1));
    oc_book_free(nullptr);  // no-op
}

TEST(CApi, invalid_side_is_rejected) {
    Book book = make_book();
    EXPECT_EQ(OC_ERR_INVALID_SIDE, oc_book_limit(book.get(), 2, 1, 10, 100, nullptr));
    EXPECT_EQ(OC_ERR_INVALID_SIDE, oc_book_market(book.get(), -1, 1, 10, nullptr));
    EXPECT_EQ(0u, oc_book_best_price(book.get(), 7));
    EXPECT_EQ(0u, oc_book_side_volume(book.get(), 7));
    EXPECT_EQ(0u, oc_book_count(book.get()));
}

TEST(CApi, validation_errors_map_to_status_codes) {
    Book book = make_book();
    EXPECT_EQ(OC_OK, oc_book_limit(book.get(), OC_SIDE_BUY, 1, 10, 100, nullptr));
    EXPECT_EQ(OC_ERR_DUPLICATE_ORDER_ID, oc_book_limit(book.get(), OC_SIDE_BUY, 1, 10, 100, nullptr));
    EXPECT_EQ(OC_ERR_INVALID_QUANTITY, oc_book_limit(book.get(), OC_SIDE_BUY, 2, 0, 100, nullptr));
    EXPECT_EQ(OC_ERR_INVALID_PRICE, oc_book_limit(book.get(), OC_SIDE_BUY, 2, 10, 0, nullptr));
    EXPECT_EQ(OC_ERR_INVALID_QUANTITY, oc_book_market(book.get(), OC_SIDE_SELL, 2, 0, nullptr));
    EXPECT_EQ(OC_ERR_UNKNOWN_ORDER_ID, oc_book_cancel(book.get(), 2));
    EXPECT_EQ(OC_ERR_UNKNOWN_ORDER_ID, oc_book_reduce(book.get(), 2, 1));
    EXPECT_EQ(OC_ERR_INVALID_QUANTITY, oc_book_reduce(book.get(), 1, 11));
    oc_order order;
    EXPECT_EQ(OC_ERR_UNKNOWN_ORDER_ID, oc_book_get(book.get(), 2, &order));
    EXPECT_EQ(OC_ERR_NULL_ARGUMENT, oc_book_get(book.get(), 1, nullptr));
    EXPECT_EQ(1u, oc_book_count(book.get()));
}

TEST(CApi, full_trading_flow) {
    Book book = make_book();
    std::vector<oc_trade> trades;
    ASSERT_EQ(OC_OK, oc_book_set_trade_callback(book.get(), collect, &trades));

    uint32_t filled = 99;
    ASSERT_EQ(OC_OK, oc_book_limit(book.get(), OC_SIDE_SELL, 1, 30, 101, &filled));
    EXPECT_EQ(0u, filled);
    ASSERT_EQ(OC_OK, oc_book_limit(book.get(), OC_SIDE_SELL, 2, 20, 102, &filled));
    ASSERT_EQ(OC_OK, oc_book_limit(book.get(), OC_SIDE_BUY, 3, 25, 99, &filled));
    ASSERT_EQ(OC_OK, oc_book_limit(book.get(), OC_SIDE_BUY, 4, 5, 99, &filled));

    EXPECT_EQ(101u, oc_book_best_price(book.get(), OC_SIDE_SELL));
    EXPECT_EQ(99u, oc_book_best_price(book.get(), OC_SIDE_BUY));
    EXPECT_EQ(100u, oc_book_mid_price(book.get()));
    EXPECT_EQ(30u, oc_book_best_volume(book.get(), OC_SIDE_BUY));
    EXPECT_EQ(80u, oc_book_volume(book.get()));
    EXPECT_EQ(50u, oc_book_side_volume(book.get(), OC_SIDE_SELL));
    EXPECT_EQ(30u, oc_book_volume_at(book.get(), 99));
    EXPECT_EQ(30u, oc_book_side_volume_at(book.get(), OC_SIDE_BUY, 99));
    EXPECT_EQ(0u, oc_book_side_volume_at(book.get(), OC_SIDE_SELL, 99));
    EXPECT_EQ(4u, oc_book_count(book.get()));
    EXPECT_EQ(2u, oc_book_side_count(book.get(), OC_SIDE_BUY));
    EXPECT_EQ(2u, oc_book_count_at(book.get(), 99));
    EXPECT_EQ(1, oc_book_has(book.get(), 3));

    oc_order order;
    ASSERT_EQ(OC_OK, oc_book_get(book.get(), 3, &order));
    EXPECT_EQ(3u, order.order_id);
    EXPECT_EQ(99u, order.price);
    EXPECT_EQ(25u, order.quantity);
    EXPECT_EQ(OC_SIDE_BUY, order.side);

    oc_level levels[4];
    std::memset(levels, 0, sizeof(levels));
    ASSERT_EQ(2u, oc_book_depth(book.get(), OC_SIDE_SELL, levels, 4));
    EXPECT_EQ(101u, levels[0].price);
    EXPECT_EQ(30u, levels[0].volume);
    EXPECT_EQ(1u, levels[0].count);
    EXPECT_EQ(102u, levels[1].price);
    EXPECT_EQ(1u, oc_book_depth(book.get(), OC_SIDE_SELL, levels, 1));

    // aggressive buy sweeps one level and part of the next
    ASSERT_EQ(OC_OK, oc_book_market(book.get(), OC_SIDE_BUY, 10, 40, &filled));
    EXPECT_EQ(40u, filled);
    ASSERT_EQ(2u, trades.size());
    EXPECT_EQ(10u, trades[0].taker_id);
    EXPECT_EQ(1u, trades[0].maker_id);
    EXPECT_EQ(101u, trades[0].price);
    EXPECT_EQ(30u, trades[0].quantity);
    EXPECT_EQ(0u, trades[0].maker_remaining);
    EXPECT_EQ(OC_SIDE_BUY, trades[0].taker_side);
    EXPECT_EQ(2u, trades[1].maker_id);
    EXPECT_EQ(10u, trades[1].quantity);
    EXPECT_EQ(10u, trades[1].maker_remaining);

    ASSERT_EQ(OC_OK, oc_book_reduce(book.get(), 3, 5));
    ASSERT_EQ(OC_OK, oc_book_cancel(book.get(), 4));
    EXPECT_EQ(20u, oc_book_side_volume(book.get(), OC_SIDE_BUY));

    // disabling the callback stops events
    ASSERT_EQ(OC_OK, oc_book_set_trade_callback(book.get(), nullptr, nullptr));
    ASSERT_EQ(OC_OK, oc_book_market(book.get(), OC_SIDE_SELL, 11, 5, &filled));
    EXPECT_EQ(5u, filled);
    EXPECT_EQ(2u, trades.size());

    ASSERT_EQ(OC_OK, oc_book_clear(book.get()));
    EXPECT_EQ(0u, oc_book_count(book.get()));
    EXPECT_EQ(0u, oc_book_mid_price(book.get()));
}
