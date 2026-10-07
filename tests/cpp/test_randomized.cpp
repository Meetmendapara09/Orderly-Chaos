// Orderly Chaos: randomized differential test against a reference model.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara
//
// A deliberately simple (and slow) reference order book implements the same
// matching rules with std::map + std::deque. Random order flow is applied to
// both books and every observable result is compared after each operation:
// filled quantities, trades, errors, best prices, volumes, counts, and depth.

#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <random>
#include <vector>
#include "orderly_chaos/limit_order_book.hpp"

using namespace orderly_chaos;

namespace {

/// The reference model: a straightforward price-time priority book.
class ReferenceBook {
 public:
    struct Entry { UID id; Quantity quantity; };

    std::vector<Trade> trades;

    /// Submit a limit order; returns the filled quantity or throws.
    Quantity limit(Side side, UID id, Quantity quantity, Price price) {
        if (quantity == 0) throw OrderBookError(ErrorCode::InvalidQuantity, "");
        if (price == 0) throw OrderBookError(ErrorCode::InvalidPrice, "");
        if (find(id) != nullptr) throw OrderBookError(ErrorCode::DuplicateOrderId, "");
        Quantity remaining = match(side, id, quantity, price);
        if (remaining > 0) {
            levels(side)[price].push_back(Entry{id, remaining});
            sides[id] = side;
        }
        return quantity - remaining;
    }

    /// Submit a market order; returns the filled quantity or throws.
    Quantity market(Side side, UID id, Quantity quantity) {
        if (quantity == 0) throw OrderBookError(ErrorCode::InvalidQuantity, "");
        return quantity - match(side, id, quantity, 0);
    }

    void cancel(UID id) {
        Entry* entry = find(id);
        if (entry == nullptr) throw OrderBookError(ErrorCode::UnknownOrderId, "");
        remove(id);
    }

    void reduce(UID id, Quantity quantity) {
        Entry* entry = find(id);
        if (entry == nullptr) throw OrderBookError(ErrorCode::UnknownOrderId, "");
        if (quantity == 0 || quantity > entry->quantity) throw OrderBookError(ErrorCode::InvalidQuantity, "");
        entry->quantity -= quantity;
        if (entry->quantity == 0) remove(id);
    }

    Price best(Side side) const {
        const auto& book = side == Side::Buy ? bids : asks;
        if (book.empty()) return 0;
        return side == Side::Buy ? book.rbegin()->first : book.begin()->first;
    }

    Volume volume(Side side) const {
        Volume total = 0;
        for (const auto& level : side == Side::Buy ? bids : asks)
            for (const auto& entry : level.second) total += entry.quantity;
        return total;
    }

    Count count(Side side) const {
        Count total = 0;
        for (const auto& level : side == Side::Buy ? bids : asks) total += level.second.size();
        return total;
    }

    std::vector<PriceLevel> depth(Side side) const {
        std::vector<PriceLevel> result;
        auto add = [&](const std::pair<const Price, std::deque<Entry>>& level) {
            Volume volume = 0;
            for (const auto& entry : level.second) volume += entry.quantity;
            result.push_back(PriceLevel{level.first, volume, level.second.size()});
        };
        if (side == Side::Buy)
            for (auto it = bids.rbegin(); it != bids.rend(); ++it) add(*it);
        else
            for (const auto& level : asks) add(level);
        return result;
    }

    std::vector<UID> resting_ids() const {
        std::vector<UID> ids;
        for (const auto& item : sides) ids.push_back(item.first);
        return ids;
    }

 private:
    std::map<Price, std::deque<Entry>> bids, asks;
    std::map<UID, Side> sides;

    std::map<Price, std::deque<Entry>>& levels(Side side) { return side == Side::Buy ? bids : asks; }

    Entry* find(UID id) {
        auto side = sides.find(id);
        if (side == sides.end()) return nullptr;
        for (auto& level : levels(side->second))
            for (auto& entry : level.second)
                if (entry.id == id) return &entry;
        return nullptr;
    }

    void remove(UID id) {
        auto& book = levels(sides.at(id));
        for (auto level = book.begin(); level != book.end(); ++level) {
            auto& queue = level->second;
            auto it = std::find_if(queue.begin(), queue.end(), [&](const Entry& e) { return e.id == id; });
            if (it != queue.end()) {
                queue.erase(it);
                if (queue.empty()) book.erase(level);
                break;
            }
        }
        sides.erase(id);
    }

    Quantity match(Side side, UID taker, Quantity quantity, Price limit) {
        auto& book = levels(!side);
        while (quantity > 0 && !book.empty()) {
            auto level = side == Side::Buy ? book.begin() : std::prev(book.end());
            const Price price = level->first;
            if (limit != 0 && (side == Side::Buy ? price > limit : price < limit)) break;
            Entry& maker = level->second.front();
            const Quantity traded = std::min(quantity, maker.quantity);
            quantity -= traded;
            maker.quantity -= traded;
            Trade trade;
            trade.taker_id = taker;
            trade.maker_id = maker.id;
            trade.taker_side = side;
            trade.price = price;
            trade.quantity = traded;
            trade.maker_remaining = maker.quantity;
            trades.push_back(trade);
            if (maker.quantity == 0) {
                sides.erase(maker.id);
                level->second.pop_front();
                if (level->second.empty()) book.erase(level);
            }
        }
        return quantity;
    }
};

/// Run `body` and return the error code it throws, or 0 for success.
int outcome(const std::function<void()>& body) {
    try {
        body();
        return 0;
    } catch (const OrderBookError& error) {
        return static_cast<int>(error.code());
    }
}

void expect_trades_equal(const std::vector<Trade>& expected, const std::vector<Trade>& actual) {
    ASSERT_EQ(expected.size(), actual.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i].taker_id, actual[i].taker_id) << "trade " << i;
        EXPECT_EQ(expected[i].maker_id, actual[i].maker_id) << "trade " << i;
        EXPECT_EQ(expected[i].taker_side, actual[i].taker_side) << "trade " << i;
        EXPECT_EQ(expected[i].price, actual[i].price) << "trade " << i;
        EXPECT_EQ(expected[i].quantity, actual[i].quantity) << "trade " << i;
        EXPECT_EQ(expected[i].maker_remaining, actual[i].maker_remaining) << "trade " << i;
    }
}

void compare(const ReferenceBook& reference, const LimitOrderBook& book) {
    book.check_invariants();
    for (Side side : {Side::Buy, Side::Sell}) {
        ASSERT_EQ(reference.best(side), book.best(side));
        ASSERT_EQ(reference.volume(side), side == Side::Buy ? book.volume_buy() : book.volume_sell());
        ASSERT_EQ(reference.count(side), side == Side::Buy ? book.count_buy() : book.count_sell());
        const auto expected = reference.depth(side);
        const auto actual = book.depth(side, expected.size() + 1);
        ASSERT_EQ(expected.size(), actual.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            ASSERT_EQ(expected[i].price, actual[i].price);
            ASSERT_EQ(expected[i].volume, actual[i].volume);
            ASSERT_EQ(expected[i].count, actual[i].count);
        }
    }
}

/// Apply `steps` random operations to both books and compare after each.
void run_scenario(uint32_t seed, int steps, Price mid, Price spread) {
    std::mt19937_64 rng(seed);
    ReferenceBook reference;
    LimitOrderBook book;
    std::vector<Trade> trades;
    book.set_trade_handler([&](const Trade& trade) { trades.push_back(trade); });
    UID next_id = 1;

    auto uniform = [&](uint64_t low, uint64_t high) {
        return std::uniform_int_distribution<uint64_t>(low, high)(rng);
    };

    for (int step = 0; step < steps; ++step) {
        SCOPED_TRACE("seed " + std::to_string(seed) + " step " + std::to_string(step));
        reference.trades.clear();
        trades.clear();
        const Side side = uniform(0, 1) ? Side::Buy : Side::Sell;
        const auto resting = reference.resting_ids();
        const UID existing = resting.empty() ? 0 : resting[uniform(0, resting.size() - 1)];
        // occasionally submit invalid input to exercise rejections
        const Quantity quantity = static_cast<Quantity>(uniform(0, 40) == 0 ? 0 : uniform(1, 100));
        const Price price = uniform(0, 60) == 0 ? 0 : uniform(mid - spread, mid + spread);
        const uint64_t action = uniform(0, 99);

        int expected = 0, actual = 0;
        Quantity expected_filled = 0, actual_filled = 0;
        if (action < 55) {  // new limit order (sometimes a duplicate ID)
            const UID id = (existing != 0 && uniform(0, 30) == 0) ? existing : next_id++;
            expected = outcome([&] { expected_filled = reference.limit(side, id, quantity, price); });
            actual = outcome([&] { actual_filled = book.limit(side, id, quantity, price); });
        } else if (action < 70) {  // market order
            const UID id = next_id++;
            expected = outcome([&] { expected_filled = reference.market(side, id, quantity); });
            actual = outcome([&] { actual_filled = book.market(side, id, quantity); });
        } else if (action < 88) {  // cancel (sometimes unknown)
            const UID id = (existing == 0 || uniform(0, 10) == 0) ? next_id + 1000 : existing;
            expected = outcome([&] { reference.cancel(id); });
            actual = outcome([&] { book.cancel(id); });
        } else {  // reduce
            const UID id = existing == 0 ? next_id + 1000 : existing;
            expected = outcome([&] { reference.reduce(id, quantity % 60); });
            actual = outcome([&] { book.reduce(id, quantity % 60); });
        }
        ASSERT_EQ(expected, actual);
        ASSERT_EQ(expected_filled, actual_filled);
        expect_trades_equal(reference.trades, trades);
        compare(reference, book);
        if (::testing::Test::HasFatalFailure() || ::testing::Test::HasNonfatalFailure()) return;
    }
}

}  // namespace

TEST(Randomized, tight_spread_many_crosses) {
    for (uint32_t seed = 1; seed <= 20; ++seed) run_scenario(seed, 2000, 1000, 5);
}

TEST(Randomized, wide_spread_deep_book) {
    for (uint32_t seed = 101; seed <= 110; ++seed) run_scenario(seed, 3000, 100000, 500);
}

TEST(Randomized, extreme_prices) {
    const Price high = UINT64_MAX - 100;
    for (uint32_t seed = 201; seed <= 205; ++seed) run_scenario(seed, 1000, high, 50);
    for (uint32_t seed = 301; seed <= 305; ++seed) run_scenario(seed, 1000, 60, 59);
}
