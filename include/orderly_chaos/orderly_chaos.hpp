// Orderly Chaos: umbrella header for the C++ API.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara
//
// Usage:
//
//     #include <orderly_chaos/orderly_chaos.hpp>
//
//     orderly_chaos::LimitOrderBook book;
//     book.limit(orderly_chaos::Side::Sell, 1, 100, 10'050);
//     book.limit(orderly_chaos::Side::Buy,  2,  40, 10'050);  // trades 40

#ifndef ORDERLY_CHAOS_ORDERLY_CHAOS_HPP_
#define ORDERLY_CHAOS_ORDERLY_CHAOS_HPP_

#include "errors.hpp"
#include "limit_order_book.hpp"
#include "limit_tree.hpp"
#include "types.hpp"
#include "version.hpp"

#endif  // ORDERLY_CHAOS_ORDERLY_CHAOS_HPP_
