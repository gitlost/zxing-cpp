#pragma once
/*
* Copyright 2022-2023 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>

namespace ZXing {

/*
  `u` can be any Unicode codepoint U+0000-10FFFF.
 */

// Note this is dependent on Unicode version used to generate tables.
int zx_iswgraph(uint32_t u); /* Returns 1 if is, zero if not */

} // ZXing
