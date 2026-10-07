// Orderly Chaos: library version information.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Meet Mendapara

#ifndef ORDERLY_CHAOS_VERSION_HPP_
#define ORDERLY_CHAOS_VERSION_HPP_

/// Major version: incremented for incompatible API changes.
#define ORDERLY_CHAOS_VERSION_MAJOR 0
/// Minor version: incremented for backwards-compatible features.
#define ORDERLY_CHAOS_VERSION_MINOR 2
/// Patch version: incremented for backwards-compatible bug fixes.
#define ORDERLY_CHAOS_VERSION_PATCH 0
/// The full semantic version string.
#define ORDERLY_CHAOS_VERSION_STRING "0.2.0"

namespace orderly_chaos {

/// @brief Return the semantic version of the library, e.g. "0.2.0".
inline constexpr const char* version() { return ORDERLY_CHAOS_VERSION_STRING; }

}  // namespace orderly_chaos

#endif  // ORDERLY_CHAOS_VERSION_HPP_
