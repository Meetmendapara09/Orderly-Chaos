// Orderly Chaos: implementation of the C API (orderly_chaos.h).
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara
//
// Every entry point is a firewall: C++ exceptions are translated into
// oc_status codes and never cross the C boundary.

#ifndef ORDERLY_CHAOS_BUILDING_LIBRARY
#define ORDERLY_CHAOS_BUILDING_LIBRARY
#endif

#include "orderly_chaos/orderly_chaos.h"
#include "orderly_chaos/limit_order_book.hpp"
#include "orderly_chaos/version.hpp"
#include <new>

using orderly_chaos::ErrorCode;
using orderly_chaos::LimitOrderBook;
using orderly_chaos::OrderBookError;
using orderly_chaos::Side;
using orderly_chaos::Trade;

/// The object behind the opaque oc_book handle.
struct oc_book {
    /// the order book
    LimitOrderBook book;
    /// the user's trade callback (may be NULL)
    oc_trade_callback callback = nullptr;
    /// the user's opaque pointer passed to the callback
    void* user_data = nullptr;
};

namespace {

/// Return true if the side value is valid.
inline bool valid_side(oc_side side) {
    return side == OC_SIDE_SELL || side == OC_SIDE_BUY;
}

/// Convert a (validated) C side to the C++ side.
inline Side to_side(oc_side side) {
    return side == OC_SIDE_BUY ? Side::Buy : Side::Sell;
}

/// Convert a C++ side to the C side.
inline oc_side from_side(Side side) {
    return side == Side::Buy ? OC_SIDE_BUY : OC_SIDE_SELL;
}

/// Map an order book error code to a C status code.
inline oc_status to_status(ErrorCode code) {
    switch (code) {
        case ErrorCode::DuplicateOrderId: return OC_ERR_DUPLICATE_ORDER_ID;
        case ErrorCode::UnknownOrderId:   return OC_ERR_UNKNOWN_ORDER_ID;
        case ErrorCode::InvalidQuantity:  return OC_ERR_INVALID_QUANTITY;
        case ErrorCode::InvalidPrice:     return OC_ERR_INVALID_PRICE;
    }
    return OC_ERR_INTERNAL;
}

/// Run `body`, translating any exception into a status code.
template<typename Body>
inline oc_status guarded(Body&& body) {
    try {
        body();
        return OC_OK;
    } catch (const OrderBookError& error) {
        return to_status(error.code());
    } catch (const std::bad_alloc&) {
        return OC_ERR_OUT_OF_MEMORY;
    } catch (...) {
        return OC_ERR_INTERNAL;
    }
}

}  // namespace

extern "C" {

OC_API const char* oc_version(void) { return orderly_chaos::version(); }

OC_API const char* oc_status_string(oc_status status) {
    switch (status) {
        case OC_OK:                     return "ok";
        case OC_ERR_DUPLICATE_ORDER_ID: return "duplicate order id";
        case OC_ERR_UNKNOWN_ORDER_ID:   return "unknown order id";
        case OC_ERR_INVALID_QUANTITY:   return "invalid quantity";
        case OC_ERR_INVALID_PRICE:      return "invalid price";
        case OC_ERR_INVALID_SIDE:       return "invalid side";
        case OC_ERR_NULL_ARGUMENT:      return "null argument";
        case OC_ERR_OUT_OF_MEMORY:      return "out of memory";
        case OC_ERR_INTERNAL:           return "internal error";
        default:                        return "unknown status";
    }
}

// ---------------------------------------------------------------------------
// MARK: Lifecycle
// ---------------------------------------------------------------------------

OC_API oc_book* oc_book_new(void) {
    return new (std::nothrow) oc_book();
}

OC_API void oc_book_free(oc_book* book) { delete book; }

OC_API oc_status oc_book_clear(oc_book* book) {
    if (book == nullptr) return OC_ERR_NULL_ARGUMENT;
    return guarded([&] { book->book.clear(); });
}

OC_API oc_status oc_book_set_trade_callback(
    oc_book* book, oc_trade_callback callback, void* user_data
) {
    if (book == nullptr) return OC_ERR_NULL_ARGUMENT;
    return guarded([&] {
        book->callback = callback;
        book->user_data = user_data;
        if (callback == nullptr) {
            book->book.set_trade_handler(nullptr);
            return;
        }
        book->book.set_trade_handler([book](const Trade& trade) {
            oc_trade event;
            event.taker_id = trade.taker_id;
            event.maker_id = trade.maker_id;
            event.price = trade.price;
            event.quantity = trade.quantity;
            event.maker_remaining = trade.maker_remaining;
            event.taker_side = from_side(trade.taker_side);
            book->callback(&event, book->user_data);
        });
    });
}

// ---------------------------------------------------------------------------
// MARK: Order entry
// ---------------------------------------------------------------------------

OC_API oc_status oc_book_limit(
    oc_book* book, oc_side side, uint64_t order_id,
    uint32_t quantity, uint64_t price, uint32_t* filled
) {
    if (filled != nullptr) *filled = 0;
    if (book == nullptr) return OC_ERR_NULL_ARGUMENT;
    if (!valid_side(side)) return OC_ERR_INVALID_SIDE;
    return guarded([&] {
        const uint32_t result = book->book.limit(to_side(side), order_id, quantity, price);
        if (filled != nullptr) *filled = result;
    });
}

OC_API oc_status oc_book_market(
    oc_book* book, oc_side side, uint64_t order_id,
    uint32_t quantity, uint32_t* filled
) {
    if (filled != nullptr) *filled = 0;
    if (book == nullptr) return OC_ERR_NULL_ARGUMENT;
    if (!valid_side(side)) return OC_ERR_INVALID_SIDE;
    return guarded([&] {
        const uint32_t result = book->book.market(to_side(side), order_id, quantity);
        if (filled != nullptr) *filled = result;
    });
}

OC_API oc_status oc_book_cancel(oc_book* book, uint64_t order_id) {
    if (book == nullptr) return OC_ERR_NULL_ARGUMENT;
    return guarded([&] { book->book.cancel(order_id); });
}

OC_API oc_status oc_book_reduce(oc_book* book, uint64_t order_id, uint32_t quantity) {
    if (book == nullptr) return OC_ERR_NULL_ARGUMENT;
    return guarded([&] { book->book.reduce(order_id, quantity); });
}

// ---------------------------------------------------------------------------
// MARK: Queries
// ---------------------------------------------------------------------------

OC_API int oc_book_has(const oc_book* book, uint64_t order_id) {
    return book != nullptr && book->book.has(order_id) ? 1 : 0;
}

OC_API oc_status oc_book_get(const oc_book* book, uint64_t order_id, oc_order* out) {
    if (book == nullptr || out == nullptr) return OC_ERR_NULL_ARGUMENT;
    return guarded([&] {
        const auto& order = book->book.get(order_id);
        out->order_id = order.uid;
        out->price = order.price;
        out->quantity = order.quantity;
        out->side = from_side(order.side);
    });
}

OC_API uint64_t oc_book_best_price(const oc_book* book, oc_side side) {
    if (book == nullptr || !valid_side(side)) return 0;
    return book->book.best(to_side(side));
}

OC_API uint64_t oc_book_mid_price(const oc_book* book) {
    return book == nullptr ? 0 : book->book.price();
}

OC_API uint64_t oc_book_best_volume(const oc_book* book, oc_side side) {
    if (book == nullptr || !valid_side(side)) return 0;
    return book->book.volume_best(to_side(side));
}

OC_API uint64_t oc_book_volume(const oc_book* book) {
    return book == nullptr ? 0 : book->book.volume();
}

OC_API uint64_t oc_book_side_volume(const oc_book* book, oc_side side) {
    if (book == nullptr || !valid_side(side)) return 0;
    return side == OC_SIDE_BUY ? book->book.volume_buy() : book->book.volume_sell();
}

OC_API uint64_t oc_book_volume_at(const oc_book* book, uint64_t price) {
    return book == nullptr ? 0 : book->book.volume(price);
}

OC_API uint64_t oc_book_side_volume_at(const oc_book* book, oc_side side, uint64_t price) {
    if (book == nullptr || !valid_side(side)) return 0;
    return side == OC_SIDE_BUY ? book->book.volume_buy(price) : book->book.volume_sell(price);
}

OC_API uint64_t oc_book_count(const oc_book* book) {
    return book == nullptr ? 0 : book->book.count();
}

OC_API uint64_t oc_book_side_count(const oc_book* book, oc_side side) {
    if (book == nullptr || !valid_side(side)) return 0;
    return side == OC_SIDE_BUY ? book->book.count_buy() : book->book.count_sell();
}

OC_API uint64_t oc_book_count_at(const oc_book* book, uint64_t price) {
    return book == nullptr ? 0 : book->book.count_at(price);
}

OC_API size_t oc_book_depth(
    const oc_book* book, oc_side side, oc_level* levels, size_t capacity
) {
    if (book == nullptr || levels == nullptr || capacity == 0 || !valid_side(side)) return 0;
    size_t written = 0;
    try {
        for (const auto& level : book->book.depth(to_side(side), capacity)) {
            levels[written].price = level.price;
            levels[written].volume = level.volume;
            levels[written].count = level.count;
            ++written;
        }
    } catch (...) {  // allocation failure: report what was written
    }
    return written;
}

}  // extern "C"

#if defined(_WIN32) && defined(ORDERLY_CHAOS_PYTHON_EXTENSION)
// setuptools builds the shared library as a Python extension module and, on
// Windows, the linker requires the module init symbol to exist. The library
// is loaded with ctypes, so the function is never called.
extern "C" __declspec(dllexport) void* PyInit_lib_orderly_chaos(void) { return nullptr; }
#endif
