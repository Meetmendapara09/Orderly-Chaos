// Orderly Chaos: error codes and the exception type thrown by the order book.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#ifndef ORDERLY_CHAOS_ERRORS_HPP_
#define ORDERLY_CHAOS_ERRORS_HPP_

#include <stdexcept>
#include <string>

namespace orderly_chaos {

/// @brief Reasons an order book operation can be rejected.
///
/// The numeric values are part of the stable C ABI (see `oc_status` in
/// orderly_chaos.h) and must never be reordered.
enum class ErrorCode : int {
    /// A limit order was submitted with an ID that is already resting.
    DuplicateOrderId = 1,
    /// No resting order exists with the given ID.
    UnknownOrderId = 2,
    /// The quantity is zero, or larger than the order's remaining quantity.
    InvalidQuantity = 3,
    /// The limit price is zero (zero is reserved for market orders).
    InvalidPrice = 4,
};

/// @brief Return a short, human-readable description of an error code.
inline const char* to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::DuplicateOrderId: return "duplicate order id";
        case ErrorCode::UnknownOrderId:   return "unknown order id";
        case ErrorCode::InvalidQuantity:  return "invalid quantity";
        case ErrorCode::InvalidPrice:     return "invalid price";
    }
    return "unknown error";
}

/// @brief Return the C API name of an error code (for example
/// "OC_ERR_UNKNOWN_ORDER_ID").
///
/// The C, C++, and Python APIs share these names, so an error seen in one
/// language can be looked up in any API reference.
inline const char* code_name(ErrorCode code) {
    switch (code) {
        case ErrorCode::DuplicateOrderId: return "OC_ERR_DUPLICATE_ORDER_ID";
        case ErrorCode::UnknownOrderId:   return "OC_ERR_UNKNOWN_ORDER_ID";
        case ErrorCode::InvalidQuantity:  return "OC_ERR_INVALID_QUANTITY";
        case ErrorCode::InvalidPrice:      return "OC_ERR_INVALID_PRICE";
    }
    return "OC_ERR_INTERNAL";
}

/// @brief Exception thrown when an order book operation is rejected.
///
/// A rejected operation never modifies the book: validation happens before
/// any state is touched.
class OrderBookError : public std::runtime_error {
 public:
    /// @brief Create an error with the given code and detail message.
    ///
    /// @param code the machine-readable reason for the error
    /// @param detail additional context appended to the message
    ///
    OrderBookError(ErrorCode code, const std::string& detail) :
        std::runtime_error(
            std::string(to_string(code)) + " [" + code_name(code) + "]: " + detail
        ),
        code_(code) { }

    /// @brief Return the machine-readable reason for the error.
    ErrorCode code() const noexcept { return code_; }

 private:
    /// the machine-readable reason for the error
    ErrorCode code_;
};

}  // namespace orderly_chaos

#endif  // ORDERLY_CHAOS_ERRORS_HPP_
