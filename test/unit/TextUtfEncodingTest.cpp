/*
* Copyright 2021 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "TextUtfEncoding.h"
#include "Utf.h"

#include "gtest/gtest.h"
#include <vector>

using namespace ZXing;

#if __cplusplus > 201703L
std::string EscapeNonGraphical(const char8_t* utf8)
{
	return EscapeNonGraphical(reinterpret_cast<const char*>(utf8));
}
#endif

TEST(TextUtfEncodingTest, ToUtf8)
{
	EXPECT_EQ(ToUtf8(L"\u00B6\u0416"), "\u00B6\u0416");

	EXPECT_EQ(ToUtf8(L"\u00B6\xD800\u0416"), "\xC2\xB6\xED\xA0\x80\xD0\x96"); // Converts unpaired surrogate
}

TEST(TextUtfEncodingTest, EscapeNonGraphical)
{
	EXPECT_EQ(EscapeNonGraphical("\u00B6\u0416"), "\u00B6\u0416");
	EXPECT_EQ(EscapeNonGraphical("\x01\x1F\x7F"), "<SOH><US><DEL>");
	EXPECT_EQ(EscapeNonGraphical("\u0080\u009F"), "<U+80><U+9F>");
	EXPECT_EQ(EscapeNonGraphical("\u00A0"), "<U+A0>"); // NO-BREAK space (nbsp)
	EXPECT_EQ(EscapeNonGraphical("\u2007"), "<U+2007>"); // NO-BREAK space (numsp)
	EXPECT_EQ(EscapeNonGraphical("\uFFEF"), "<U+FFEF>"); // Was NO-BREAK space but now isn't (BOM)
	EXPECT_EQ(EscapeNonGraphical("\u2000"), "<U+2000>"); // Space char (nqsp)
	EXPECT_EQ(EscapeNonGraphical("\uFFFD"), "\uFFFD");
	EXPECT_EQ(EscapeNonGraphical("\uFFFF"), "<U+FFFF>");
}

TEST(TextUtfEncodingTest, ToUtf8AngleEscape)
{
	bool angleEscape = true;

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(disable: 4996) /* was declared deprecated */
#endif

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u00B6\u0416", angleEscape), "\u00B6\u0416");
	EXPECT_EQ(EscapeNonGraphical("\u00B6\u0416"), "\u00B6\u0416");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u2602", angleEscape), "\u2602");
	EXPECT_EQ(EscapeNonGraphical("\u2602"), "\u2602");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\x01\x1F\x7F", angleEscape), "<SOH><US><DEL>");
	EXPECT_EQ(EscapeNonGraphical("\x01\x1F\x7F"), "<SOH><US><DEL>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\x80\x9F", angleEscape), "<U+80><U+9F>");
	EXPECT_EQ(EscapeNonGraphical("\u0080\u009F"), "<U+80><U+9F>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\xA0", angleEscape), "<U+A0>"); // NO-BREAK space (nbsp)
	EXPECT_EQ(EscapeNonGraphical("\u00A0"), "<U+A0>");
	EXPECT_EQ(EscapeNonGraphical("\xA0"), "<0xA0>"); // Not UTF-8

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\x2007", angleEscape), "<U+2007>"); // NO-BREAK space (numsp)
	EXPECT_EQ(EscapeNonGraphical("\u2007"), "<U+2007>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\xFFEF", angleEscape), "<U+FFEF>"); // Was NO-BREAK space but now isn't (BOM)
	EXPECT_EQ(EscapeNonGraphical("\uFFEF"), "<U+FFEF>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u0100", angleEscape), "\u0100");
	EXPECT_EQ(EscapeNonGraphical("\u0100"), "\u0100");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u1000", angleEscape), "\u1000");
	EXPECT_EQ(EscapeNonGraphical("\u1000"), "\u1000");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u2000", angleEscape), "<U+2000>"); // Space char (nqsp)
	EXPECT_EQ(EscapeNonGraphical("\u2000"), "<U+2000>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\uFFFD", angleEscape), "\uFFFD");
	EXPECT_EQ(EscapeNonGraphical("\uFFFD"), "\uFFFD");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\uFFFF", angleEscape), "<U+FFFF>");
	EXPECT_EQ(EscapeNonGraphical("\uFFFF"), "<U+FFFF>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\U00010000", angleEscape), "\U00010000");
	EXPECT_EQ(EscapeNonGraphical("\U00010000"), "\U00010000");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\xD800Z", angleEscape), "<0xED><0xA0><0x80>Z"); // Unpaired high surrogate
	EXPECT_EQ(EscapeNonGraphical("\xED\xA0\x80Z"), "<0xED><0xA0><0x80>Z");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"A\xDC00", angleEscape), "A<0xED><0xB0><0x80>"); // Unpaired low surrogate
	EXPECT_EQ(EscapeNonGraphical("A\xED\xB0\x80"), "A<0xED><0xB0><0x80>");

	// Malformed UTF-8
	EXPECT_EQ(EscapeNonGraphical("A\x80\x91\xA2" "B\xC2" "C\xE2\xA4\xF0\x90\x8D" "D"),
				"A<0x80><0x91><0xA2>B<0xC2>C<0xE2><0xA4><0xF0><0x90><0x8D>D");

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

TEST(TextUtfEncodingTest, FromUtf8)
{
	EXPECT_EQ(FromUtf8("\U00010000"), L"\U00010000");
	EXPECT_EQ(FromUtf8("\U00010FFF"), L"\U00010FFF");
	EXPECT_EQ(FromUtf8("A\xE8\x80\xBFG"), L"A\u803FG"); // U+803F

	EXPECT_EQ(FromUtf8("A\xE8\x80\xBF\x80G"), L"A\u803FG"); // Bad UTF-8 (extra continuation byte)
	EXPECT_EQ(FromUtf8("A\xE8\x80\xC0G"), L"AG");           // Bad UTF-8 (non-continuation byte)
	EXPECT_EQ(FromUtf8("A\xE8\x80G"), L"AG");               // Bad UTF-8 (missing continuation byte)
	EXPECT_EQ(FromUtf8("A\xE8G"), L"AG");                   // Bad UTF-8 (missing continuation bytes)
	EXPECT_EQ(FromUtf8("A\xED\xA0\x80G"), L"AG");           // Bad UTF-8 (unpaired high surrogate U+D800)
}
