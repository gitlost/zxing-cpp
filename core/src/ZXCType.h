#pragma once
/*
* Copyright 2022-2023 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>

namespace ZXing {

/*
  Versions of <ctype> functions with no dependence on locale.
  `u` can be any Unicode codepoint U+0000-10FFFF, though only the `w` versions access non-ASCII.
 */
static inline int zx_isupper(uint32_t u) { return u >= 'A' && u <= 'Z'; }
static inline int zx_islower(uint32_t u) { return u >= 'a' && u <= 'z'; }
static inline int zx_isdigit(uint32_t u) { return u <= '9' && u >= '0'; }
static inline int zx_isspace(uint32_t u) { return u == ' ' || (u <= '\r' && u >= '\t'); }

// Note this is dependent on Unicode version used to generate tables.
int zx_iswgraph(uint32_t u); /* Returns 1 if is, zero if not */

} // ZXing
