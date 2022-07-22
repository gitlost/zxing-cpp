/*
* Copyright 2021 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "CharacterSet.h"
#include "TextDecoder.h"
#include "TextEncoder.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ZXing;
using namespace testing;

TEST(TextEncoderTest, Cp437)
{
	CharacterSet cs = CharacterSet::Cp437;
	{
		std::string str(u8"\u00C7"); // LATIN CAPITAL LETTER C WITH CEDILLA
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x80");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_1)
{
	CharacterSet cs = CharacterSet::ISO8859_1;
	{
		std::string str(u8"\u0080");
		EXPECT_THROW(TextEncoder::FromUnicode(str, cs), std::invalid_argument); // Not mapped
	}
	{
		std::string str(u8"\u00A0"); // NO-BREAK SPACE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA0");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_2)
{
	CharacterSet cs = CharacterSet::ISO8859_2;
	{
		std::string str(u8"\u0104"); // LATIN CAPITAL LETTER A WITH OGONEK
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA1");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_3)
{
	CharacterSet cs = CharacterSet::ISO8859_3;
	{
		std::string str(u8"\u0126"); // LATIN CAPITAL LETTER H WITH STROKE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA1");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_4)
{
	CharacterSet cs = CharacterSet::ISO8859_4;
	{
		std::string str(u8"\u0138"); // LATIN SMALL LETTER KRA
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA2");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_5)
{
	CharacterSet cs = CharacterSet::ISO8859_5;
	{
		std::string str(u8"\u045F"); // CYRILLIC SMALL LETTER DZHE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xFF");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_6)
{
	CharacterSet cs = CharacterSet::ISO8859_6;
	{
		std::string str(u8"\u0652"); // ARABIC SUKUN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xF2");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_7)
{
	CharacterSet cs = CharacterSet::ISO8859_7;
	{
		std::string str(u8"\u03CE"); // GREEK SMALL LETTER OMEGA WITH TONOS
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xFE");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_8)
{
	CharacterSet cs = CharacterSet::ISO8859_8;
	{
		std::string str(u8"\u05EA"); // HEBREW LETTER TAV
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xFA");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_9)
{
	CharacterSet cs = CharacterSet::ISO8859_9;
	{
		std::string str(u8"\u011E"); // LATIN CAPITAL LETTER G WITH BREVE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xD0");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_10)
{
	CharacterSet cs = CharacterSet::ISO8859_10;
	{
		std::string str(u8"\u0138"); // LATIN SMALL LETTER KRA
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xFF");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_11)
{
	CharacterSet cs = CharacterSet::ISO8859_11;
	{
		std::string str(u8"\u0E5B"); // THAI CHARACTER KHOMUT
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xFB");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_13)
{
	CharacterSet cs = CharacterSet::ISO8859_13;
	{
		std::string str(u8"\u2019"); // RIGHT SINGLE QUOTATION MARK
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xFF");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_14)
{
	CharacterSet cs = CharacterSet::ISO8859_14;
	{
		std::string str(u8"\u1E6B"); // LATIN SMALL LETTER T WITH DOT ABOVE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xF7");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_15)
{
	CharacterSet cs = CharacterSet::ISO8859_15;
	{
		std::string str(u8"\u00BF"); // INVERTED QUESTION MARK
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xBF");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO8859_16)
{
	CharacterSet cs = CharacterSet::ISO8859_16;
	{
		std::string str(u8"\u017C"); // LATIN SMALL LETTER Z WITH DOT ABOVE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xBF");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, Shift_JIS)
{
	CharacterSet cs = CharacterSet::Shift_JIS;
	{
		std::string str(u8"\u00A5"); // YEN SIGN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x5C"); // Mapped to backslash

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, "\\"); // Mapped straight-thru to backslash
	}
	{
		std::string str(u8"\u203E"); // OVERLINE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x7E"); // Mapped to tilde

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, "~"); // Mapped straight-thru to tilde
	}
	{
		std::string str(u8"\u3000"); // IDEOGRAPHIC SPACE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x81\x40");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, Cp1250)
{
	CharacterSet cs = CharacterSet::Cp1250;
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x80");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, Cp1251)
{
	CharacterSet cs = CharacterSet::Cp1251;
	{
		std::string str(u8"\u045F"); // CYRILLIC SMALL LETTER DZHE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x9F");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, Cp1252)
{
	CharacterSet cs = CharacterSet::Cp1252;
	{
		std::string str(u8"\u02DC"); // SMALL TILDE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x98");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, Cp1256)
{
	CharacterSet cs = CharacterSet::Cp1256;
	{
		std::string str(u8"\u0686"); // ARABIC LETTER TCHEH
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x8D");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, UnicodeBig)
{
	CharacterSet cs = CharacterSet::UnicodeBig; // UTF16BE
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x20\xAC");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, UTF8)
{
	CharacterSet cs = CharacterSet::UTF8;
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xE2\x82\xAC");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ASCII)
{
	CharacterSet cs = CharacterSet::ASCII;
	{
		std::string str("#");
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "#");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
	{
		std::string str(u8"\u00A0"); // Not mapped
		EXPECT_THROW(TextEncoder::FromUnicode(str, cs), std::invalid_argument);
	}
}

TEST(TextEncoderTest, Big5)
{
	CharacterSet cs = CharacterSet::Big5;
	{
		std::string str(u8"\u3000"); // IDEOGRAPHIC SPACE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA1\x40");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, GB2312)
{
	CharacterSet cs = CharacterSet::GB2312;
	{
		std::string str(u8"\u3000"); // IDEOGRAPHIC SPACE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA1\xA1");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, EUC_KR)
{
	CharacterSet cs = CharacterSet::EUC_KR;
	{
		std::string str(u8"\u3000"); // IDEOGRAPHIC SPACE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA1\xA1");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, GBK)
{
	CharacterSet cs = CharacterSet::GBK;
	{
		std::string str(u8"\u3000"); // IDEOGRAPHIC SPACE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA1\xA1");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, GB18030)
{
	CharacterSet cs = CharacterSet::GB18030;
	{
		std::string str(u8"\u3000"); // IDEOGRAPHIC SPACE
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xA1\xA1");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, UTF16LE)
{
	CharacterSet cs = CharacterSet::UTF16LE;
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\xAC\x20");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, UTF32BE)
{
	CharacterSet cs = CharacterSet::UTF32BE;
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, std::string("\x00\x00\x20\xAC", 4));

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, UTF32LE)
{
	CharacterSet cs = CharacterSet::UTF32LE;
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, std::string("\xAC\x20\x00\x00", 4));

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
}

TEST(TextEncoderTest, ISO646_Inv)
{
	CharacterSet cs = CharacterSet::ISO646_Inv;
	{
		std::string str("%");
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "%");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
	{
		std::string str("#"); // Not mapped
		EXPECT_THROW(TextEncoder::FromUnicode(str, cs), std::invalid_argument);
	}
}

TEST(TextEncoderTest, BINARY)
{
	CharacterSet cs = CharacterSet::BINARY;
	{
		std::string str(u8"\u0080\u00FF");
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x80\xFF");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		EXPECT_THROW(TextEncoder::FromUnicode(str, cs), std::invalid_argument); // Not mapped
	}
}

TEST(TextEncoderTest, Unknown)
{
	CharacterSet cs = CharacterSet::Unknown; // Treated as binary
	{
		std::string str(u8"\u0080");
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x80");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		EXPECT_THROW(TextEncoder::FromUnicode(str, cs), std::invalid_argument); // Not mapped
	}
}

TEST(TextEncoderTest, EUC_JP)
{
	CharacterSet cs = CharacterSet::EUC_JP; // Not supported, treated as binary
	{
		std::string str(u8"\u0080");
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		EXPECT_EQ(bytes, "\x80");

		std::string dec;
		TextDecoder::Append(dec, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), cs);
		EXPECT_EQ(dec, str);
	}
	{
		std::string str(u8"\u20AC"); // EURO SIGN
		EXPECT_THROW(TextEncoder::FromUnicode(str, cs), std::invalid_argument); // Not mapped
	}
}
