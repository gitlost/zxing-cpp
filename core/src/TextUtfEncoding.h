/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ZXing {
namespace TextUtfEncoding {

std::string ToUtf8(const std::wstring& str);
std::string ToUtf8(const std::wstring& str, const bool angleEscape);
std::string AngleEscape(std::string_view str);
std::wstring FromUtf8(const std::string& utf8);

void ToUtf8(const std::wstring& str, std::string& utf8);

// Returns the UTF-8 sequence in `str` starting at posn `start` as a Unicode codepoint, or -1 on error.
// `cnt` is set to no. of chars read (including skipped continuation bytes on error).
int Utf8Next(std::string_view str, const int start, int& cnt);

// Encode Unicode codepoint `utf32` as UTF-8 into `out`, returning no. of UTF-8 chars
int Utf8Encode(const uint32_t utf32, char* out);

template <typename T>
bool IsUtf16HighSurrogate(T c)
{
	return (c & 0xfc00) == 0xd800;
}

template <typename T>
bool IsUtf16LowSurrogate(T c)
{
	return (c & 0xfc00) == 0xdc00;
}

template <typename T>
uint32_t CodePointFromUtf16Surrogates(T high, T low)
{
	return (uint32_t(high) << 10) + low - 0x35fdc00;
}

} // namespace TextUtfEncoding
} // namespace ZXing
