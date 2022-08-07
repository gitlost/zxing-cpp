/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <string_view>

namespace ZXing::TextUtfEncoding {

std::string ToUtf8(std::wstring_view str);
[[deprecated]] std::string ToUtf8(std::wstring_view str, const bool angleEscape);
std::wstring FromUtf8(std::string_view utf8);

std::string EscapeNonGraphical(std::wstring_view str);
std::string EscapeNonGraphical(std::string_view utf8);

void ToUtf8(const std::wstring& str, std::string& utf8);

// Returns the UTF-8 sequence in `str` starting at posn `start` as a Unicode codepoint, or -1 on error.
// `cnt` is set to no. of chars read (including skipped continuation bytes on error).
int Utf8Next(std::string_view str, const int start, int& cnt);

// Encode Unicode codepoint `utf32` as UTF-8
std::string Utf8Encode(const uint32_t utf32);

} // namespace ZXing::TextUtfEncoding
