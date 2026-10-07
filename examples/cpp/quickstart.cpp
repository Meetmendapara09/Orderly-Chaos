// Orderly Chaos: C++ quick start.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara
//
// Build and run:  bazel run //examples/cpp:quickstart

#include <iostream>
#include <orderly_chaos/orderly_chaos.hpp>

using orderly_chaos::LimitOrderBook;
using orderly_chaos::OrderBookError;
using orderly_chaos::Side;
using orderly_chaos::Trade;

int main() {
    LimitOrderBook book;

    // 1. Print every execution.
    book.set_trade_handler([](const Trade& trade) {
        std::cout << "TRADE taker=" << trade.taker_id << " maker=" << trade.maker_id
                  << " qty=" << trade.quantity << " @ " << trade.price << '\n';
    });

    // 2. Build a book. Prices are integer ticks (here: cents).
    book.limit(Side::Sell, 1, 100, 10'050);  // ask 100 @ 100.50
    book.limit(Side::Sell, 2, 200, 10'075);  // ask 200 @ 100.75
    book.limit(Side::Buy, 3, 150, 10'000);   // bid 150 @ 100.00
    std::cout << "bid " << book.best_buy() << " / ask " << book.best_sell()
              << "  mid " << book.price() << '\n';

    // 3. An aggressive limit order trades up to its price; the rest rests.
    const auto filled = book.limit(Side::Buy, 4, 250, 10'050);
    std::cout << "order 4 filled " << filled << ", resting " << book.get(4).quantity << '\n';

    // 4. A market order sweeps the book; any remainder is discarded.
    std::cout << "market buy filled " << book.market(Side::Buy, 5, 500) << " of 500\n";

    // 5. Depth snapshot (best first).
    for (const auto& level : book.depth(Side::Buy, 5))
        std::cout << "  bid level " << level.price << " x " << level.volume
                  << " (" << level.count << " orders)\n";

    // 6. Errors are reported with exceptions and never modify the book.
    try {
        book.cancel(999);
    } catch (const OrderBookError& error) {
        std::cout << "rejected: " << error.what() << '\n';
    }
    return 0;
}
