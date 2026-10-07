// Orderly Chaos: tests for the core value types.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#include <gtest/gtest.h>
#include "orderly_chaos/types.hpp"

using namespace LOB;

// ---------------------------------------------------------------------------
// MARK: Side
// ---------------------------------------------------------------------------

TEST(Structures, invert_side_sell) {
    EXPECT_TRUE(Side::Buy == !Side::Sell);
}

TEST(Structures, invert_side_buy) {
    EXPECT_TRUE(Side::Sell == !Side::Buy);
}

// ---------------------------------------------------------------------------
// MARK: Order
// ---------------------------------------------------------------------------

TEST(Structures, initialize_default_order) {
    Order order;
    EXPECT_TRUE(order.next == nullptr);
    EXPECT_TRUE(order.prev == nullptr);
    EXPECT_EQ(order.uid, 0);
    EXPECT_EQ(order.side, Side::Sell);
    EXPECT_EQ(order.quantity, 0);
    EXPECT_EQ(order.price, 0);
    EXPECT_TRUE(order.limit == nullptr);
}

TEST(Structures, initialize_order_with_parameters) {
    UID uid = 5;
    auto side = Side::Buy;
    Quantity quantity = 100;
    Price price = 5746;
    Order order = {uid, side, quantity, price};
    EXPECT_TRUE(order.next == nullptr);
    EXPECT_TRUE(order.prev == nullptr);
    EXPECT_EQ(order.uid, uid);
    EXPECT_EQ(order.side, side);
    EXPECT_EQ(order.quantity, quantity);
    EXPECT_EQ(order.price, price);
    EXPECT_TRUE(order.limit == nullptr);
}

// ---------------------------------------------------------------------------
// MARK: Limit
// ---------------------------------------------------------------------------

TEST(Structures, initialize_default_limit) {
    Limit limit;
    EXPECT_EQ(limit.key, 0);
    EXPECT_EQ(limit.count, 0);
    EXPECT_EQ(limit.volume, 0);
    EXPECT_TRUE(limit.order_head == nullptr);
    EXPECT_TRUE(limit.order_tail == nullptr);
}

TEST(Structures, initialize_limit_with_parameters) {
    Quantity quantity = 100;
    Price price = 5;
    Order order = {5, Side::Buy, quantity, price};
    Limit limit{&order};
    EXPECT_EQ(limit.key, price);
    EXPECT_EQ(limit.count, 1);
    EXPECT_EQ(limit.volume, quantity);
    EXPECT_EQ(limit.order_head, &order);
    EXPECT_EQ(limit.order_tail, &order);
}

