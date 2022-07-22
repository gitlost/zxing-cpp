/*
* Copyright 2021 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "CharacterSet.h"
#include "TextDecoder.h"
#include "TextEncoder.h"
#include "TextUtfEncoding.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ZXing;
using namespace ZXing::TextUtfEncoding;
using namespace testing;

TEST(TextDecoderTest, AppendBINARY_ASCII)
{
	std::string expected;
	uint8_t data[256];
	for (int i = 0; i < 256; i++) {
		uint8_t ch = static_cast<uint8_t>(i);
		data[i] = ch;
		char buf[4];
		int len = Utf8Encode(i, buf);
		expected.append(buf, len);
	}
	EXPECT_EQ(expected.size(), 256 + 128);

	{
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), CharacterSet::BINARY);
		EXPECT_EQ(str, expected);

		TextDecoder::Append(str, data, sizeof(data), CharacterSet::BINARY);
		EXPECT_EQ(str, expected + expected);
	}

	{
		// ASCII accepts non-ASCII
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), CharacterSet::ASCII);
		EXPECT_EQ(str, expected);
	}

	{
		// ISO646_Inv accepts non-ASCII and non-invariant
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), CharacterSet::ISO646_Inv);
		EXPECT_EQ(str, expected);
	}

	{
		// Unknown accepts anything also
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), CharacterSet::Unknown);
		EXPECT_EQ(str, expected);
	}
}

TEST(TextDecoderTest, AppendAllASCIIRange00_7F)
{
	std::string expected;
	uint8_t data[0x80];
	uint8_t dataUnicodeBig[0x80 * 2];
	uint8_t dataUTF16LE[0x80 * 2];
	uint8_t dataUTF32BE[0x80 * 4];
	uint8_t dataUTF32LE[0x80 * 4];

	for (int i = 0; i < 0x80; i++) {
		uint8_t ch = static_cast<uint8_t>(i);
		data[i] = ch;
		char buf[4];
		int len = Utf8Encode(i, buf);
		expected.append(buf, len);

		int j = i << 1;
		int k = j << 1;

		dataUnicodeBig[j] = 0;
		dataUnicodeBig[j + 1] = ch;

		dataUTF16LE[j] = ch;
		dataUTF16LE[j + 1] = 0;

		dataUTF32BE[k] = dataUTF32BE[k + 1] = dataUTF32BE[k + 2] = 0;
		dataUTF32BE[k + 3] = ch;

		dataUTF32LE[k] = ch;
		dataUTF32LE[k + 1] = dataUTF32LE[k + 2] = dataUTF32LE[k + 3] = 0;
	}
	EXPECT_EQ(expected.size(), 128);

	for (int i = 0; i < static_cast<int>(CharacterSet::CharsetCount); i++) {
		std::string str;
		CharacterSet cs = static_cast<CharacterSet>(i);
		if (cs == CharacterSet::UnicodeBig) {
			TextDecoder::Append(str, dataUnicodeBig, sizeof(dataUnicodeBig), cs);
		} else if (cs == CharacterSet::UTF16LE) {
			TextDecoder::Append(str, dataUTF16LE, sizeof(dataUTF16LE), cs);
		} else if (cs == CharacterSet::UTF32BE) {
			TextDecoder::Append(str, dataUTF32BE, sizeof(dataUTF32BE), cs);
		} else if (cs == CharacterSet::UTF32LE) {
			TextDecoder::Append(str, dataUTF32LE, sizeof(dataUTF32LE), cs);
		} else {
			TextDecoder::Append(str, data, sizeof(data), cs);
		}
		EXPECT_EQ(str, expected) << " charset: " << ToString(cs);
	}
}

TEST(TextDecoderTest, AppendISO8859Range80_9F)
{
	std::string expected;
	uint8_t data[0xA0 - 0x80];
	for (int i = 0x80; i < 0xA0; i++) {
		uint8_t ch = static_cast<uint8_t>(i);
		data[i - 0x80] = ch;
		char buf[4];
		int len = Utf8Encode(i, buf);
		expected.append(buf, len);
	}
	EXPECT_EQ(expected.size(), 0x20 * 2);

	static const CharacterSet isos[] = {
		CharacterSet::ISO8859_1, CharacterSet::ISO8859_2, CharacterSet::ISO8859_3, CharacterSet::ISO8859_4,
		CharacterSet::ISO8859_5, CharacterSet::ISO8859_6, CharacterSet::ISO8859_7, CharacterSet::ISO8859_8,
		CharacterSet::ISO8859_7, CharacterSet::ISO8859_8, CharacterSet::ISO8859_9, CharacterSet::ISO8859_10,
		CharacterSet::ISO8859_11, // Note used to map 9 CP874 codepoints in 0x80-9F range
		CharacterSet::ISO8859_13, CharacterSet::ISO8859_14, CharacterSet::ISO8859_15, CharacterSet::ISO8859_16
	};

	for (CharacterSet iso : isos) {
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), iso);
		EXPECT_EQ(str, expected) << " iso: " << ToString(iso);
	}
}

TEST(TextDecoderTest, AppendShift_JIS)
{
	CharacterSet cs = CharacterSet::Shift_JIS;
	{
		// Shift JIS 0x5C (backslash in ASCII) normally mapped to U+00A5 YEN SIGN, but direct ASCII mapping used
		static const uint8_t data[] = { 0x5C };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\u005C"); // Would normally be "\u00A5" ("¥")
	}
	{
		// Non-direct ASCII mapping
		static const uint8_t data[] = { 0x5C };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs, false /*sjisASCII*/);
		EXPECT_EQ(str, u8"\u00A5");
	}

	{
		// Shift JIS 0x815F used to go to U+FF3C FULL WIDTH REVERSE SOLIDUS, now goes to U+005C REVERSE SOLIDUS (backslash)
		static const uint8_t data[] = { 0x81, 0x5F };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\u005C"); // Backslash
	}

	{
		// Shift JIS 0xA5 (Yen sign in ISO/IEC 8859-1) goes to U+FF65 HALFWIDTH KATAKANA MIDDLE DOT
		static const uint8_t data[] = { 0xA5 };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\uFF65");
	}

	{
		// Shift JIS 0x7E (tilde in ASCII) normally mapped to U+203E (overline), but direct ASCII mapping used
		static const uint8_t data[] = { 0x7E };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\u007E"); // Tilde, would normally be "\u203E" ("‾")
	}
	{
		// Non-direct ASCII mapping
		static const uint8_t data[] = { 0x7E };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs, false /*sjisASCII*/);
		EXPECT_EQ(str, u8"\u203E");
	}

	{
		static const uint8_t data[] = { 'a', 0x83, 0xC0, 'c', 0x84, 0x47, 0xA5, 0xBF, 0x81, 0x5F, 0x93, 0x5F,
										0xE4, 0xAA, 0x83, 0x65 };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"a\u03B2c\u0416\uFF65\uFF7F\\\u70B9\u8317\u30C6"); // Note 0x815F now -> backslash

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
	{
		// Non-direct ASCII mapping
		static const uint8_t data[] = { '\\', 0x83, 0xC0, '~', 0x84, 0x47, 0xA5, 0xBF, 0x81, 0x5F, 0x93, 0x5F,
										0xE4, 0xAA, 0x83, 0x65 };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs, false /*sjisASCII*/);
		EXPECT_EQ(str, u8"\u00A5\u03B2\u203E\u0416\uFF65\uFF7F\\\u70B9\u8317\u30C6");

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendBig5)
{
	CharacterSet cs = CharacterSet::Big5;
	{
		static const uint8_t data[] = { 0xA1, 0x5A }; // Drawings box light left in Big5-2003; not in original Big5
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\uFFFD"); // Note used to map to U+2574, now no mapping (so replacement char U+FFFD)
	}

	{
		static const uint8_t data[] = { 0xA1, 0x56 }; // En dash U+2013 in Big5, horizontal bar U+2015 in Big5-2003
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\u2013");
	}

	{
		static const uint8_t data[] = { 0x1, ' ', 0xA1, 0x71, '@', 0xC0, 0x40, 0xF9, 0xD5, 0x7F };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\u0001 \u3008@\u9310\u9F98\u007F");

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendGB2312)
{
	CharacterSet cs = CharacterSet::GB2312;
	{
		// In GB 2312, 0xA1A4 -> U+30FB KATAKANA MIDDLE DOT and 0xA1AA -> U+2015 HORIZONTAL BAR
		static const uint8_t data[] = { 'a', 0xA6, 0xC2, 'c', 0xA1, 0xA4, 0xA1, 0xAA, 0xA8, 0xA6, 'Z' };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"a\u03B2c\u30FB\u2015\u00E9Z"); // Note previous used GBK values below, now uses GB 2312 values

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendGBK)
{
	CharacterSet cs = CharacterSet::GBK;
	{
		// In GBK, 0xA1A4 -> U+00B7 MIDDLE DOT and 0xA1AA -> U+2014 EM DASH
		static const uint8_t data[] = { 'a', 0xA6, 0xC2, 'c', 0xA1, 0xA4, 0xA1, 0xAA, 0xA8, 0xA6, 'Z' };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, std::string(u8"a\u03B2c\u00B7\u2014\u00E9Z"));

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendGB18030)
{
	CharacterSet cs = CharacterSet::GB18030;
	{
		// GB 18030 uses GBK values (superset)
		static const uint8_t data[] = { 'a', 0xA6, 0xC2, 'c', 0x81, 0x39, 0xA7, 0x39, 0xA1, 0xA4, 0xA1, 0xAA,
										0xA8, 0xA6, 'Z' };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"a\u03B2c\u30FB\u00B7\u2014\u00E9Z");

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendEUC_KR)
{
	CharacterSet cs = CharacterSet::EUC_KR;
	{
		static const uint8_t data[] = { 0xA2, 0xE6 }; // Euro sign U+20AC added KS X 1001:1998, now supported, previously not
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\u20AC"); // Note previously unmapped
	}

	{
		static const uint8_t data[] = { 'a', 0xA4, 0xA1, 'Z' };
		std::string str;
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"a\u3131Z");

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendUnicodeBig)
{
	CharacterSet cs = CharacterSet::UnicodeBig;
	{
		std::string str;
		static const uint8_t data[] = { 0x00, 0x01, 0x00, 0x7F, 0x00, 0x80, 0x00, 0xFF, 0x01, 0xFF, 0x10, 0xFF,
										0xFF, 0xFD };
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD");

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}

	{
		std::string str;
		static const uint8_t data[] = { 0xD8, 0x00, 0xDC, 0x00 }; // Surrogate pair U+10000
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, std::string(u8"\U00010000"));

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendUTF16LE)
{
	CharacterSet cs = CharacterSet::UTF16LE;
	{
		std::string str;
		static const uint8_t data[] = { 0x01, 0x00, 0x7F, 0x00, 0x80, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0xFF, 0x10,
										0xFD, 0xFF };
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, std::string(u8"\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD"));

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}

	{
		std::string str;
		static const uint8_t data[] = { 0x00, 0xD8, 0x00, 0xDC }; // Surrogate pair U+10000
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, std::string(u8"\U00010000"));

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendUTF32BE)
{
	CharacterSet cs = CharacterSet::UTF32BE;
	{
		std::string str;
		static const uint8_t data[] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x80,
										0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x10, 0xFF,
										0x00, 0x00, 0xFF, 0xFD };
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, std::string(u8"\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD"));

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}

	{
		std::string str;
		static const uint8_t data[] = { 0x00, 0x01, 0x00, 0x00  }; // U+10000
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, std::string(u8"\U00010000"));

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendUTF32LE)
{
	CharacterSet cs = CharacterSet::UTF32LE;
	{
		std::string str;
		static const uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
										0xFF, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0xFF, 0x10, 0x00, 0x00,
										0xFD, 0xFF, 0x00, 0x00 };
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, std::string(u8"\u0001\u007F\u0080\u00FF\u01FF\u10FF\uFFFD"));

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}

	{
		std::string str;
		static const uint8_t data[] = { 0x00, 0x00, 0x01, 0x00  }; // U+10000
		TextDecoder::Append(str, data, sizeof(data), cs);
		EXPECT_EQ(str, u8"\U00010000");

		std::string enc = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(enc, std::string(reinterpret_cast<const char*>(data), sizeof(data)));
	}
}

TEST(TextDecoderTest, AppendMixed)
{
	std::string str;

	static const uint8_t dataBINARY[] = { 0x80, 0x9F, 0xA0, 0xFF };
	std::string expectedBINARY(u8"\u0080\u009F\u00A0\u00FF");

	TextDecoder::Append(str, dataBINARY, sizeof(dataBINARY), CharacterSet::BINARY);
	EXPECT_EQ(str, expectedBINARY);

	static const uint8_t dataASCII[] = { '\0', '\x1F', '\\', '~', '\x7F' };
	std::string expectedASCII(u8"\u0000\u001F\u005C\u007E\u007F", 5);

	TextDecoder::Append(str, dataASCII, sizeof(dataASCII), CharacterSet::ASCII);
	EXPECT_EQ(str, expectedBINARY + expectedASCII);

	static const uint8_t dataShift_JIS[] = { 'a', 0x83, 0xC0, 'c', 0x84, 0x47 };
	std::string expectedShift_JIS(u8"a\u03B2c\u0416");

	TextDecoder::Append(str, dataShift_JIS, sizeof(dataShift_JIS), CharacterSet::Shift_JIS);
	EXPECT_EQ(str, expectedBINARY + expectedASCII + expectedShift_JIS);

	static const uint8_t dataEUC_KR[] = { 0xA2, 0xE6, 'a', 0xA4, 0xA1, 'Z' };
	std::string expectedEUC_KR(u8"\u20ACa\u3131Z");

	TextDecoder::Append(str, dataEUC_KR, sizeof(dataEUC_KR), CharacterSet::EUC_KR);
	EXPECT_EQ(str, expectedBINARY + expectedASCII + expectedShift_JIS + expectedEUC_KR);

	static const uint8_t dataUnicodeBig[] = { 0x00, 0x00, 0x00, 0xFF, 0x30, 0xFD };
	std::string expectedUnicodeBig(u8"\u0000\u00FF\u30FD", 6);

	TextDecoder::Append(str, dataUnicodeBig, sizeof(dataUnicodeBig), CharacterSet::UnicodeBig);
	EXPECT_EQ(str, expectedBINARY + expectedASCII + expectedShift_JIS + expectedEUC_KR + expectedUnicodeBig);
}
