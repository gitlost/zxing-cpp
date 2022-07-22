/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CharacterSet.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ZXing {

class TextDecoder
{
public:
	static CharacterSet DefaultEncoding();
	static CharacterSet GuessEncoding(const uint8_t* bytes, size_t length, CharacterSet fallback = DefaultEncoding());

	// If `sjisASCII` set then for Shift_JIS maps ASCII directly (straight-thru), i.e. does not map ASCII backslash & tilde
	// to Yen sign & overline resp. (JIS X 0201 Roman)
	static void Append(std::string& str, const uint8_t* bytes, size_t length, CharacterSet charset, bool sjisASCII = true);
};

} // ZXing
