// Orderly Chaos: core value types: sides, orders, price levels, and trades.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#ifndef ORDERLY_CHAOS_TYPES_HPP_
#define ORDERLY_CHAOS_TYPES_HPP_

#include <cstdint>

namespace orderly_chaos {

/// @brief The side of the book an order belongs to.
///
/// The underlying values match `OC_SIDE_SELL` / `OC_SIDE_BUY` in the C API.
enum class Side : bool { Sell = false, Buy = true };

/// @brief Return the opposite side.
///
/// @param side the side to invert
/// @returns Side::Buy for Side::Sell and vice versa
///
inline constexpr Side operator!(Side side) {
    return static_cast<Side>(!static_cast<bool>(side));
}

/// A caller-assigned order identifier, unique among resting orders.
typedef uint64_t UID;
/// A number of units (shares, contracts, lots) in an order.
typedef uint32_t Quantity;
/// A price in integer ticks. Zero is reserved to mean "no limit" (market).
typedef uint64_t Price;
/// A sum of quantities across many orders.
typedef uint64_t Volume;
/// A number of orders.
typedef uint64_t Count;

// forward declare the `Limit` structure so `Order` can reference it
struct Limit;

/// @brief A single order resting in (or being matched against) the book.
///
/// Orders are intrusive nodes of their price level's FIFO queue, which makes
/// queue insertion and removal O(1) with no extra allocation.
struct Order {
    /// the previous (older) order at the same price level
    Order* prev = nullptr;
    /// the next (newer) order at the same price level
    Order* next = nullptr;
    /// the caller-assigned identifier of the order
    const UID uid = 0;
    /// the side of the book the order belongs to
    const Side side = Side::Sell;
    /// the quantity that is still open
    Quantity quantity = 0;
    /// the limit price of the order (0 for market orders)
    const Price price = 0;
    /// the price level the order rests at (nullptr when not resting)
    Limit* limit = nullptr;

    /// @brief Initialize an empty order.
    Order() { }

    /// @brief Initialize a new order.
    ///
    /// @param uid_ the caller-assigned identifier of the order
    /// @param side_ the side of the book the order belongs to
    /// @param quantity_ the quantity of the order
    /// @param price_ the limit price of the order (0 for market orders)
    ///
    Order(UID uid_, Side side_, Quantity quantity_, Price price_) :
        uid(uid_),
        side(side_),
        quantity(quantity_),
        price(price_) { }
};

/// @brief A price level: the FIFO queue of orders resting at one price.
struct Limit {
    /// the price of this level
    Price key = 0;
    /// the number of orders resting at this level
    Count count = 0;
    /// the total open quantity resting at this level
    Volume volume = 0;
    /// the oldest order at this level (first to match)
    Order* order_head = nullptr;
    /// the newest order at this level (last to match)
    Order* order_tail = nullptr;

    /// @brief Initialize an empty price level.
    Limit() { }

    /// @brief Initialize a price level containing a single order.
    ///
    /// @param order the first order resting at the level
    ///
    explicit Limit(Order* order) :
        key(order->price),
        count(1),
        volume(order->quantity),
        order_head(order),
        order_tail(order) { }

    /// @brief Append an order to the back of the queue (time priority).
    ///
    /// @param order the order to append; it must not be linked anywhere
    ///
    inline void append(Order* order) {
        order->prev = order_tail;
        order->next = nullptr;
        if (order_tail == nullptr)
            order_head = order;
        else
            order_tail->next = order;
        order_tail = order;
    }

    /// @brief Unlink an order from the queue.
    ///
    /// @param order an order currently linked into this level's queue
    ///
    inline void unlink(Order* order) {
        if (order->prev == nullptr)
            order_head = order->next;
        else
            order->prev->next = order->next;
        if (order->next == nullptr)
            order_tail = order->prev;
        else
            order->next->prev = order->prev;
        order->prev = nullptr;
        order->next = nullptr;
    }
};

/// @brief An execution between an incoming (taker) and a resting (maker) order.
///
/// Trades always execute at the maker's price.
struct Trade {
    /// the ID of the incoming order that removed liquidity
    UID taker_id = 0;
    /// the ID of the resting order that provided liquidity
    UID maker_id = 0;
    /// the side of the incoming order
    Side taker_side = Side::Sell;
    /// the execution price (the maker's limit price)
    Price price = 0;
    /// the executed quantity
    Quantity quantity = 0;
    /// the maker's open quantity after this trade (0 means fully filled)
    Quantity maker_remaining = 0;
};

}  // namespace orderly_chaos

/// Backwards-compatible alias for the namespace used before v0.2.0.
namespace LOB = orderly_chaos;

#endif  // ORDERLY_CHAOS_TYPES_HPP_
