// Orderly Chaos: micro-benchmarks for the limit order book.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara
//
// Each scenario is repeated several times and the median nanoseconds per
// operation is reported. Always benchmark optimized builds:
//
//     bazel run -c opt //benchmarks:benchmark_lob
//     bazel run -c opt //benchmarks:benchmark_lob -- --quick

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <orderly_chaos/orderly_chaos.hpp>

using namespace orderly_chaos;

namespace {

/// Prevent the optimizer from discarding a value.
volatile uint64_t sink = 0;

struct Scenario {
    const char* name;
    const char* description;
    /// Prepare state (untimed), then return the timed body and its op count.
    std::function<std::pair<std::function<void()>, uint64_t>(LimitOrderBook&, uint64_t n)> setup;
};

std::vector<Price> random_prices(uint64_t n, Price low, Price high, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<Price> price(low, high);
    std::vector<Price> prices(n);
    for (auto& p : prices) p = price(rng);
    return prices;
}

const std::vector<Scenario> SCENARIOS = {
    {"limit_new_levels", "limit orders, each at a new random price", [](LimitOrderBook& book, uint64_t n) {
        auto prices = std::make_shared<std::vector<Price>>(random_prices(n * 4, 1, n * 1000, 1));
        std::sort(prices->begin(), prices->end());
        prices->erase(std::unique(prices->begin(), prices->end()), prices->end());
        std::shuffle(prices->begin(), prices->end(), std::mt19937_64(2));
        prices->resize(std::min<size_t>(prices->size(), n));
        return std::make_pair(std::function<void()>([&book, prices] {
            UID id = 1;
            for (Price p : *prices) book.limit(Side::Buy, id++, 10, p);
        }), static_cast<uint64_t>(prices->size()));
    }},
    {"limit_monotonic_levels", "limit orders at strictly increasing prices (worst case for unbalanced trees)",
     [](LimitOrderBook& book, uint64_t n) {
        return std::make_pair(std::function<void()>([&book, n] {
            for (UID id = 1; id <= n; ++id) book.limit(Side::Buy, id, 10, id);
        }), n);
    }},
    {"limit_existing_levels", "limit orders joining 10 existing price levels", [](LimitOrderBook& book, uint64_t n) {
        return std::make_pair(std::function<void()>([&book, n] {
            for (UID id = 1; id <= n; ++id) book.limit(Side::Sell, id, 10, 1000 + id % 10);
        }), n);
    }},
    {"cancel_random", "cancel resting orders in random order", [](LimitOrderBook& book, uint64_t n) {
        auto prices = random_prices(n, 1000, 2000, 3);
        for (UID id = 1; id <= n; ++id) book.limit(Side::Buy, id, 10, prices[id - 1]);
        auto ids = std::make_shared<std::vector<UID>>(n);
        std::iota(ids->begin(), ids->end(), 1);
        std::shuffle(ids->begin(), ids->end(), std::mt19937_64(4));
        return std::make_pair(std::function<void()>([&book, ids] {
            for (UID id : *ids) book.cancel(id);
        }), n);
    }},
    {"market_fill_one_each", "market orders, each filling one resting order", [](LimitOrderBook& book, uint64_t n) {
        auto prices = random_prices(n, 1000, 1100, 5);
        for (UID id = 1; id <= n; ++id) book.limit(Side::Sell, id, 10, prices[id - 1]);
        return std::make_pair(std::function<void()>([&book, n] {
            for (UID id = 1; id <= n; ++id) sink += book.market(Side::Buy, n + id, 10);
        }), n);
    }},
    {"mixed_flow", "60% limit / 10% market / 30% cancel around a mid price", [](LimitOrderBook& book, uint64_t n) {
        return std::make_pair(std::function<void()>([&book, n] {
            std::mt19937_64 rng(6);
            std::normal_distribution<double> offset(0.0, 10.0);
            std::vector<UID> live;
            live.reserve(n);
            UID id = 1;
            for (uint64_t i = 0; i < n; ++i) {
                const uint64_t roll = rng() % 10;
                const Side side = (rng() & 1) ? Side::Buy : Side::Sell;
                if (roll < 6) {
                    const double skew = side == Side::Buy ? -3 : 3;
                    book.limit(side, id, 1 + rng() % 100, static_cast<Price>(10000 + skew + offset(rng)));
                    live.push_back(id++);
                } else if (roll < 7) {
                    sink += book.market(side, id++, 1 + rng() % 100);
                } else if (!live.empty()) {
                    const size_t index = rng() % live.size();
                    const UID victim = live[index];
                    live[index] = live.back();
                    live.pop_back();
                    if (book.has(victim)) book.cancel(victim);
                }
            }
        }), n);
    }},
    {"top_of_book_queries", "best bid/ask + best volume + total volume", [](LimitOrderBook& book, uint64_t n) {
        auto prices = random_prices(10000, 1, 100000, 7);
        for (UID id = 1; id <= prices.size(); ++id)
            book.limit(id % 2 ? Side::Buy : Side::Sell, id, 10, id % 2 ? prices[id - 1] : prices[id - 1] + 100000);
        return std::make_pair(std::function<void()>([&book, n] {
            for (uint64_t i = 0; i < n; ++i)
                sink += book.best_buy() + book.best_sell() + book.volume_buy_best() + book.volume();
        }), n);
    }},
};

}  // namespace

int main(int argc, char** argv) {
    bool quick = false;
    const char* filter = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--quick") == 0) {
            quick = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::printf("usage: benchmark_lob [--quick] [scenario-substring]\n");
            return 0;
        } else {
            filter = argv[i];
        }
    }
    const uint64_t n = quick ? 20000 : 1000000;
    const int repetitions = quick ? 3 : 7;

    std::printf("Orderly Chaos %s benchmark (n = %llu, median of %d runs)\n\n",
                version(), static_cast<unsigned long long>(n), repetitions);
    std::printf("%-24s %12s %14s  %s\n", "scenario", "ns/op", "ops/s", "description");
    for (const auto& scenario : SCENARIOS) {
        if (filter != nullptr && std::string(scenario.name).find(filter) == std::string::npos) continue;
        std::vector<double> samples;
        for (int r = 0; r < repetitions; ++r) {
            LimitOrderBook book;
            auto prepared = scenario.setup(book, n);
            const auto start = std::chrono::steady_clock::now();
            prepared.first();
            const auto stop = std::chrono::steady_clock::now();
            const double ns = std::chrono::duration<double, std::nano>(stop - start).count();
            samples.push_back(ns / static_cast<double>(prepared.second));
        }
        std::sort(samples.begin(), samples.end());
        const double median = samples[samples.size() / 2];
        std::printf("%-24s %12.1f %14.0f  %s\n", scenario.name, median, 1e9 / median, scenario.description);
    }
    return 0;
}
