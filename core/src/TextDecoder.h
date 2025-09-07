/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CharacterSet.h"
#include "ECI.h"
#include "Range.h"
#if __has_include("Version.h")
#include "Version.h"
#endif

#include <string>

namespace ZXing {

#ifdef ZXING_READERS
std::string BytesToUtf8(ByteView bytes, ECI eci, bool sjisASCII = true);

inline std::string BytesToUtf8(ByteView bytes, CharacterSet cs, bool sjisASCII = true)
{
	return BytesToUtf8(bytes, ToECI(cs), sjisASCII);
}
#endif

CharacterSet GuessTextEncoding(ByteView bytes, CharacterSet fallback = CharacterSet::ISO8859_1);

} // ZXing
