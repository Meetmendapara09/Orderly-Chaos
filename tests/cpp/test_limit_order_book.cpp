// Orderly Chaos: behavioural tests for LimitOrderBook.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#include <gtest/gtest.h>
#include "orderly_chaos/limit_order_book.hpp"

using namespace LOB;

TEST(LimitOrderBook, initialize_LimitOrderBook__default_parameters__a_LimitOrderBook_is_initialized_with_no_parameters) {
    EXPECT_NO_THROW(new LimitOrderBook());
}

TEST(LimitOrderBook, send_single_order_to_LimitOrderBook__an_order_book_and_a_single_sell_order__the_order_is_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 57;
    Price price = 0xFEDCBA9876543210;
    book.limit(side, 1, quantity, price);
    EXPECT_EQ(quantity, book.volume(price));
    EXPECT_EQ(0, book.volume(price - 1));
    EXPECT_EQ(0, book.volume(price + 1));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(price, book.best_sell());
}

TEST(LimitOrderBook, send_single_order_to_LimitOrderBook__an_order_book_and_a_single_buy_order__the_order_is_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantity = 57;
    Price price = 0xFEDCBA9876543210;
    book.limit(side, 1, quantity, price);
    EXPECT_EQ(quantity, book.volume(price));
    EXPECT_EQ(0, book.volume(price - 1));
    EXPECT_EQ(0, book.volume(price + 1));
    EXPECT_EQ(price, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, send_homogeneous_orders_to_LimitOrderBook_at_same_price__an_order_book_and_a_series_of_sell_orders__the_orders_are_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 0xFEDCBA9876543210;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    EXPECT_EQ(quantityA + quantityB + quantityC, book.volume(price));
    EXPECT_EQ(0, book.volume(price - 1));
    EXPECT_EQ(0, book.volume(price + 1));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(price, book.best_sell());
}

TEST(LimitOrderBook, send_homogeneous_orders_to_LimitOrderBook_at_same_price__an_order_book_and_a_series_of_buy_orders__the_orders_are_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 0xFEDCBA9876543210;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    EXPECT_EQ(quantityA + quantityB + quantityC, book.volume(price));
    EXPECT_EQ(0, book.volume(price - 1));
    EXPECT_EQ(0, book.volume(price + 1));
    EXPECT_EQ(price, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, send_homogeneous_orders_to_LimitOrderBook_at_different_price__an_order_book_and_a_series_of_sell_orders__the_orders_are_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price priceA = 3253;
    Price priceB = 3240;
    Price priceC = 3245;
    book.limit(side, 1, quantityA, priceA);
    book.limit(side, 2, quantityB, priceB);
    book.limit(side, 3, quantityC, priceC);
    EXPECT_EQ(quantityA, book.volume(priceA));
    EXPECT_EQ(quantityB, book.volume(priceB));
    EXPECT_EQ(quantityC, book.volume(priceC));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(priceB, book.best_sell());
}

TEST(LimitOrderBook, send_homogeneous_orders_to_LimitOrderBook_at_different_price__an_order_book_and_a_series_of_buy_orders__the_orders_are_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price priceA = 3240;
    Price priceB = 3253;
    Price priceC = 3245;
    book.limit(side, 1, quantityA, priceA);
    book.limit(side, 2, quantityB, priceB);
    book.limit(side, 3, quantityC, priceC);
    EXPECT_EQ(quantityA, book.volume(priceA));
    EXPECT_EQ(quantityB, book.volume(priceB));
    EXPECT_EQ(quantityC, book.volume(priceC));
    EXPECT_EQ(priceB, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, a_limit_order_is_submitted_that_crosses__a_book_with_2_buy_limit_orders_and_a_sell_limit_order__the_sell_limit_order_is_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantity = 20;
    Quantity quantityMarket = 40;
    Price priceA = 100;
    Price priceB = 101;
    book.limit(side, 1, quantity, priceA);
    book.limit(side, 2, quantity, priceB);
    book.limit(!side, 3, quantityMarket, priceB);
    EXPECT_EQ(1, book.count_buy());
    EXPECT_EQ(quantity, book.volume_buy(priceA));
    EXPECT_EQ(0, book.volume_buy(priceB));
    EXPECT_EQ(priceA, book.best_buy());
    EXPECT_EQ(1, book.count_sell());
    EXPECT_EQ(quantityMarket - quantity, book.volume_sell(priceB));
    EXPECT_EQ(priceB, book.best_sell());
}

TEST(LimitOrderBook, a_limit_order_is_submitted_that_crosses__a_book_with_2_sell_limit_orders_and_a_buy_limit_order__the_sell_limit_order_is_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 20;
    Quantity quantityMarket = 40;
    Price priceA = 101;
    Price priceB = 100;
    book.limit(side, 1, quantity, priceA);
    book.limit(side, 2, quantity, priceB);
    book.limit(!side, 3, quantityMarket, priceB);
    EXPECT_EQ(1, book.count_sell());
    EXPECT_EQ(quantity, book.volume_sell(priceA));
    EXPECT_EQ(0, book.volume_sell(priceB));
    EXPECT_EQ(priceA, book.best_sell());
    EXPECT_EQ(1, book.count_buy());
    EXPECT_EQ(quantityMarket - quantity, book.volume_buy(priceB));
    EXPECT_EQ(priceB, book.best_buy());
}

TEST(LimitOrderBook, a_limit_order_is_submitted_that_crosses_and_fills__a_book_with_a_buy_limit_order__the_sell_limit_order_is_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantity = 20;
    Price price = 100;
    book.limit(side, 1, quantity, price);
    book.limit(!side, 2, quantity, price);
    EXPECT_EQ(0, book.count_buy());
    EXPECT_EQ(0, book.volume_buy(price));
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.count_sell());
    EXPECT_EQ(0, book.volume_sell(price));
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, a_limit_order_is_submitted_that_crosses_and_fills__a_book_with_a_sell_limit_order__the_sell_limit_order_is_sent) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 20;
    Price price = 100;
    book.limit(side, 1, quantity, price);
    book.limit(!side, 2, quantity, price);
    EXPECT_EQ(0, book.count_sell());
    EXPECT_EQ(0, book.volume_sell(price));
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.best_sell());
    EXPECT_EQ(0, book.count_buy());
    EXPECT_EQ(0, book.volume_buy(price));
    EXPECT_EQ(0, book.best_buy());
}

TEST(LimitOrderBook, an_order_book_and_an_ID_for_a_single_sell_order__the_order_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 50;
    Price price = 3253;
    book.limit(side, 1, quantity, price);
    book.cancel(1);
    EXPECT_EQ(0, book.volume(price));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_and_an_ID_for_a_single_sell_order__the_order_is_duplicated_added_and_canceled_again) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 50;
    Price price = 3253;
    book.limit(side, 1, quantity, price);
    book.cancel(1);
    book.limit(side, 2, quantity, price);
    book.cancel(2);
    EXPECT_EQ(0, book.volume(price));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_and_an_ID_for_a_single_buy_order__the_orders_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantity = 50;
    Price price = 3253;
    book.limit(side, 1, quantity, price);
    book.cancel(1);
    EXPECT_EQ(0, book.volume(price));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_and_an_ID_for_a_single_buy_order__the_order_is_duplicated_added_and_canceled_again) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantity = 50;
    Price price = 3253;
    book.limit(side, 1, quantity, price);
    book.cancel(1);
    book.limit(side, 2, quantity, price);
    book.cancel(2);
    EXPECT_EQ(0, book.volume(price));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_sell__the_left_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_sell__the_left_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    book.limit(side, 4, quantity, prices[0]);
    book.cancel(4);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_buy__the_left_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_buy__the_left_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    book.limit(side, 4, quantity, prices[0]);
    book.cancel(4);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_sell__the_right_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_sell__the_right_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    book.limit(side, 4, quantity, prices[2]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_buy__the_right_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_buy__the_right_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    book.limit(side, 4, quantity, prices[2]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_sell__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_sell__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    book.limit(side, 4, quantity, prices[1]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_buy__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_buy__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    book.limit(side, 4, quantity, prices[1]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_sell__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_sell__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    book.limit(side, 4, quantity, prices[0]);
    book.cancel(4);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_buy__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_buy__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(1);
    book.limit(side, 4, quantity, prices[0]);
    book.cancel(4);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_sell__the_middle_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_sell__the_middle_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    book.limit(side, 4, quantity, prices[1]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_buy__the_middle_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_buy__the_middle_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(2);
    book.limit(side, 4, quantity, prices[1]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_sell__the_leaf_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_sell__the_leaf_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    book.limit(side, 4, quantity, prices[2]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_buy__the_leaf_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_buy__the_leaf_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.cancel(3);
    book.limit(side, 4, quantity, prices[2]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_sell__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(1);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_sell__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(1);
    book.limit(side, 4, quantity, prices[2]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_buy__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(1);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_buy__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(1);
    book.limit(side, 4, quantity, prices[2]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(0, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_sell__the_middle_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(2);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_sell__the_middle_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(2);
    book.limit(side, 4, quantity, prices[1]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[0], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_buy__the_middle_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(2);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_buy__the_middle_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(2);
    book.limit(side, 4, quantity, prices[1]);
    book.cancel(4);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_sell__the_leaf_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(3);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_sell__the_leaf_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(3);
    book.limit(side, 4, quantity, prices[0]);
    book.cancel(4);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[1], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_buy__the_leaf_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(3);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_buy__the_leaf_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.cancel(3);
    book.limit(side, 4, quantity, prices[0]);
    book.cancel(4);
    EXPECT_EQ(0, book.volume(prices[0]));
    EXPECT_EQ(quantity, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_subtree_with_left_branch_shaped_lim__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[4] = {1, 2, 4, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.cancel(1);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_subtree_with_left_branch_shaped_lim__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[4] = {1, 2, 4, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.cancel(1);
    book.limit(side, 5, quantity, prices[1]);
    book.cancel(5);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_subtree_with_left_branch_and_termin__the_root_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[5] = {1, 2, 5, 3, 4};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.limit(side, 5, quantity, prices[4]);
    book.cancel(1);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(quantity, book.volume(prices[4]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_subtree_with_left_branch_and_termin__the_root_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[5] = {1, 2, 5, 3, 4};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.limit(side, 5, quantity, prices[4]);
    book.cancel(1);
    book.limit(side, 6, quantity, prices[1]);
    book.cancel(6);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(quantity, book.volume(prices[4]));
    EXPECT_EQ(prices[2], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_zigzag_shaped_limit_tree_buy__the_root_child_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[4] = {1, 4, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.cancel(2);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(prices[3], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_right_zigzag_shaped_limit_tree_buy__the_root_child_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[4] = {1, 4, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.cancel(2);
    book.limit(side, 5, quantity, prices[1]);
    book.cancel(5);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(prices[3], book.best_buy());
}

TEST(LimitOrderBook, an_order_book_with_left_zigzag_shaped_limit_tree_sell__the_root_child_order_is_canceled) {
    Quantity quantity = 50;
    Price prices[4] = {4, 1, 3, 2};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.cancel(2);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(prices[3], book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_zigzag_shaped_limit_tree_sell__the_root_child_order_is_duplicated_added_and_canceled_again) {
    Quantity quantity = 50;
    Price prices[4] = {4, 1, 3, 2};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.cancel(2);
    book.limit(side, 5, quantity, prices[1]);
    book.cancel(5);
    EXPECT_EQ(quantity, book.volume(prices[0]));
    EXPECT_EQ(0, book.volume(prices[1]));
    EXPECT_EQ(quantity, book.volume(prices[2]));
    EXPECT_EQ(quantity, book.volume(prices[3]));
    EXPECT_EQ(prices[3], book.best_sell());
}

TEST(LimitOrderBook, cancel_first_order_in_a_Limit_queue_of_orders__an_order_book_and_a_Limit_queue_of_sell_orders__the_first_order_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.cancel(1);
    EXPECT_EQ(quantityB + quantityC, book.volume(price));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(price, book.best_sell());
}

TEST(LimitOrderBook, cancel_first_order_in_a_Limit_queue_of_orders__an_order_book_and_a_Limit_queue_of_buy_orders__the_first_order_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.cancel(1);
    EXPECT_EQ(quantityB + quantityC, book.volume(price));
    EXPECT_EQ(price, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, cancel_middle_order_in_a_Limit_queue_of_orders__an_order_book_and_a_Limit_queue_of_sell_orders__the_middle_order_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.cancel(2);
    EXPECT_EQ(quantityA + quantityC, book.volume(price));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(price, book.best_sell());
}

TEST(LimitOrderBook, cancel_middle_order_in_a_Limit_queue_of_orders__an_order_book_and_a_Limit_queue_of_buy_orders__the_middle_order_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.cancel(2);
    EXPECT_EQ(quantityA + quantityC, book.volume(price));
    EXPECT_EQ(price, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, cancel_last_order_in_a_Limit_queue_of_orders__an_order_book_and_a_Limit_queue_of_sell_orders__the_last_order_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.cancel(3);
    EXPECT_EQ(quantityA + quantityB, book.volume(price));
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(price, book.best_sell());
}

TEST(LimitOrderBook, cancel_last_order_in_a_Limit_queue_of_orders__an_order_book_and_a_Limit_queue_of_buy_orders__the_last_order_is_canceled) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.cancel(3);
    EXPECT_EQ(quantityA + quantityB, book.volume(price));
    EXPECT_EQ(price, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, a_market_order_is_submitted_with_no_order_in_the_book__An_empty_limit_order_book__a_buy_market_order_is_submitted) {
    LimitOrderBook book;
    Quantity quantity = 100;
    book.market(Side::Sell, 1, quantity);
}

TEST(LimitOrderBook, a_market_order_is_submitted_with_a_perfect_match__An_an_order_book_with_a_limit_order_and_a_matched_market_ord__a_sell_market_order_is_matched_to_a_buy_limit_order) {
    Quantity quantity = 100;
    Price price = 50;
    auto book = LimitOrderBook();
    book.limit(Side::Buy, 1, quantity, price);
    book.market(Side::Sell, 2, quantity);
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.volume(price));
}

TEST(LimitOrderBook, a_market_order_is_submitted_that_is_partially_filled_by_a_li__An_order_book_with_a_limit_order_and_a_smaller_market_order__a_buy_market_order_is_submitted) {
    auto count_limit = 100;
    auto count_market = 20;
    Price price = 50;
    auto book = LimitOrderBook();
    book.limit(Side::Buy, 1, count_limit, price);
    book.market(Side::Sell, 2, count_market);
    EXPECT_EQ(price, book.best_buy());
    EXPECT_EQ(count_limit - count_market, book.volume(price));
}

TEST(LimitOrderBook, a_market_order_is_submitted_that_spans_several_limits__An_order_book_with_two_limits_and_a_market_order_that_requir__a_buy_market_order_is_submitted) {
    auto count_limit1 = 40;
    auto count_limit2 = 20;
    auto count_market = 50;
    Price price = 100;
    auto book = LimitOrderBook();
    book.limit(Side::Buy, 1, count_limit1, price);
    book.limit(Side::Buy, 2, count_limit2, price);
    book.market(Side::Sell, 3, count_market);
    EXPECT_EQ(100, book.best_buy());
    EXPECT_EQ(count_limit1 + count_limit2 - count_market, book.volume(price));
}

TEST(LimitOrderBook, a_market_order_is_submitted_that_spans_several_limits_and_de__An_order_book_with_two_limits_and_a_market_order_that_requir__a_buy_market_order_is_submitted) {
    auto count_limit1 = 20;
    auto count_limit2 = 20;
    auto count_market = 50;
    Price price = 100;
    auto book = LimitOrderBook();
    book.limit(Side::Buy, 1, count_limit1, price);
    book.limit(Side::Buy, 2, count_limit2, price);
    book.market(Side::Sell, 3, count_market);
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.volume(price));
}

TEST(LimitOrderBook, an_order_book_and_an_ID_for_a_single_sell_order__the_book_is_cleared) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 50;
    Price price = 3253;
    book.limit(side, 1, quantity, price);
    book.clear();
    EXPECT_EQ(0, book.volume(price));
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_and_an_ID_for_a_single_buy_order__the_book_is_cleared) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantity = 50;
    Price price = 3253;
    book.limit(side, 1, quantity, price);
    book.clear();
    EXPECT_EQ(0, book.volume(price));
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_sell__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_V_shaped_limit_tree_buy__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_sell__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_leg_shaped_limit_tree_buy__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_sell__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_leg_shaped_limit_tree_buy__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[3] = {1, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[2]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[0]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_subtree_with_left_branch_shaped_lim__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[4] = {1, 2, 4, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_subtree_with_left_branch_and_termin__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[5] = {1, 2, 5, 3, 4};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[1]);
    book.limit(side, 2, quantity, prices[0]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.limit(side, 5, quantity, prices[4]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_right_zigzag_shaped_limit_tree_buy__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[4] = {1, 4, 2, 3};
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, an_order_book_with_left_zigzag_shaped_limit_tree_sell__the_book_is_cleared) {
    Quantity quantity = 50;
    Price prices[4] = {4, 1, 3, 2};
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    book.limit(side, 1, quantity, prices[0]);
    book.limit(side, 2, quantity, prices[1]);
    book.limit(side, 3, quantity, prices[2]);
    book.limit(side, 4, quantity, prices[3]);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, clear_book_with_queue_of_orders_at_limit__an_order_book_and_a_Limit_queue_of_sell_orders__the_book_is_cleared) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, clear_book_with_queue_of_orders_at_limit__an_order_book_and_a_Limit_queue_of_buy_orders__the_book_is_cleared) {
    auto book = LimitOrderBook();
    auto side = Side::Buy;
    Quantity quantityA = 50;
    Quantity quantityB = 40;
    Quantity quantityC = 30;
    Price price = 3253;
    book.limit(side, 1, quantityA, price);
    book.limit(side, 2, quantityB, price);
    book.limit(side, 3, quantityC, price);
    book.clear();
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
}

TEST(LimitOrderBook, reduce_the_size_of_an_active_order__an_order_book_with_an_active_order__the_order_quantity_is_reduced) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 50;
    Price price = 3000;
    UID uid = 1;
    book.limit(side, uid, quantity, price);
    Quantity reduce = 20;
    book.reduce(uid, reduce);
    EXPECT_EQ(0, book.count_buy());
    EXPECT_EQ(1, book.count_sell());
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(quantity - reduce, book.volume_sell());
    EXPECT_EQ(quantity - reduce, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(price, book.best_sell());
    EXPECT_EQ(quantity - reduce, book.get(uid).quantity);
}

TEST(LimitOrderBook, reduce_the_size_of_an_active_order__an_order_book_with_an_active_order__the_order_quantity_is_reduced_entirely) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 50;
    Price price = 3000;
    UID uid = 1;
    book.limit(side, uid, quantity, price);
    Quantity reduce = 50;
    book.reduce(uid, reduce);
    EXPECT_EQ(0, book.count_buy());
    EXPECT_EQ(0, book.count_sell());
    EXPECT_EQ(0, book.volume_buy());
    EXPECT_EQ(0, book.volume_sell());
    EXPECT_EQ(0, book.volume());
    EXPECT_EQ(0, book.best_buy());
    EXPECT_EQ(0, book.best_sell());
    EXPECT_FALSE(book.has(uid));
}

TEST(LimitOrderBook, reduce_the_size_of_an_active_order__an_order_book_with_an_active_order__the_reduce_quantity_exceeds_what_is_available) {
    auto book = LimitOrderBook();
    auto side = Side::Sell;
    Quantity quantity = 50;
    Price price = 3000;
    UID uid = 1;
    book.limit(side, uid, quantity, price);
    Quantity reduce = 70;
    EXPECT_ANY_THROW(book.reduce(uid, reduce));
}

