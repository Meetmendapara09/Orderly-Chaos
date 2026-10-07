// Orderly Chaos: a price-time priority limit order book (matching engine).
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#ifndef ORDERLY_CHAOS_LIMIT_ORDER_BOOK_HPP_
#define ORDERLY_CHAOS_LIMIT_ORDER_BOOK_HPP_

#include "errors.hpp"
#include "limit_tree.hpp"
#include "types.hpp"
#include <cstddef>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace orderly_chaos {

/// @brief An aggregated price level, as returned by LimitOrderBook::depth().
struct PriceLevel {
    /// the price of the level
    Price price = 0;
    /// the total open quantity at the level
    Volume volume = 0;
    /// the number of orders at the level
    Count count = 0;
};

/// @brief A continuous double-auction limit order book with price-time
/// priority matching.
///
/// - **Limit orders** that cross the spread trade immediately against the
///   opposite side; any remainder rests in the book.
/// - **Market orders** trade against the opposite side until filled or the
///   side is empty; any remainder is discarded (immediate-or-cancel).
/// - Resting orders trade best price first, then oldest first (FIFO).
/// - Trades execute at the resting (maker) order's price.
///
/// Invalid requests throw OrderBookError and leave the book unchanged.
///
/// @note Not thread-safe. Use one book per thread, or synchronise access.
///
class LimitOrderBook {
 public:
    /// a callback receiving each trade as it happens
    typedef std::function<void(const Trade&)> TradeHandler;

    /// @brief Initialize an empty order book.
    LimitOrderBook() = default;
    LimitOrderBook(const LimitOrderBook&) = delete;
    LimitOrderBook& operator=(const LimitOrderBook&) = delete;
    LimitOrderBook(LimitOrderBook&&) = default;
    LimitOrderBook& operator=(LimitOrderBook&&) = default;

    // -----------------------------------------------------------------------
    // MARK: Configuration
    // -----------------------------------------------------------------------

    /// @brief Set (or clear, with nullptr) the handler that receives trades.
    ///
    /// The handler runs after each operation has fully updated the book, so
    /// it may safely query the book. It must not modify the book. If the
    /// handler throws, the remaining trades of that operation are still
    /// delivered and the first exception is re-thrown afterwards.
    ///
    /// @param handler the trade handler, or nullptr to disable trade events
    ///
    inline void set_trade_handler(TradeHandler handler) {
        trade_handler = std::move(handler);
    }

    // -----------------------------------------------------------------------
    // MARK: Order entry
    // -----------------------------------------------------------------------

    /// @brief Remove every order from the book.
    inline void clear() {
        sells.clear();
        buys.clear();
        orders.clear();
    }

    /// @brief Submit a limit order.
    ///
    /// @param side the side of the order
    /// @param order_id an ID not used by any resting order
    /// @param quantity the quantity, greater than zero
    /// @param price the limit price, greater than zero
    /// @returns the quantity filled immediately (the rest is now resting)
    /// @throws OrderBookError if the ID is in use, or quantity/price is zero
    ///
    inline Quantity limit(Side side, UID order_id, Quantity quantity, Price price) {
        return side == Side::Sell ?
            submit_limit(sells, buys, side, order_id, quantity, price) :
            submit_limit(buys, sells, side, order_id, quantity, price);
    }

    /// @brief Submit a sell limit order. See limit().
    inline Quantity limit_sell(UID order_id, Quantity quantity, Price price) {
        return limit(Side::Sell, order_id, quantity, price);
    }

    /// @brief Submit a buy limit order. See limit().
    inline Quantity limit_buy(UID order_id, Quantity quantity, Price price) {
        return limit(Side::Buy, order_id, quantity, price);
    }

    /// @brief Submit a market (immediate-or-cancel) order.
    ///
    /// Market orders never rest, so their ID is only used to label trades.
    ///
    /// @param side the side of the order
    /// @param order_id the ID reported as `taker_id` in trades
    /// @param quantity the quantity, greater than zero
    /// @returns the quantity filled; any unfilled remainder is discarded
    /// @throws OrderBookError if quantity is zero
    ///
    inline Quantity market(Side side, UID order_id, Quantity quantity) {
        if (quantity == 0)
            throw OrderBookError(ErrorCode::InvalidQuantity, "quantity must be greater than zero");
        Order order{order_id, side, quantity, 0};
        if (side == Side::Sell)
            match(buys, &order);
        else
            match(sells, &order);
        const Quantity filled = quantity - order.quantity;
        dispatch_trades();
        return filled;
    }

    /// @brief Submit a sell market order. See market().
    inline Quantity market_sell(UID order_id, Quantity quantity) {
        return market(Side::Sell, order_id, quantity);
    }

    /// @brief Submit a buy market order. See market().
    inline Quantity market_buy(UID order_id, Quantity quantity) {
        return market(Side::Buy, order_id, quantity);
    }

    /// @brief Cancel a resting order.
    ///
    /// @param order_id the ID of a resting order
    /// @throws OrderBookError if no resting order has this ID
    ///
    inline void cancel(UID order_id) {
        auto entry = find(order_id);
        Order* order = &entry->second;
        if (order->side == Side::Sell)
            sells.cancel(order);
        else
            buys.cancel(order);
        orders.erase(entry);
    }

    /// @brief Reduce a resting order's quantity, keeping its time priority.
    ///
    /// Reducing by the full open quantity cancels the order.
    ///
    /// @param order_id the ID of a resting order
    /// @param quantity the amount to remove, between 1 and the open quantity
    /// @throws OrderBookError if the ID is unknown or the quantity is invalid
    ///
    inline void reduce(UID order_id, Quantity quantity) {
        auto entry = find(order_id);
        Order* order = &entry->second;
        if (quantity == 0 || quantity > order->quantity) {
            throw OrderBookError(ErrorCode::InvalidQuantity,
                "cannot reduce order " + std::to_string(order_id) + " by " +
                std::to_string(quantity) + " (open quantity " +
                std::to_string(order->quantity) + ")");
        }
        if (quantity == order->quantity) {
            cancel(order_id);
        } else if (order->side == Side::Sell) {
            sells.reduce(order, quantity);
        } else {
            buys.reduce(order, quantity);
        }
    }

    // -----------------------------------------------------------------------
    // MARK: Order queries
    // -----------------------------------------------------------------------

    /// @brief Return true if an order with this ID is resting in the book.
    inline bool has(UID order_id) const { return orders.count(order_id) != 0; }

    /// @brief Return the resting order with this ID.
    ///
    /// The reference is invalidated by any operation that modifies the book.
    ///
    /// @throws OrderBookError if no resting order has this ID
    ///
    inline const Order& get(UID order_id) const {
        auto entry = orders.find(order_id);
        if (entry == orders.end()) throw unknown(order_id);
        return entry->second;
    }

    // -----------------------------------------------------------------------
    // MARK: Price queries (0 means "no price")
    // -----------------------------------------------------------------------

    /// @brief Return the lowest ask, or 0 if there are no sell orders.
    inline Price best_sell() const { return sells.best == nullptr ? 0 : sells.best->key; }

    /// @brief Return the highest bid, or 0 if there are no buy orders.
    inline Price best_buy() const { return buys.best == nullptr ? 0 : buys.best->key; }

    /// @brief Return the best price on a side, or 0 if that side is empty.
    inline Price best(Side side) const {
        return side == Side::Sell ? best_sell() : best_buy();
    }

    /// @brief Return the mid price: the midpoint of the best bid and ask, the
    /// only best price if one side is empty, or 0 if the book is empty.
    /// Rounds down.
    inline Price price() const {
        if (sells.best == nullptr && buys.best == nullptr) return 0;
        if (sells.best == nullptr) return buys.best->key;
        if (buys.best == nullptr) return sells.best->key;
        return midpoint(sells.best->key, buys.best->key);
    }

    /// @brief Return the best ask after the most recent change to the asks.
    inline Price last_best_sell() const { return sells.last_best_price; }

    /// @brief Return the best bid after the most recent change to the bids.
    inline Price last_best_buy() const { return buys.last_best_price; }

    /// @brief Return last_best_sell() or last_best_buy() for a side.
    inline Price last_best(Side side) const {
        return side == Side::Sell ? last_best_sell() : last_best_buy();
    }

    /// @brief Return the midpoint of the last best bid and ask.
    inline Price last_price() const {
        return midpoint(sells.last_best_price, buys.last_best_price);
    }

    // -----------------------------------------------------------------------
    // MARK: Volume and count queries
    // -----------------------------------------------------------------------

    /// @brief Return the open sell quantity at a price.
    inline Volume volume_sell(Price price) const { return sells.volume_at(price); }

    /// @brief Return the total open sell quantity.
    inline Volume volume_sell() const { return sells.volume; }

    /// @brief Return the open quantity at the best ask (0 if none).
    inline Volume volume_sell_best() const {
        return sells.best == nullptr ? 0 : sells.best->volume;
    }

    /// @brief Return the open buy quantity at a price.
    inline Volume volume_buy(Price price) const { return buys.volume_at(price); }

    /// @brief Return the total open buy quantity.
    inline Volume volume_buy() const { return buys.volume; }

    /// @brief Return the open quantity at the best bid (0 if none).
    inline Volume volume_buy_best() const {
        return buys.best == nullptr ? 0 : buys.best->volume;
    }

    /// @brief Return the open quantity at the best price of a side.
    inline Volume volume_best(Side side) const {
        return side == Side::Sell ? volume_sell_best() : volume_buy_best();
    }

    /// @brief Return the open quantity at a price (both sides).
    inline Volume volume(Price price) const {
        return buys.volume_at(price) + sells.volume_at(price);
    }

    /// @brief Return the total open quantity (both sides).
    inline Volume volume() const { return sells.volume + buys.volume; }

    /// @brief Return the number of orders resting at a price (both sides).
    inline Count count_at(Price price) const {
        return buys.count_at(price) + sells.count_at(price);
    }

    /// @brief Return the number of resting sell orders.
    inline Count count_sell() const { return sells.count; }

    /// @brief Return the number of resting buy orders.
    inline Count count_buy() const { return buys.count; }

    /// @brief Return the number of resting orders (both sides).
    inline Count count() const { return sells.count + buys.count; }

    /// @brief Return true if no orders are resting.
    inline bool empty() const { return orders.empty(); }

    /// @brief Return up to `max_levels` aggregated price levels of a side,
    /// best price first (a "level 2" market-depth snapshot).
    ///
    /// @param side the side to snapshot
    /// @param max_levels the maximum number of levels to return
    /// @returns the levels, best first
    ///
    inline std::vector<PriceLevel> depth(Side side, std::size_t max_levels) const {
        std::vector<PriceLevel> result;
        auto visit = [&](const Limit& level) {
            if (result.size() >= max_levels) return false;
            result.push_back(PriceLevel{level.key, level.volume, level.count});
            return true;
        };
        if (side == Side::Sell)
            sells.for_each_level(visit);
        else
            buys.for_each_level(visit);
        return result;
    }

    /// @brief Verify every internal invariant (O(N)); throws std::logic_error
    /// on the first violation. Intended for tests and debugging.
    void check_invariants() const {
        sells.check_invariants();
        buys.check_invariants();
        if (orders.size() != sells.count + buys.count)
            throw std::logic_error("LimitOrderBook invariant: order index size differs from resting orders");
        for (const auto& entry : orders) {
            const Order& order = entry.second;
            if (entry.first != order.uid || order.limit == nullptr)
                throw std::logic_error("LimitOrderBook invariant: order index entry is not resting");
        }
        if (sells.best != nullptr && buys.best != nullptr && sells.best->key <= buys.best->key)
            throw std::logic_error("LimitOrderBook invariant: book is crossed");
    }

 private:
    /// the resting sell orders (asks)
    LimitTree<Side::Sell> sells;
    /// the resting buy orders (bids)
    LimitTree<Side::Buy> buys;
    /// every resting order by ID (owns the Order objects)
    std::unordered_map<UID, Order> orders;
    /// the user's trade handler (may be empty)
    TradeHandler trade_handler;
    /// trades produced by the current operation, awaiting dispatch
    std::vector<Trade> pending_trades;

    /// @brief Return the midpoint of two prices without overflow, rounding down.
    static inline Price midpoint(Price a, Price b) {
        return a / 2 + b / 2 + (a % 2 + b % 2) / 2;
    }

    /// @brief Build the error for an unknown order ID.
    static inline OrderBookError unknown(UID order_id) {
        return OrderBookError(ErrorCode::UnknownOrderId, "no resting order with id " + std::to_string(order_id));
    }

    /// @brief Find a resting order or throw.
    inline std::unordered_map<UID, Order>::iterator find(UID order_id) {
        auto entry = orders.find(order_id);
        if (entry == orders.end()) throw unknown(order_id);
        return entry;
    }

    /// @brief Validate, match, and rest a limit order.
    template<typename Same, typename Opposite>
    inline Quantity submit_limit(
        Same& same, Opposite& opposite,
        Side side, UID order_id, Quantity quantity, Price price
    ) {
        if (quantity == 0)
            throw OrderBookError(ErrorCode::InvalidQuantity, "quantity must be greater than zero");
        if (price == 0)
            throw OrderBookError(ErrorCode::InvalidPrice, "limit price must be greater than zero");
        auto inserted = orders.emplace(std::piecewise_construct,
            std::forward_as_tuple(order_id),
            std::forward_as_tuple(order_id, side, quantity, price));
        if (!inserted.second)
            throw OrderBookError(ErrorCode::DuplicateOrderId, "order id " + std::to_string(order_id) + " is already resting");
        Order* order = &inserted.first->second;
        try {
            match(opposite, order);
            if (order->quantity > 0) same.limit(order);
        } catch (...) {  // allocation failure: drop the incoming order so the
                         // order index only ever contains resting orders
            orders.erase(inserted.first);
            pending_trades.clear();
            throw;
        }
        const Quantity filled = quantity - order->quantity;
        if (order->quantity == 0) orders.erase(inserted.first);  // fully filled
        dispatch_trades();
        return filled;
    }

    /// @brief Match an incoming order against the opposite side.
    template<typename Opposite>
    inline void match(Opposite& opposite, Order* taker) {
        const bool record = static_cast<bool>(trade_handler);
        opposite.market(taker, [&](const Order& maker, Price price, Quantity quantity, bool filled) {
            Trade trade;
            trade.taker_id = taker->uid;
            trade.maker_id = maker.uid;
            trade.taker_side = taker->side;
            trade.price = price;
            trade.quantity = quantity;
            trade.maker_remaining = filled ? 0 : maker.quantity;
            // remove the filled maker from the index first so the book stays
            // consistent even if recording the trade fails to allocate
            if (filled) orders.erase(maker.uid);
            if (record) pending_trades.push_back(trade);
        });
    }

    /// @brief Deliver pending trades to the handler (book is consistent now).
    inline void dispatch_trades() {
        if (pending_trades.empty()) return;
        std::exception_ptr first_error;
        for (const Trade& trade : pending_trades) {
            try {
                trade_handler(trade);
            } catch (...) {
                if (!first_error) first_error = std::current_exception();
            }
        }
        pending_trades.clear();
        if (first_error) std::rethrow_exception(first_error);
    }
};

}  // namespace orderly_chaos

#endif  // ORDERLY_CHAOS_LIMIT_ORDER_BOOK_HPP_
