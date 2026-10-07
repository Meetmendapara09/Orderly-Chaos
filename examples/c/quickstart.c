/*
 * Orderly Chaos: C API quick start.
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Meet Mendapara
 *
 * Build and run:  bazel run //examples/c:quickstart
 */

#include <inttypes.h>
#include <stdio.h>
#include <orderly_chaos/orderly_chaos.h>

/* Called once per execution. */
static void on_trade(const oc_trade* trade, void* user_data) {
    unsigned* count = (unsigned*)user_data;
    ++*count;
    printf("TRADE taker=%" PRIu64 " maker=%" PRIu64 " qty=%" PRIu32 " @ %" PRIu64 "\n",
           trade->taker_id, trade->maker_id, trade->quantity, trade->price);
}

/* Abort on unexpected errors. */
#define CHECK(call)                                                        \
    do {                                                                   \
        oc_status status_ = (call);                                        \
        if (status_ != OC_OK) {                                            \
            fprintf(stderr, "%s failed: %s\n", #call, oc_status_string(status_)); \
            oc_book_free(book);                                            \
            return 1;                                                      \
        }                                                                  \
    } while (0)

int main(void) {
    unsigned trades = 0;
    uint32_t filled = 0;
    oc_level levels[5];
    size_t depth, i;
    oc_status status;

    oc_book* book = oc_book_new();
    if (book == NULL) return 1;
    printf("Orderly Chaos %s\n", oc_version());

    CHECK(oc_book_set_trade_callback(book, on_trade, &trades));
    CHECK(oc_book_limit(book, OC_SIDE_SELL, 1, 100, 10050, NULL));
    CHECK(oc_book_limit(book, OC_SIDE_SELL, 2, 200, 10075, NULL));
    CHECK(oc_book_limit(book, OC_SIDE_BUY, 3, 150, 10000, NULL));
    printf("bid %" PRIu64 " / ask %" PRIu64 "\n",
           oc_book_best_price(book, OC_SIDE_BUY), oc_book_best_price(book, OC_SIDE_SELL));

    CHECK(oc_book_limit(book, OC_SIDE_BUY, 4, 250, 10050, &filled));
    printf("order 4 filled %" PRIu32 "\n", filled);

    CHECK(oc_book_market(book, OC_SIDE_BUY, 5, 500, &filled));
    printf("market buy filled %" PRIu32 " of 500\n", filled);

    depth = oc_book_depth(book, OC_SIDE_BUY, levels, 5);
    for (i = 0; i < depth; ++i)
        printf("  bid level %" PRIu64 " x %" PRIu64 "\n", levels[i].price, levels[i].volume);

    /* Errors are returned as status codes and never modify the book. */
    status = oc_book_cancel(book, 999);
    printf("cancel 999: %s\n", oc_status_string(status));

    printf("%u trades\n", trades);
    oc_book_free(book);
    return status == OC_ERR_UNKNOWN_ORDER_ID && trades == 2 ? 0 : 1;
}
