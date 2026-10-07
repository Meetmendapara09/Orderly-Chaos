/*
 * Orderly Chaos: C API for the limit order book.
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Meet Mendapara
 *
 * A stable, C-compatible interface for use from C and from any language with
 * a C foreign-function interface (Python ctypes/cffi, Rust, Go, C#, ...).
 *
 * Conventions:
 *   - Every mutating function returns an oc_status; OC_OK means success.
 *     A failed call never modifies the book.
 *   - Query functions return 0 when the book pointer is NULL, the side is
 *     invalid, or there is no data (e.g. best price of an empty side).
 *   - Prices are integer ticks; 0 is reserved and means "no price".
 *   - A book is not thread-safe; synchronise access externally.
 *
 * Example:
 *
 *     oc_book* book = oc_book_new();
 *     uint32_t filled = 0;
 *     oc_book_limit(book, OC_SIDE_SELL, 1, 100, 10050, NULL);
 *     oc_book_limit(book, OC_SIDE_BUY,  2,  40, 10050, &filled);  // 40
 *     oc_book_free(book);
 */

#ifndef ORDERLY_CHAOS_ORDERLY_CHAOS_H_
#define ORDERLY_CHAOS_ORDERLY_CHAOS_H_

#include <stddef.h>
#include <stdint.h>

/* Symbol visibility for shared library builds. */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(ORDERLY_CHAOS_BUILDING_LIBRARY)
        #define OC_API __declspec(dllexport)
    #else
        #define OC_API
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define OC_API __attribute__((visibility("default")))
#else
    #define OC_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* Types                                                                    */
/* ------------------------------------------------------------------------ */

/** An opaque handle to a limit order book. */
typedef struct oc_book oc_book;

/** The side of an order: OC_SIDE_SELL or OC_SIDE_BUY. */
typedef int32_t oc_side;

/** Sell side (asks). */
#define OC_SIDE_SELL 0
/** Buy side (bids). */
#define OC_SIDE_BUY 1

/** The result of a mutating call. */
typedef int32_t oc_status;

/** Success. */
#define OC_OK 0
/** A limit order used an ID that is already resting. */
#define OC_ERR_DUPLICATE_ORDER_ID 1
/** No resting order has the given ID. */
#define OC_ERR_UNKNOWN_ORDER_ID 2
/** The quantity is zero, or exceeds the order's open quantity. */
#define OC_ERR_INVALID_QUANTITY 3
/** The limit price is zero. */
#define OC_ERR_INVALID_PRICE 4
/** The side is neither OC_SIDE_SELL nor OC_SIDE_BUY. */
#define OC_ERR_INVALID_SIDE 5
/** A required pointer argument is NULL. */
#define OC_ERR_NULL_ARGUMENT 6
/** Memory allocation failed. */
#define OC_ERR_OUT_OF_MEMORY 7
/** An unexpected internal error occurred. */
#define OC_ERR_INTERNAL 8

/** A resting order, as returned by oc_book_get(). */
typedef struct oc_order {
    uint64_t order_id;  /**< the order's ID */
    uint64_t price;     /**< the limit price */
    uint32_t quantity;  /**< the open quantity */
    oc_side side;       /**< OC_SIDE_SELL or OC_SIDE_BUY */
} oc_order;

/** An execution, delivered to the trade callback. */
typedef struct oc_trade {
    uint64_t taker_id;         /**< the incoming order's ID */
    uint64_t maker_id;         /**< the resting order's ID */
    uint64_t price;            /**< the execution price (maker's price) */
    uint32_t quantity;         /**< the executed quantity */
    uint32_t maker_remaining;  /**< the maker's open quantity afterwards */
    oc_side taker_side;        /**< the incoming order's side */
} oc_trade;

/** An aggregated price level, as returned by oc_book_depth(). */
typedef struct oc_level {
    uint64_t price;   /**< the price of the level */
    uint64_t volume;  /**< the total open quantity at the level */
    uint64_t count;   /**< the number of orders at the level */
} oc_level;

/**
 * A trade callback. Called once per execution, after the operation has
 * fully updated the book. The callback may query the book but must not
 * modify it. `trade` is only valid for the duration of the call.
 */
typedef void (*oc_trade_callback)(const oc_trade* trade, void* user_data);

/* ------------------------------------------------------------------------ */
/* Library                                                                  */
/* ------------------------------------------------------------------------ */

/** Return the library version, e.g. "0.2.0". */
OC_API const char* oc_version(void);

/** Return a static, human-readable description of a status code. */
OC_API const char* oc_status_string(oc_status status);

/* ------------------------------------------------------------------------ */
/* Lifecycle                                                                */
/* ------------------------------------------------------------------------ */

/** Create an empty book. Returns NULL if memory allocation fails. */
OC_API oc_book* oc_book_new(void);

/** Destroy a book. Passing NULL is a no-op. */
OC_API void oc_book_free(oc_book* book);

/** Remove every order from the book. */
OC_API oc_status oc_book_clear(oc_book* book);

/**
 * Set the trade callback (or disable it by passing NULL). `user_data` is
 * passed through to every call.
 */
OC_API oc_status oc_book_set_trade_callback(
    oc_book* book, oc_trade_callback callback, void* user_data);

/* ------------------------------------------------------------------------ */
/* Order entry                                                              */
/* ------------------------------------------------------------------------ */

/**
 * Submit a limit order. Any part that crosses the spread trades immediately;
 * the remainder rests. `filled` (optional, may be NULL) receives the quantity
 * filled immediately.
 */
OC_API oc_status oc_book_limit(
    oc_book* book, oc_side side, uint64_t order_id,
    uint32_t quantity, uint64_t price, uint32_t* filled);

/**
 * Submit a market (immediate-or-cancel) order. `filled` (optional) receives
 * the quantity filled; any remainder is discarded.
 */
OC_API oc_status oc_book_market(
    oc_book* book, oc_side side, uint64_t order_id,
    uint32_t quantity, uint32_t* filled);

/** Cancel a resting order. */
OC_API oc_status oc_book_cancel(oc_book* book, uint64_t order_id);

/**
 * Reduce a resting order's quantity while keeping its time priority.
 * Reducing by the full open quantity cancels the order.
 */
OC_API oc_status oc_book_reduce(
    oc_book* book, uint64_t order_id, uint32_t quantity);

/* ------------------------------------------------------------------------ */
/* Queries                                                                  */
/* ------------------------------------------------------------------------ */

/** Return 1 if an order with this ID is resting, otherwise 0. */
OC_API int oc_book_has(const oc_book* book, uint64_t order_id);

/** Copy a resting order into `out`. */
OC_API oc_status oc_book_get(
    const oc_book* book, uint64_t order_id, oc_order* out);

/** Return the best price on a side (highest bid / lowest ask), or 0. */
OC_API uint64_t oc_book_best_price(const oc_book* book, oc_side side);

/** Return the midpoint of the best bid and ask (rounded down), or 0. */
OC_API uint64_t oc_book_mid_price(const oc_book* book);

/** Return the open quantity at the best price of a side. */
OC_API uint64_t oc_book_best_volume(const oc_book* book, oc_side side);

/** Return the total open quantity in the book (both sides). */
OC_API uint64_t oc_book_volume(const oc_book* book);

/** Return the total open quantity on one side. */
OC_API uint64_t oc_book_side_volume(const oc_book* book, oc_side side);

/** Return the open quantity at a price (both sides). */
OC_API uint64_t oc_book_volume_at(const oc_book* book, uint64_t price);

/** Return the open quantity at a price on one side. */
OC_API uint64_t oc_book_side_volume_at(
    const oc_book* book, oc_side side, uint64_t price);

/** Return the number of resting orders (both sides). */
OC_API uint64_t oc_book_count(const oc_book* book);

/** Return the number of resting orders on one side. */
OC_API uint64_t oc_book_side_count(const oc_book* book, oc_side side);

/** Return the number of orders resting at a price (both sides). */
OC_API uint64_t oc_book_count_at(const oc_book* book, uint64_t price);

/**
 * Copy up to `capacity` aggregated price levels of a side into `levels`,
 * best price first. Returns the number of levels written.
 */
OC_API size_t oc_book_depth(
    const oc_book* book, oc_side side, oc_level* levels, size_t capacity);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ORDERLY_CHAOS_ORDERLY_CHAOS_H_ */
