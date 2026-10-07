// Orderly Chaos: tests for LimitTree (one side of the book).
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#include <gtest/gtest.h>
#include "orderly_chaos/limit_tree.hpp"

using namespace LOB;

TEST(LimitTree, should_initialize_LimitTree__a_buy_limit_tree_is_constructed__the_initial_parameters_are_correct) {
    LimitTree<Side::Buy> tree;
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(tree.limits.size() == 0);
    EXPECT_TRUE(tree.best == nullptr);
}

TEST(LimitTree, should_initialize_LimitTree__a_sell_limit_tree_is_constructed__the_initial_parameters_are_correct) {
    LimitTree<Side::Sell> tree;
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(tree.limits.size() == 0);
    EXPECT_TRUE(tree.best == nullptr);
}

TEST(LimitTree, add_a_single_order_to_LimitTree__a_LimitTree_and_a_single_buy_order__the_order_is_added) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Buy>();
    Order node = {1, Side::Buy, quantity, price};
    tree.limit(&node);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(price - 1));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(0, tree.volume_at(price + 1));
    EXPECT_EQ(0, tree.count_at(price - 1));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(price + 1));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node, tree.best->order_head);
    EXPECT_EQ(&node, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, add_a_single_order_to_LimitTree__a_LimitTree_and_a_single_sell_order__the_order_is_added) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Sell>();
    Order node = {1, Side::Sell, quantity, price};
    tree.limit(&node);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(price - 1));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(0, tree.volume_at(price + 1));
    EXPECT_EQ(0, tree.count_at(price - 1));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(price + 1));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node, tree.best->order_head);
    EXPECT_EQ(&node, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, add_two_orders_to_LimitTree_best_first__a_LimitTree_and_2_buy_orders__the_orders_are_added) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, priceHigher};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    EXPECT_EQ(2, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(priceHigher, tree.best->key);
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node1, tree.best->order_tail);
    EXPECT_EQ(priceHigher, tree.last_best_price);
}

TEST(LimitTree, add_two_orders_to_LimitTree_best_first__a_LimitTree_and_2_sell_orders__the_orders_are_added) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, priceHigher};
    tree.limit(&node2);
    EXPECT_EQ(2, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node1, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, add_two_orders_to_LimitTree_best_last__a_LimitTree_and_2_buy_orders__the_orders_are_added) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, priceHigher};
    tree.limit(&node2);
    EXPECT_EQ(2, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(priceHigher, tree.best->key);
    EXPECT_EQ(&node2, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(priceHigher, tree.last_best_price);
}

TEST(LimitTree, add_two_orders_to_LimitTree_best_last__a_LimitTree_and_2_sell_orders__the_orders_are_added) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, priceHigher};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    EXPECT_EQ(2, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node2, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, add_two_orders_to_LimitTree_same_price__a_LimitTree_and_2_orders_with_the_same_price__the_orders_are_added) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(2 * quantity, tree.volume_at(price));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(2, tree.count_at(price));
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_a_single_order_from_LimitTree__a_LimitTree_with_a_single_buy_order__the_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Buy>();
    Order node = {1, Side::Buy, quantity, price};
    tree.limit(&node);
    tree.cancel(&node);
    EXPECT_EQ(0, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_a_single_order_from_LimitTree__a_LimitTree_and_with_a_single_sell_order__the_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Sell>();
    Order node = {1, Side::Sell, quantity, price};
    tree.limit(&node);
    tree.cancel(&node);
    EXPECT_EQ(0, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_a_single_order_from_LimitTree__a_LimitTree_and_with_2_single_sell_orders_of_the_same_price__the_first_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Sell, quantity, price};
    tree.limit(&node2);
    tree.cancel(&node1);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(&node2, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_a_single_order_from_LimitTree__a_LimitTree_and_with_2_single_sell_orders_of_the_same_price__the_second_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Sell, quantity, price};
    tree.limit(&node2);
    tree.cancel(&node2);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node1, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_first__a_LimitTree_with_2_buy_orders__the_best_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, priceHigher};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    tree.cancel(&node1);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node2, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_first__a_LimitTree_with_2_buy_orders__the_arbitrary_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, priceHigher};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    tree.cancel(&node2);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(priceHigher, tree.best->key);
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node1, tree.best->order_tail);
    EXPECT_EQ(priceHigher, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_first__a_LimitTree_with_2_sell_orders__the_best_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, priceHigher};
    tree.limit(&node2);
    tree.cancel(&node1);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(priceHigher, tree.best->key);
    EXPECT_EQ(&node2, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(priceHigher, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_first__a_LimitTree_with_2_sell_orders__the_arbitrary_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, priceHigher};
    tree.limit(&node2);
    tree.cancel(&node2);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node1, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_last__a_LimitTree_with_2_buy_orders__the_best_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, priceHigher};
    tree.limit(&node2);
    tree.cancel(&node2);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node1, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_last__a_LimitTree_with_2_buy_orders__the_arbitrary_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, priceHigher};
    tree.limit(&node2);
    tree.cancel(&node1);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(priceHigher, tree.best->key);
    EXPECT_EQ(&node2, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(priceHigher, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_last__a_LimitTree_with_2_sell_orders__the_best_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, priceHigher};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    tree.cancel(&node2);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(quantity, tree.volume_at(priceHigher));
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_EQ(1, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(priceHigher, tree.best->key);
    EXPECT_EQ(&node1, tree.best->order_head);
    EXPECT_EQ(&node1, tree.best->order_tail);
    EXPECT_EQ(priceHigher, tree.last_best_price);
}

TEST(LimitTree, cancel_an_order_from_LimitTree_best_last__a_LimitTree_with_2_sell_orders__the_arbitrary_order_is_canceled) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, priceHigher};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    tree.cancel(&node1);
    EXPECT_EQ(1, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(priceHigher));
    EXPECT_EQ(quantity, tree.volume_at(price));
    EXPECT_EQ(1, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(priceHigher));
    EXPECT_FALSE(tree.levels.empty());
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(&node2, tree.best->order_head);
    EXPECT_EQ(&node2, tree.best->order_tail);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, a_market_order_is_submitted_with_no_order_in_the_tree__An_empty_limit_tree__a_buy_market_order_is_submitted) {
    LimitTree<Side::Buy> tree;
    Quantity quantity = 100;
    Order market = {1, Side::Sell, quantity, 0};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(tree.limits.size() == 0);
    EXPECT_TRUE(tree.best == nullptr);
    EXPECT_EQ(tree.count, 0);
    EXPECT_EQ(tree.volume, 0);
    EXPECT_EQ(tree.last_best_price, 0);
}

TEST(LimitTree, a_market_order_is_submitted_to_tree_with_a_perfect_match__An_order_book_with_a_limit_order_and_a_matched_market_order__a_sell_market_order_is_matched_to_a_buy_limit_order) {
    Quantity quantity = 100;
    Price price = 50;
    LimitTree<Side::Buy> tree;
    Order limit = {1, Side::Buy, quantity, price};
    tree.limit(&limit);
    Order market = {2, Side::Sell, quantity, 0};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, a_market_order_is_submitted_that_is_partially_filled__An_order_book_with_a_limit_order_and_a_smaller_market_order__a_buy_market_order_is_submitted) {
    Quantity quantity_limit = 100;
    Quantity quantity_market = 20;
    Price price = 50;
    LimitTree<Side::Buy> tree;
    Order limit = {1, Side::Buy, quantity_limit, price};
    tree.limit(&limit);
    Order market = {2, Side::Sell, quantity_market, 0};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(price, tree.best->key);
    EXPECT_EQ(quantity_limit - quantity_market, tree.volume_at(price));
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, a_market_order_is_submitted_that_spans_several_limit_orders__An_order_book_with_two_limits_and_a_market_order_that_requir__a_buy_market_order_is_submitted) {
    Quantity quantity_limit1 = 40;
    Quantity quantity_limit2 = 20;
    Quantity quantity_market = 50;
    Price price = 100;
    LimitTree<Side::Buy> tree;
    Order limit1 = {1, Side::Buy, quantity_limit1, price};
    tree.limit(&limit1);
    Order limit2 = {2, Side::Buy, quantity_limit2, price};
    tree.limit(&limit2);
    Order market = {3, Side::Sell, quantity_market, 0};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(100, tree.best->key);
    EXPECT_EQ(quantity_limit1 + quantity_limit2 - quantity_market, tree.volume_at(price));
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, a_market_order_is_submitted_that_spans_several_limit_orders___An_order_book_with_two_limits_and_a_market_order_that_requir__a_buy_market_order_is_submitted) {
    Quantity quantity_limit1 = 20;
    Quantity quantity_limit2 = 20;
    Quantity quantity_market = 50;
    Price price = 100;
    LimitTree<Side::Buy> tree;
    Order limit1 = {1, Side::Buy, quantity_limit1, price};
    tree.limit(&limit1);
    Order limit2 = {2, Side::Buy, quantity_limit2, price};
    tree.limit(&limit2);
    Order market = {3, Side::Sell, quantity_market, 0};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, a_market_order_is_submitted_that_spans_several_limit_prices___An_order_book_with_two_limits_and_a_market_order_that_requir__a_buy_market_order_is_submitted) {
    Quantity quantity = 20;
    Price price1 = 101;
    Price price2 = 102;
    LimitTree<Side::Buy> tree;
    Order limit1 = {1, Side::Buy, quantity, price1};
    tree.limit(&limit1);
    Order limit2 = {2, Side::Buy, quantity, price2};
    tree.limit(&limit2);
    Order market = {3, Side::Sell, 2 * quantity, 0};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(0, tree.volume_at(price1));
    EXPECT_EQ(0, tree.volume_at(price2));
    EXPECT_EQ(price1, tree.last_best_price);
}

TEST(LimitTree, a_market_order_is_submitted_with_a_limit_price__A_book_with_two_limits_and_a_market_order_with_limit_price__a_buy_market_order_is_submitted) {
    Quantity quantity_limit1 = 20;
    Quantity quantity_limit2 = 20;
    Quantity quantity_market = 40;
    Price price1 = 100;
    Price price2 = 101;
    LimitTree<Side::Buy> tree;
    Order limit1 = {1, Side::Buy, quantity_limit1, price1};
    tree.limit(&limit1);
    Order limit2 = {2, Side::Buy, quantity_limit2, price2};
    tree.limit(&limit2);
    Order market = {3, Side::Sell, quantity_market, price2};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(100, tree.best->key);
    EXPECT_EQ(20, tree.volume_at(price1));
    EXPECT_EQ(0, tree.volume_at(price2));
    EXPECT_EQ(price1, tree.last_best_price);
    EXPECT_EQ(quantity_market - quantity_limit2, market.quantity);
    EXPECT_EQ(price2, market.price);
}

TEST(LimitTree, a_market_order_is_submitted_with_a_limit_price_that_spans__A_book_with_two_limits_and_a_market_order_with_limit_price__a_buy_market_order_is_submitted) {
    Quantity quantity_limit1 = 20;
    Quantity quantity_limit2 = 20;
    Quantity quantity_limit3 = 20;
    Quantity quantity_market = 60;
    Price price1 = 100;
    Price price2 = 101;
    Price price3 = 102;
    LimitTree<Side::Buy> tree;
    Order limit1 = {1, Side::Buy, quantity_limit1, price1};
    tree.limit(&limit1);
    Order limit2 = {2, Side::Buy, quantity_limit2, price2};
    tree.limit(&limit2);
    Order limit3 = {3, Side::Buy, quantity_limit3, price3};
    tree.limit(&limit3);
    Order market = {4, Side::Sell, quantity_market, price2};
    tree.market(&market, [](const Order&, Price, Quantity, bool) { });
    EXPECT_NE(nullptr, tree.best);
    EXPECT_EQ(100, tree.best->key);
    EXPECT_EQ(20, tree.volume_at(price1));
    EXPECT_EQ(0, tree.volume_at(price2));
    EXPECT_EQ(0, tree.volume_at(price3));
    EXPECT_EQ(price1, tree.last_best_price);
    EXPECT_TRUE(quantity_market - (quantity_limit2 + quantity_limit3) == market.quantity);
    EXPECT_EQ(price2, market.price);
}

TEST(LimitTree, clear_a_single_limit_from_LimitTree__a_LimitTree_with_a_single_buy_order__the_tree_is_cleared) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Buy>();
    Order node = {1, Side::Buy, quantity, price};
    tree.limit(&node);
    tree.clear();
    EXPECT_EQ(0, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, clear_a_single_limit_from_LimitTree__a_LimitTree_and_with_a_single_sell_order__the_tree_is_cleared) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Sell>();
    Order node = {1, Side::Sell, quantity, price};
    tree.limit(&node);
    tree.clear();
    EXPECT_EQ(0, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, clear_a_single_limit_from_LimitTree__a_LimitTree_and_with_2_single_sell_orders_of_the_same_price__the_tree_is_cleared) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Sell, quantity, price};
    tree.limit(&node2);
    tree.clear();
    EXPECT_EQ(0, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(price, tree.last_best_price);
}

TEST(LimitTree, clear_multiple_limits_from_the_tree__a_LimitTree_with_2_buy_orders__the_tree_is_cleared) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Buy>();
    Order node1 = {1, Side::Buy, quantity, priceHigher};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, price};
    tree.limit(&node2);
    tree.clear();
    EXPECT_EQ(0, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(priceHigher));
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(priceHigher));
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(priceHigher, tree.last_best_price);
}

TEST(LimitTree, clear_multiple_limits_from_the_tree__a_LimitTree_with_2_sell_orders__the_tree_is_cleared) {
    Quantity quantity = 0x4545;
    Price price = 0xAABBCCDD00112233;
    auto priceHigher = price + 1;
    auto tree = LimitTree<Side::Sell>();
    Order node1 = {1, Side::Sell, quantity, price};
    tree.limit(&node1);
    Order node2 = {2, Side::Buy, quantity, priceHigher};
    tree.limit(&node2);
    tree.clear();
    EXPECT_EQ(0, tree.limits.size());
    EXPECT_EQ(0, tree.volume_at(priceHigher));
    EXPECT_EQ(0, tree.volume_at(price));
    EXPECT_EQ(0, tree.count_at(price));
    EXPECT_EQ(0, tree.count_at(priceHigher));
    EXPECT_TRUE(tree.levels.empty());
    EXPECT_TRUE(nullptr == tree.best);
    EXPECT_EQ(price, tree.last_best_price);
}

