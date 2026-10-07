// Orderly Chaos: random order-flow simulation with throughput statistics.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara
//
// Simulates a stream of limit, market, and cancel orders around a drifting
// fair price, then reports book statistics and processing throughput.
//
// Build and run:  bazel run -c opt //examples/cpp:market_simulation -- 1000000

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>
#include <orderly_chaos/orderly_chaos.hpp>

using namespace orderly_chaos;

int main(int argc, char** argv) {
    const long events = argc > 1 ? std::atol(argv[1]) : 200000;
    if (events <= 0) {
        std::cerr << "usage: market_simulation [events > 0]\n";
        return 1;
    }

    LimitOrderBook book;
    uint64_t trades = 0;
    uint64_t traded_volume = 0;
    book.set_trade_handler([&](const Trade& trade) {
        ++trades;
        traded_volume += trade.quantity;
    });

    std::mt19937_64 rng(42);
    std::normal_distribution<double> offset(0.0, 8.0);
    std::uniform_int_distribution<int> action(0, 99);
    std::uniform_int_distribution<Quantity> quantity(1, 500);
    double fair = 10'000;
    UID next_id = 1;
    std::vector<UID> live;

    const auto start = std::chrono::steady_clock::now();
    for (long i = 0; i < events; ++i) {
        fair += offset(rng) * 0.05;  // random walk
        const Side side = (rng() & 1) ? Side::Buy : Side::Sell;
        const int roll = action(rng);
        if (roll < 60) {  // passive-ish limit order around the fair price
            const double skew = side == Side::Buy ? -2.0 : 2.0;
            const auto price = static_cast<Price>(std::max(1.0, fair + skew + offset(rng)));
            const UID id = next_id++;
            book.limit(side, id, quantity(rng), price);
            if (book.has(id)) live.push_back(id);
        } else if (roll < 70) {  // market order
            book.market(side, next_id++, quantity(rng));
        } else if (!live.empty()) {  // cancel a random (possibly filled) order
            const auto index = std::uniform_int_distribution<size_t>(0, live.size() - 1)(rng);
            const UID id = live[index];
            live[index] = live.back();
            live.pop_back();
            if (book.has(id)) book.cancel(id);
        }
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    book.check_invariants();
    std::cout << "events          " << events << '\n'
              << "trades          " << trades << " (" << traded_volume << " units)\n"
              << "resting orders  " << book.count() << " (" << book.depth(Side::Buy, 1000000).size()
              << " bid levels, " << book.depth(Side::Sell, 1000000).size() << " ask levels)\n"
              << "best bid / ask  " << book.best_buy() << " / " << book.best_sell() << '\n'
              << "elapsed         " << elapsed << " s\n"
              << "throughput      " << static_cast<long>(events / elapsed) << " events/s\n";
    return 0;
}
