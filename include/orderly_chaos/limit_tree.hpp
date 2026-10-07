// Orderly Chaos: one side (bids or asks) of the limit order book.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#ifndef ORDERLY_CHAOS_LIMIT_TREE_HPP_
#define ORDERLY_CHAOS_LIMIT_TREE_HPP_

#include "types.hpp"
#include "tsl/robin_map.h"
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace orderly_chaos {

/// @brief Return true if an incoming order at `incoming` can trade with a
/// resting order at `resting` on side `side`.
///
/// @tparam side the side of the *resting* order
/// @param resting the resting (best) price
/// @param incoming the incoming order's limit price; 0 means market order
/// @returns true if the prices cross
///
template<Side side>
inline constexpr bool can_match(Price resting, Price incoming);

/// Resting bids match incoming sells priced at or below the bid.
template<>
inline constexpr bool can_match<Side::Buy>(Price resting, Price incoming) {
    return incoming == 0 || incoming <= resting;
}

/// Resting asks match incoming buys priced at or above the ask.
template<>
inline constexpr bool can_match<Side::Sell>(Price resting, Price incoming) {
    return incoming == 0 || incoming >= resting;
}

/// @brief One side of the book: price levels kept in price order, each
/// holding a FIFO queue of orders.
///
/// Data structures:
/// - `levels` is a `std::map` (red-black tree) ordered by price, giving
///   guaranteed O(log L) insertion/removal of price levels (L = levels).
/// - `limits` is a hash map from price to level, giving O(1) access to an
///   existing level when an order joins it.
/// - `best` caches the best level, so top-of-book reads are O(1).
///
/// The tree owns its `Limit` objects; it does not own `Order` objects.
///
/// @tparam side the side of the book this tree holds
///
template<Side side>
struct LimitTree {
    /// price levels ordered by price (ascending)
    std::map<Price, Limit*> levels;
    /// price levels by price for O(1) lookup (owns the Limit objects)
    tsl::robin_map<Price, Limit*> limits;
    /// the best price level (highest bid / lowest ask), or nullptr if empty
    Limit* best = nullptr;
    /// the best price after the most recent change (0 if never set)
    Price last_best_price = 0;
    /// the number of resting orders on this side
    Count count = 0;
    /// the total open quantity on this side
    Volume volume = 0;

    LimitTree() = default;
    LimitTree(const LimitTree&) = delete;
    LimitTree& operator=(const LimitTree&) = delete;

    /// @brief Take ownership of another tree's levels, leaving it empty.
    LimitTree(LimitTree&& other) :
        levels(std::move(other.levels)),
        limits(std::move(other.limits)),
        best(other.best),
        last_best_price(other.last_best_price),
        count(other.count),
        volume(other.volume) { other.forget(); }

    /// @brief Replace this tree's levels with another tree's levels.
    LimitTree& operator=(LimitTree&& other) {
        if (this != &other) {
            clear();
            levels = std::move(other.levels);
            limits = std::move(other.limits);
            best = other.best;
            last_best_price = other.last_best_price;
            count = other.count;
            volume = other.volume;
            other.forget();
        }
        return *this;
    }

    ~LimitTree() { clear(); }

    /// @brief Remove (and free) every price level.
    void clear() {
        for (auto& item : limits) delete item.second;
        limits.clear();
        levels.clear();
        best = nullptr;
        count = 0;
        volume = 0;
    }

    /// @brief Rest an order at its limit price (time priority: back of queue).
    ///
    /// @param order an order of this tree's side with a non-zero price
    ///
    void limit(Order* order) {
        auto existing = limits.find(order->price);
        if (existing == limits.end()) {  // first order at this price
            std::unique_ptr<Limit> level(new Limit(order));
            levels.emplace(order->price, level.get());
            try {
                limits.emplace(order->price, level.get());
            } catch (...) {
                levels.erase(order->price);
                throw;
            }
            order->prev = nullptr;
            order->next = nullptr;
            order->limit = level.release();
            if (best == nullptr || is_better(order->limit->key, best->key))
                best = order->limit;
        } else {  // join the queue of an existing level
            Limit* level = existing->second;
            level->append(order);
            ++level->count;
            level->volume += order->quantity;
            order->limit = level;
        }
        ++count;
        volume += order->quantity;
        last_best_price = best->key;
    }

    /// @brief Remove a resting order from the tree.
    ///
    /// @param order an order currently resting in this tree
    ///
    void cancel(Order* order) {
        Limit* level = order->limit;
        if (level->count == 1) {  // last order: remove the whole level
            levels.erase(level->key);
            limits.erase(level->key);
            if (best == level) best = find_best();
            delete level;
        } else {  // other orders remain at this level
            level->unlink(order);
            --level->count;
            level->volume -= order->quantity;
        }
        order->limit = nullptr;
        --count;
        volume -= order->quantity;
        if (best != nullptr) last_best_price = best->key;
    }

    /// @brief Reduce a resting order's open quantity without losing priority.
    ///
    /// @param order an order currently resting in this tree
    /// @param quantity the amount to remove, strictly less than the order's
    ///        open quantity (use cancel() to remove the remainder)
    ///
    void reduce(Order* order, Quantity quantity) {
        order->quantity -= quantity;
        order->limit->volume -= quantity;
        volume -= quantity;
    }

    /// @brief Match an incoming order against this side of the book.
    ///
    /// Orders are matched best price first and, within a price, oldest
    /// first. The incoming order's quantity is reduced by the amount filled.
    ///
    /// @tparam OnFill callable as on_fill(const Order& maker, Price price,
    ///         Quantity quantity, bool maker_filled). It is invoked once per
    ///         execution, after the tree has been updated. When maker_filled
    ///         is true the maker has been removed from the tree.
    /// @param taker the incoming order (price 0 for a market order)
    /// @param on_fill the execution callback
    ///
    template<typename OnFill>
    void market(Order* taker, OnFill&& on_fill) {
        while (best != nullptr &&
               taker->quantity > 0 &&
               can_match<side>(best->key, taker->price)) {
            Order* maker = best->order_head;
            const Price price = best->key;
            if (maker->quantity > taker->quantity) {  // partial maker fill
                const Quantity quantity = taker->quantity;
                reduce(maker, quantity);
                taker->quantity = 0;
                on_fill(*maker, price, quantity, false);
                return;
            }
            // the maker is completely filled
            const Quantity quantity = maker->quantity;
            taker->quantity -= quantity;
            cancel(maker);
            on_fill(*maker, price, quantity, true);
        }
    }

    /// @brief Return the open quantity resting at a price (0 if none).
    inline Volume volume_at(Price price) const {
        auto level = limits.find(price);
        return level == limits.end() ? 0 : level->second->volume;
    }

    /// @brief Return the number of orders resting at a price (0 if none).
    inline Count count_at(Price price) const {
        auto level = limits.find(price);
        return level == limits.end() ? 0 : level->second->count;
    }

    /// @brief Visit price levels from best to worst.
    ///
    /// @tparam Visitor callable as visitor(const Limit&) returning true to
    ///         continue or false to stop
    /// @param visitor the visitor
    ///
    template<typename Visitor>
    void for_each_level(Visitor&& visitor) const {
        if (side == Side::Buy) {
            for (auto it = levels.rbegin(); it != levels.rend(); ++it)
                if (!visitor(*it->second)) return;
        } else {
            for (auto it = levels.begin(); it != levels.end(); ++it)
                if (!visitor(*it->second)) return;
        }
    }

    /// @brief Verify every internal invariant, throwing std::logic_error on
    /// the first violation. Intended for tests and debugging; O(N).
    void check_invariants() const {
        if (levels.size() != limits.size())
            fail("levels and limits disagree on the number of price levels");
        Count total_count = 0;
        Volume total_volume = 0;
        for (const auto& item : levels) {
            const Limit* level = item.second;
            auto indexed = limits.find(item.first);
            if (indexed == limits.end() || indexed->second != level)
                fail("price level missing from the hash index");
            if (level->key != item.first)
                fail("price level key does not match its position");
            Count level_count = 0;
            Volume level_volume = 0;
            const Order* previous = nullptr;
            for (const Order* order = level->order_head; order != nullptr; order = order->next) {
                if (order->prev != previous) fail("broken queue back-link");
                if (order->limit != level) fail("order points to the wrong level");
                if (order->side != side) fail("order on the wrong side");
                if (order->price != level->key) fail("order price differs from level");
                if (order->quantity == 0) fail("resting order with zero quantity");
                ++level_count;
                level_volume += order->quantity;
                previous = order;
            }
            if (previous != level->order_tail) fail("queue tail is wrong");
            if (level_count == 0) fail("empty price level");
            if (level_count != level->count) fail("level count is wrong");
            if (level_volume != level->volume) fail("level volume is wrong");
            total_count += level_count;
            total_volume += level_volume;
        }
        if (total_count != count) fail("side count is wrong");
        if (total_volume != volume) fail("side volume is wrong");
        if (best != find_best()) fail("cached best level is wrong");
    }

 private:
    /// @brief Return true if price `a` is better than price `b` on this side.
    static constexpr bool is_better(Price a, Price b) {
        return side == Side::Buy ? a > b : a < b;
    }

    /// @brief Return the best level according to the ordered map.
    Limit* find_best() const {
        if (levels.empty()) return nullptr;
        return side == Side::Buy ? levels.rbegin()->second : levels.begin()->second;
    }

    /// @brief Reset scalar state after the containers were moved away.
    void forget() {
        levels.clear();
        limits.clear();
        best = nullptr;
        last_best_price = 0;
        count = 0;
        volume = 0;
    }

    /// @brief Throw an invariant violation.
    [[noreturn]] static void fail(const std::string& message) {
        throw std::logic_error(
            std::string("LimitTree<") + (side == Side::Buy ? "Buy" : "Sell") + "> invariant: " + message
        );
    }
};

}  // namespace orderly_chaos

#endif  // ORDERLY_CHAOS_LIMIT_TREE_HPP_
