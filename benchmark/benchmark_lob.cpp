// Benchmark the Limit Order Book (LOB).
// Copyright 2019 Christian Kauten
//
// Converted from Catch2 benchmarks to Google Test

#include <random>
#include <vector>
#include <chrono>
#include <iostream>
#include <gtest/gtest.h>
#include "limit_order_book.hpp"

using namespace LOB;

//
// MARK: new limits
//

inline void spam_limits(LimitOrderBook& book, int count) {
    for (int i = 0; i < count; i++) book.limit(Side::Buy, i, 50, i);
}

TEST(Benchmark, SpamNewLimits_1) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limits(book, 1);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 1 new limits: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamNewLimits_10) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limits(book, 10);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 10 new limits: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamNewLimits_100) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limits(book, 100);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 100 new limits: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamNewLimits_1000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limits(book, 1000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 1000 new limits: " << duration.count() << " us" << std::endl;
}

//
// MARK: new orders
//

inline void spam_orders(LimitOrderBook& book, int count, int variance = 5) {
    for (int i = 0; i < count; i++)
        book.limit(Side::Buy, i, 50, i % variance);
}

TEST(Benchmark, SpamOrders_1) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders(book, 1);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 1 orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamOrders_10) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders(book, 10);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 10 orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamOrders_100) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders(book, 100);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 100 orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamOrders_1000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders(book, 1000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send 1000 orders: " << duration.count() << " us" << std::endl;
}

//
// MARK: Random submission and cancellation
//

inline void spam_orders_random_cancels(
    LimitOrderBook& book,
    int count,
    int mean = 500,
    int variance = 30,
    int cancel_every = 5
) {
    auto generator = std::default_random_engine();
    auto price_distribution = std::normal_distribution<double>(mean, variance);
    book.limit(Side::Buy, 0, 50, price_distribution(generator));
    for (int i = 1; i < count; i++) {
        book.limit(Side::Buy, i, 50, price_distribution(generator));
        if (i % cancel_every == 0)
            book.cancel(i - cancel_every);
    }
}

TEST(Benchmark, SpamOrdersRandomCancels_10) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders_random_cancels(book, 10);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send and cancel 10 orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamOrdersRandomCancels_100) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders_random_cancels(book, 100);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send and cancel 100 orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamOrdersRandomCancels_1000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders_random_cancels(book, 1000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send and cancel 1000 orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamOrdersRandomCancels_10000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_orders_random_cancels(book, 10000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "send and cancel 10000 orders: " << duration.count() << " us" << std::endl;
}

//
// MARK: Random submission, cancellation, and market orders
//

inline void spam_limit_random_orders(
    LimitOrderBook& book,
    int count,
    int price_mean = 500,
    int price_variance = 20,
    int quantity_mean = 100,
    int quantity_variance = 10,
    int order_every = 100
) {
    auto generator = std::default_random_engine();
    auto price = std::normal_distribution<double>(price_mean, price_variance);
    auto quantity = std::normal_distribution<double>(quantity_mean, quantity_variance);
    for (int i = 1; i < count; i++) {
        auto price_ = static_cast<uint64_t>(price(generator));
        auto quantity_ = static_cast<uint32_t>(quantity(generator));
        book.limit(Side::Buy, i, 100, price_);
        if (i % order_every == 0)
            book.market(Side::Sell, i, quantity_);
    }
}

TEST(Benchmark, SpamLimitOrders_1000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limit_random_orders(book, 1000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "1000 limit orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamLimitOrders_10000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limit_random_orders(book, 10000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "10000 limit orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamLimitOrders_100000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limit_random_orders(book, 100000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "100000 limit orders: " << duration.count() << " us" << std::endl;
}

inline void spam_limit_many_market_orders(
    LimitOrderBook& book,
    int count,
    int price_mean = 500,
    int price_variance = 20,
    int quantity_mean = 50,
    int quantity_variance = 10
) {
    auto generator = std::default_random_engine();
    auto price = std::normal_distribution<double>(price_mean, price_variance);
    auto quantity = std::normal_distribution<double>(quantity_mean, quantity_variance);
    for (int i = 1; i < count; i++) {
        auto price_ = static_cast<uint64_t>(price(generator));
        auto quantity_ = static_cast<uint32_t>(quantity(generator));
        book.limit(Side::Buy, i, 100, price_);
        book.market(Side::Sell, i, quantity_);
    }
}

TEST(Benchmark, SpamLimitOrdersWithMarket_10) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limit_many_market_orders(book, 10);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "10 limit orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamLimitOrdersWithMarket_100) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limit_many_market_orders(book, 100);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "100 limit orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamLimitOrdersWithMarket_1000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limit_many_market_orders(book, 1000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "1000 limit orders: " << duration.count() << " us" << std::endl;
}

TEST(Benchmark, SpamLimitOrdersWithMarket_10000) {
    auto book = LimitOrderBook();
    auto start = std::chrono::high_resolution_clock::now();
    spam_limit_many_market_orders(book, 10000);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "10000 limit orders: " << duration.count() << " us" << std::endl;
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
