/*
* Copyright 2021 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "CharacterSet.h"
#include "TextDecoder.h"
#ifndef ZXING_USE_ZINT
#include "TextEncoder.h"
#endif
#include "Utf.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ZXing;
using namespace testing;

namespace ZXing {
int Utf32ToUtf8(char32_t utf32, char* out);
}

// Encode Unicode codepoint `utf32` as UTF-8
std::string Utf32ToUtf8(const char32_t utf32)
{
	char buf[4];
	int len = Utf32ToUtf8(utf32, buf);
	return std::string(buf, len);
}

TEST(TextDecoderTest, AppendBINARY_ASCII)
{
    uint8_t data[256];
	std::iota(std::begin(data), std::end(data), 0);

	{
		std::wstring str = FromUtf8(BytesToUtf8(data, CharacterSet::BINARY));
		EXPECT_THAT(str, ElementsAreArray(data));
	}
	{
		// ASCII accepts non-ASCII
		std::wstring str = FromUtf8(BytesToUtf8(data, CharacterSet::ASCII));
		EXPECT_THAT(str, ElementsAreArray(data));
	}

	{
		// ISO646_Inv accepts non-ASCII and non-invariant
		std::wstring str = FromUtf8(BytesToUtf8(data, CharacterSet::ISO646_Inv));
		EXPECT_THAT(str, ElementsAreArray(data));
	}

	{
		// Unknown accepts anything also
		std::wstring str = FromUtf8(BytesToUtf8(data, CharacterSet::Unknown));
		EXPECT_THAT(str, ElementsAreArray(data));
	}
}

TEST(TextDecoderTest, AppendAllASCIIRange00_7F)
{
	std::string expected;
	uint8_t data[0x80] = {0};
	uint8_t dataUTF16BE[0x80 * 2] = {0};
	uint8_t dataUTF16LE[0x80 * 2] = {0};
	uint8_t dataUTF32BE[0x80 * 4] = {0};
	uint8_t dataUTF32LE[0x80 * 4] = {0};

	for (int i = 0; i < 0x80; i++) {
		uint8_t ch = static_cast<uint8_t>(i);
		data[i] = ch;
		expected.append(Utf32ToUtf8(i));

		dataUTF16BE[i * 2 + 1] = ch;
		dataUTF16LE[i * 2] = ch;

		dataUTF32BE[i * 4 + 3] = ch;
		dataUTF32LE[i * 4] = ch;
	}
	EXPECT_EQ(expected.size(), 128);

	for (int i = 0; i < static_cast<int>(CharacterSet::CharsetCount); i++) {
		std::string str;
		CharacterSet cs = static_cast<CharacterSet>(i);
		switch(cs) {
		case CharacterSet::UTF16BE: str = BytesToUtf8(dataUTF16BE, cs); break;
		case CharacterSet::UTF16LE: str = BytesToUtf8(dataUTF16LE, cs); break;
		case CharacterSet::UTF32BE: str = BytesToUtf8(dataUTF32BE, cs); break;
		case CharacterSet::UTF32LE: str = BytesToUtf8(dataUTF32LE, cs); break;
		default: str = BytesToUtf8(data, cs); break;
		}
		EXPECT_EQ(str, expected) << " charset: " << ToString(cs);
	}
}

TEST(TextDecoderTest, AppendISO8859Range80_9F)
{
	std::string expected;
	uint8_t data[0xA0 - 0x80];
	std::iota(std::begin(data), std::end(data), uint8_t(0x80));

	static const CharacterSet isos[] = {
		CharacterSet::ISO8859_1, CharacterSet::ISO8859_2, CharacterSet::ISO8859_3, CharacterSet::ISO8859_4,
		CharacterSet::ISO8859_5, CharacterSet::ISO8859_6, CharacterSet::ISO8859_7, CharacterSet::ISO8859_8,
		CharacterSet::ISO8859_7, CharacterSet::ISO8859_8, CharacterSet::ISO8859_9, CharacterSet::ISO8859_10,
		CharacterSet::ISO8859_11, // Note used to map 9 CP874 codepoints in 0x80-9F range
		CharacterSet::ISO8859_13, CharacterSet::ISO8859_14, CharacterSet::ISO8859_15, CharacterSet::ISO8859_16
	};

	for (CharacterSet iso : isos) {
		std::wstring str = FromUtf8(BytesToUtf8(data, iso));
		EXPECT_THAT(str, ElementsAreArray(data)) << "iso: " << static_cast<int>(iso);
	}
}

TEST(TextDecoderTest, AppendShift_JIS)
{
	CharacterSet cs = CharacterSet::Shift_JIS;
	{
		// Shift JIS 0x5C (backslash in ASCII) normally mapped to U+00A5 YEN SIGN, but direct ASCII mapping used
		static const uint8_t data[] = { 0x5C };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\u005C"); // Would normally be "\u00A5"
		EXPECT_EQ(ToUtf8(str), "\\"); // "¥" ditto
	}
	{
		// Non-direct ASCII mapping
		static const uint8_t data[] = { 0x5C };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs, false /*sjisASCII*/));
		EXPECT_EQ(ToUtf8(str), "\u00A5");
	}

	{
		// Shift JIS 0x815F used to go to U+FF3C FULL WIDTH REVERSE SOLIDUS, now goes to U+005C REVERSE SOLIDUS (backslash)
		static const uint8_t data[] = { 0x81, 0x5F };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), "\u005C"); // Backslash
	}
//	{
//		// Shift JIS 0x815F goes to U+FF3C (full width reverse solidus i.e. backslash)
//		const uint8_t data[] = { 0x81, 0x5F };
//		std::wstring str = FromUtf8(BytesToUtf8(data, CharacterSet::Shift_JIS));
//		EXPECT_EQ(str, L"\uFF3C");
//		EXPECT_EQ(ToUtf8(str), "＼");
//	}

	{
		// Shift JIS 0xA5 (Yen sign in ISO/IEC 8859-1) goes to U+FF65 HALFWIDTH KATAKANA MIDDLE DOT
		static const uint8_t data[] = { 0xA5 };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\uFF65");
		EXPECT_EQ(ToUtf8(str), "･");
	}

	{
		// Shift JIS 0x7E (tilde in ASCII) normally mapped to U+203E (overline), but direct ASCII mapping used
		static const uint8_t data[] = { 0x7E };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"~"); // Would normally be "\u203E"
		EXPECT_EQ(ToUtf8(str), "~"); // "‾" ditto
	}
	{
		// Non-direct ASCII mapping
		static const uint8_t data[] = { 0x7E };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs, false /*sjisASCII*/));
		EXPECT_EQ(ToUtf8(str), "\u203E");
	}

	{
		static const uint8_t data[] = { 'a', 0x83, 0xC0, 'c', 0x84, 0x47, 0xA5, 0xBF, 0x81, 0x5F, 0x93, 0x5F,
										0xE4, 0xAA, 0x83, 0x65 };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), "a\u03B2c\u0416\uFF65\uFF7F\\\u70B9\u8317\u30C6"); // Note 0x815F now -> backslash

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
	{
		static const uint8_t data[] = { 'a', 0x83, 0xC0, 'c', 0x84, 0x47, 0xA5, 0xBF, 0x93, 0x5F,
										0xE4, 0xAA, 0x83, 0x65 };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"a\u03B2c\u0416\uFF65\uFF7F\u70B9\u8317\u30C6");
		EXPECT_EQ(ToUtf8(str), "aβcЖ･ｿ点茗テ");
	}
	{
		// Non-direct ASCII mapping
		static const uint8_t data[] = { '\\', 0x83, 0xC0, '~', 0x84, 0x47, 0xA5, 0xBF, 0x81, 0x5F, 0x93, 0x5F,
										0xE4, 0xAA, 0x83, 0x65 };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs, false /*sjisASCII*/));
		EXPECT_EQ(ToUtf8(str), "\u00A5\u03B2\u203E\u0416\uFF65\uFF7F\\\u70B9\u8317\u30C6");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}

TEST(TextDecoderTest, AppendBig5)
{
	CharacterSet cs = CharacterSet::Big5;
	{
		static const uint8_t data[] = { 0xA1, 0x5A }; // Drawings box light left in Big5-2003; not in original Big5
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), "\uFFFD"); // Note used to map to U+2574, now no mapping (so replacement char U+FFFD)
	}
//	{
//		const uint8_t data[] = { 0xA1, 0x5A }; // Drawings box light left in Big5-2003; not in original Big5
//		std::wstring str = FromUtf8(BytesToUtf8(data, CharacterSet::Big5));
//		EXPECT_EQ(str, L"\u2574");
//		EXPECT_EQ(ToUtf8(str), "╴");
//	}

	{
		static const uint8_t data[] = { 0xA1, 0x56 }; // En dash U+2013 in Big5, horizontal bar U+2015 in Big5-2003
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\u2013");
		EXPECT_EQ(ToUtf8(str), "–");
	}

	{
		static const uint8_t data[] = { 0x1, ' ', 0xA1, 0x71, '@', 0xC0, 0x40, 0xF9, 0xD5, 0x7F };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\u0001 \u3008@\u9310\u9F98\u007F");
		EXPECT_EQ(ToUtf8(str), "\x01 〈@錐龘\x7F");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}

TEST(TextDecoderTest, AppendGB2312)
{
	CharacterSet cs = CharacterSet::GB2312;
	{
		// In GB 2312, 0xA1A4 -> U+30FB KATAKANA MIDDLE DOT and 0xA1AA -> U+2015 HORIZONTAL BAR
		static const uint8_t data[] = { 'a', 0xA6, 0xC2, 'c', 0xA1, 0xA4, 0xA1, 0xAA, 0xA8, 0xA6, 'Z' };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), "a\u03B2c\u30FB\u2015\u00E9Z"); // Note previous used GBK values below, now uses GB 2312 values

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}

TEST(TextDecoderTest, AppendGBK)
{
	CharacterSet cs = CharacterSet::GBK;
	{
		// In GBK, 0xA1A4 -> U+00B7 MIDDLE DOT and 0xA1AA -> U+2014 EM DASH
		const uint8_t data[] = { 'a', 0xA6, 0xC2, 'c', 0xA1, 0xA4, 0xA1, 0xAA, 0xA8, 0xA6, 'Z' };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), std::string("a\u03B2c\u00B7\u2014\u00E9Z"));

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
	{
		const uint8_t data[] = { 'a', 0xB0, 0xA1 };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"a\u554a");
		EXPECT_EQ(ToUtf8(str), "a啊");
	}
}

TEST(TextDecoderTest, AppendGB18030)
{
	CharacterSet cs = CharacterSet::GB18030;
	{
		// GB 18030 uses GBK values (superset)
		const uint8_t data[] = { 'a', 0xA6, 0xC2, 'c', 0x81, 0x39, 0xA7, 0x39, 0xA1, 0xA4, 0xA1, 0xAA,
										0xA8, 0xA6, 'Z' };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"a\u03B2c\u30FB\u00B7\u2014\u00E9Z");
		EXPECT_EQ(ToUtf8(str), "aβc・·—éZ");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}

TEST(TextDecoderTest, AppendEUC_KR)
{
	CharacterSet cs = CharacterSet::EUC_KR;
	{
		const uint8_t data[] = { 0xA2, 0xE6 }; // Euro sign U+20AC added KS X 1001:1998, now supported, previously not
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), "\u20AC"); // Note previously unmapped
	}
	{
		const uint8_t data[] = { 0xA2, 0xE6 }; // Euro sign U+20AC added KS X 1001:1998
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\u20AC");
		EXPECT_EQ(ToUtf8(str), "€");
	}

	{
		const uint8_t data[] = { 'a', 0xA4, 0xA1, 'Z' };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), "a\u3131Z");
		EXPECT_EQ(str, L"a\u3131Z");
		EXPECT_EQ(ToUtf8(str), "aㄱZ");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}

TEST(TextDecoderTest, AppendUTF16BE)
{
	CharacterSet cs = CharacterSet::UTF16BE;
	{
		const uint8_t data[] = { 0x00, 0x01, 0x00, 0x7F, 0x00, 0x80, 0x00, 0xFF, 0x01, 0xFF, 0x10, 0xFF,
										0xFF, 0xFD };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), "\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
	{
		const uint8_t data[] = { 0x00, 0x01, 0x00, 0x7F, 0x00, 0x80, 0x00, 0xFF, 0x01, 0xFF, 0x10, 0xFF,
										0xFF, 0xFD };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD");
		EXPECT_EQ(ToUtf8(str), "\x01\x7F\xC2\x80ÿǿჿ\xEF\xBF\xBD");
	}

	{
		const uint8_t data[] = { 0xD8, 0x00, 0xDC, 0x00 }; // Surrogate pair U+10000
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), std::string("\U00010000"));

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
	{
		const uint8_t data[] = { 0xD8, 0x00, 0xDC, 0x00 }; // Surrogate pair U+10000
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\U00010000");
		EXPECT_EQ(ToUtf8(str), "𐀀");
	}
}

TEST(TextDecoderTest, AppendUTF16LE)
{
	CharacterSet cs = CharacterSet::UTF16LE;
	{
		static const uint8_t data[] = { 0x01, 0x00, 0x7F, 0x00, 0x80, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0xFF, 0x10,
										0xFD, 0xFF };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), std::string("\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD"));

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}

	{
		static const uint8_t data[] = { 0x00, 0xD8, 0x00, 0xDC }; // Surrogate pair U+10000
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\U00010000");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}

TEST(TextDecoderTest, AppendUTF32BE)
{
	CharacterSet cs = CharacterSet::UTF32BE;
	{
		static const uint8_t data[] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x80,
										0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x10, 0xFF,
										0x00, 0x00, 0xFF, 0xFD };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), std::string("\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD"));

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}

	{
		static const uint8_t data[] = { 0x00, 0x01, 0x00, 0x00  }; // U+10000
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\U00010000");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}

TEST(TextDecoderTest, AppendUTF32LE)
{
	CharacterSet cs = CharacterSet::UTF32LE;
	{
		static const uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
										0xFF, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0xFF, 0x10, 0x00, 0x00,
										0xFD, 0xFF, 0x00, 0x00 };
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(ToUtf8(str), std::string("\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD"));

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}

	{
		static const uint8_t data[] = { 0x00, 0x00, 0x01, 0x00  }; // U+10000
		std::wstring str = FromUtf8(BytesToUtf8(data, cs));
		EXPECT_EQ(str, L"\U00010000");

#ifndef ZXING_USE_ZINT
		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
#endif
	}
}
