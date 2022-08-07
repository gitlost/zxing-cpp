/*
* Copyright 2016 Nu-book Inc.
* Copyright 2021 gitlost
* Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "TextUtfEncoding.h"
#include "ZXAlgorithms.h"
#include "ZXCType.h"
#include "ZXTestSupport.h"

#include <iomanip>
#include <sstream>

namespace ZXing::TextUtfEncoding {

// TODO: c++20 has char8_t
using char8_t = uint8_t;
using utf8_t = std::basic_string_view<char8_t>;

constexpr uint32_t kAccepted = 0;
constexpr uint32_t kRejected [[maybe_unused]] = 12;

static void Utf8Decode(char8_t byte, uint32_t& state, uint32_t& codep)
{
	// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
	// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
	static const uint8_t kUtf8Data[] = {
		/* The first part of the table maps bytes to character classes that
		 * reduce the size of the transition table and create bitmasks. */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

		/* The second part is a transition table that maps a combination
		 * of a state of the automaton and a character class to a state. */
		0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
		12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
		12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
		12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
		12,36,12,12,12,12,12,12,12,12,12,12,
	};

	uint32_t type = kUtf8Data[byte];
	codep = (state != kAccepted) ? (byte & 0x3fu) | (codep << 6) : (0xff >> type) & (byte);
	state = kUtf8Data[256 + state + type];
}

static_assert(sizeof(wchar_t) == 4 || sizeof(wchar_t) == 2, "wchar_t needs to be 2 or 4 bytes wide");

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
uint32_t Utf32FromUtf16Surrogates(T high, T low)
{
	return (uint32_t(high) << 10) + low - 0x35fdc00;
}

static size_t Utf8CountCodePoints(utf8_t utf8)
{
	size_t count = 0;

	for (size_t i = 0; i < utf8.size();) {
		if (utf8[i] < 128) {
			++i;
		} else {
			switch (utf8[i] & 0xf0) {
			case 0xc0: [[fallthrough]];
			case 0xd0: i += 2; break;
			case 0xe0: i += 3; break;
			case 0xf0: i += 4; break;
			default: // we are in middle of a sequence
				++i;
				while (i < utf8.size() && (utf8[i] & 0xc0) == 0x80)
					++i;
				break;
			}
		}
		++count;
	}
	return count;
}

// Returns the UTF-8 sequence in `str` starting at posn `start` as a Unicode codepoint, or -1 on error.
// `cnt` is set to no. of chars read (including skipped continuation bytes on error).
int Utf8Next(std::string_view str, const int start, int& cnt)
{
	const char* i = str.data() + start;
	const char* const end = str.data() + str.length();
	uint32_t state = kAccepted;
	uint32_t codepoint = 0; // GCC 11.1.0 gives false warning if not initialized
	while (i < end) {
		Utf8Decode(*i++, state, codepoint);
		if (state == kRejected) {
			if (!(*(i - 1) & 0x80)) { // If previous ASCII, backtrack
				i--;
			} else {
				while (i < end && (*i & 0xC0) == 0x80) { // Skip any continuation bytes
					i++;
				}
			}
			cnt = narrow_cast<int>(i - (str.data() + start));
			return -1;
		}
		if (state == kAccepted) {
			break;
		}
	}
	cnt = narrow_cast<int>(i - (str.data() + start));
	return narrow_cast<int>(codepoint);
}

static void AppendFromUtf8(utf8_t utf8, std::wstring& buffer)
{
	buffer.reserve(buffer.size() + Utf8CountCodePoints(utf8));

	uint32_t codePoint = 0;
	uint32_t state = kAccepted;
	const uint8_t* i = utf8.data();
	const uint8_t* const end = utf8.data() + utf8.length();

	while (i < end) {
		Utf8Decode(*i++, state, codePoint);
		if (state == kRejected) {
			if (!(*(i - 1) & 0x80)) { // If previous ASCII, backtrack
				i--;
			} else {
				while (i < end && (*i & 0xC0) == 0x80) // Skip any continuation bytes
					i++;
			}
			state = kAccepted;
			continue;
		}
		if (state != kAccepted)
			continue;

		if (sizeof(wchar_t) == 2 && codePoint > 0xffff) { // surrogate pair
			buffer.push_back(narrow_cast<wchar_t>(0xd7c0 + (codePoint >> 10)));
			buffer.push_back(narrow_cast<wchar_t>(0xdc00 + (codePoint & 0x3ff)));
		} else {
			buffer.push_back(narrow_cast<wchar_t>(codePoint));
		}
	}
}

std::wstring FromUtf8(std::string_view utf8)
{
	std::wstring str;
	AppendFromUtf8({reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()}, str);
	return str;
}

// Count the number of bytes required to store given code points in UTF-8.
static size_t Utf8CountBytes(std::wstring_view str)
{
	int result = 0;
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] < 0x80)
			result += 1;
		else if (str[i] < 0x800)
			result += 2;
		else if (sizeof(wchar_t) == 4) {
			if (str[i] < 0x10000)
				result += 3;
			else
				result += 4;
		} else {
			if (IsUtf16HighSurrogate(str[i])) {
				result += 4;
				++i;
			} else
				result += 3;
		}
	}
	return result;
}

ZXING_EXPORT_TEST_ONLY
int Utf32ToUtf8(const uint32_t utf32, char* out)
{
	if (utf32 < 0x80) {
		*out++ = static_cast<uint8_t>(utf32);
		return 1;
	}
	if (utf32 < 0x800) {
		*out++ = narrow_cast<uint8_t>((utf32 >> 6) | 0xc0);
		*out++ = narrow_cast<uint8_t>((utf32 & 0x3f) | 0x80);
		return 2;
	}
	if (utf32 < 0x10000) {
		*out++ = narrow_cast<uint8_t>((utf32 >> 12) | 0xe0);
		*out++ = narrow_cast<uint8_t>(((utf32 >> 6) & 0x3f) | 0x80);
		*out++ = narrow_cast<uint8_t>((utf32 & 0x3f) | 0x80);
		return 3;
	}

	*out++ = narrow_cast<uint8_t>((utf32 >> 18) | 0xf0);
	*out++ = narrow_cast<uint8_t>(((utf32 >> 12) & 0x3f) | 0x80);
	*out++ = narrow_cast<uint8_t>(((utf32 >> 6) & 0x3f) | 0x80);
	*out++ = narrow_cast<uint8_t>((utf32 & 0x3f) | 0x80);
	return 4;
}

// Encode Unicode codepoint `utf32` as UTF-8
std::string Utf8Encode(const uint32_t utf32)
{
	char buf[4];
	int len = Utf32ToUtf8(utf32, buf);
	return std::string(buf, len);
}

static void AppendToUtf8(std::wstring_view str, std::string& utf8)
{
	utf8.reserve(utf8.size() + Utf8CountBytes(str));

	char buffer[4];
	for (size_t i = 0; i < str.size(); ++i)
	{
		uint32_t cp;
		if (sizeof(wchar_t) == 2 && i + 1 < str.size() && IsUtf16HighSurrogate(str[i]) && IsUtf16LowSurrogate(str[i + 1])) {
			cp = Utf32FromUtf16Surrogates(str[i], str[i + 1]);
			++i;
		} else
			cp = str[i];

		auto bufLength = Utf32ToUtf8(cp, buffer);
		utf8.append(buffer, bufLength);
	}
}

std::string ToUtf8(std::wstring_view str)
{
	std::string utf8;
	AppendToUtf8(str, utf8);
	return utf8;
}

// Same as `ToUtf8()` above, except if angleEscape set, places non-graphical characters in angle brackets with text name
std::string ToUtf8(std::wstring_view str, const bool angleEscape)
{
	if (angleEscape)
		return EscapeNonGraphical(str);

	return ToUtf8(str);
}

std::string EscapeNonGraphical(std::wstring_view str)
{
	return EscapeNonGraphical(ToUtf8(str));
}

// Places non-graphical characters in angle brackets with text name
std::string EscapeNonGraphical(std::string_view str)
{
	static const char* const ascii_nongraphs[33] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		 "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN",  "EM", "SUB", "ESC",  "FS",  "GS",  "RS",  "US",
		"DEL",
	};

	std::ostringstream oss;

	oss.fill('0');

	int cnt;
	for (int posn = 0, len = narrow_cast<int>(str.length()); posn < len; posn += cnt) {
		int u = Utf8Next(str, posn, cnt);
		if (u == -1) { // Invalid UTF-8
			// Write out each byte with "0x" prefix
			for (int i = 0; i < cnt; i++) {
				oss << "<0x" << std::setw(2) << std::uppercase << std::hex
				   << static_cast<unsigned int>(str[posn + i] & 0xFF) << ">";
			}
		} else if (u < 128) { // ASCII
			if (u < 32 || u == 127) { // Non-graphical ASCII, excluding space
				oss << "<" << ascii_nongraphs[u == 127 ? 32 : u] << ">";
			} else {
				oss << str.substr(posn, cnt);
			}
		} else {
			if (zx_iswgraph(u)) {
				oss << str.substr(posn, cnt);
			} else { // Non-graphical Unicode
				int width = u < 256 ? 2 : 4;
				oss << "<U+" << std::setw(width) << std::uppercase << std::hex
				   << static_cast<unsigned int>(u) << ">";
			}
		}
	}

	return oss.str();
}

} // namespace ZXing::TextUtfEncoding
