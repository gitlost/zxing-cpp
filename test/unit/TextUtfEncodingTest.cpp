/*
* Copyright 2021 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "TextUtfEncoding.h"
#include "Utf.h"

#include "gtest/gtest.h"
#include <vector>

using namespace ZXing;

TEST(TextUtfEncodingTest, ToUtf8)
{
	EXPECT_EQ(ToUtf8(L"\u00B6\u0416"), u8"\u00B6\u0416");

	EXPECT_EQ(ToUtf8(L"\u00B6\xD800\u0416"), "\xC2\xB6\xED\xA0\x80\xD0\x96"); // Converts unpaired surrogate
}

TEST(TextUtfEncodingTest, EscapeNonGraphical)
{
	EXPECT_EQ(EscapeNonGraphical(u8"\u00B6\u0416"), u8"\u00B6\u0416");
	EXPECT_EQ(EscapeNonGraphical(u8"\x01\x1F\x7F"), "<SOH><US><DEL>");
	EXPECT_EQ(EscapeNonGraphical(u8"\u0080\u009F"), "<U+80><U+9F>");
	EXPECT_EQ(EscapeNonGraphical(u8"\u00A0"), "<U+A0>"); // NO-BREAK space (nbsp)
	EXPECT_EQ(EscapeNonGraphical(u8"\u2007"), "<U+2007>"); // NO-BREAK space (numsp)
	EXPECT_EQ(EscapeNonGraphical(u8"\uFFEF"), "<U+FFEF>"); // Was NO-BREAK space but now isn't (BOM)
	EXPECT_EQ(EscapeNonGraphical(u8"\u2000"), "<U+2000>"); // Space char (nqsp)
	EXPECT_EQ(EscapeNonGraphical(u8"\uFFFD"), u8"\uFFFD");
	EXPECT_EQ(EscapeNonGraphical(u8"\uFFFF"), "<U+FFFF>");
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

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u00B6\u0416", angleEscape), u8"\u00B6\u0416");
	EXPECT_EQ(EscapeNonGraphical(u8"\u00B6\u0416"), u8"\u00B6\u0416");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u2602", angleEscape), u8"\u2602");
	EXPECT_EQ(EscapeNonGraphical(u8"\u2602"), u8"\u2602");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\x01\x1F\x7F", angleEscape), "<SOH><US><DEL>");
	EXPECT_EQ(EscapeNonGraphical(u8"\x01\x1F\x7F"), "<SOH><US><DEL>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\x80\x9F", angleEscape), "<U+80><U+9F>");
	EXPECT_EQ(EscapeNonGraphical(u8"\u0080\u009F"), "<U+80><U+9F>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\xA0", angleEscape), "<U+A0>"); // NO-BREAK space (nbsp)
	EXPECT_EQ(EscapeNonGraphical(u8"\u00A0"), "<U+A0>");
	EXPECT_EQ(EscapeNonGraphical("\xA0"), "<0xA0>"); // Not UTF-8

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\x2007", angleEscape), "<U+2007>"); // NO-BREAK space (numsp)
	EXPECT_EQ(EscapeNonGraphical(u8"\u2007"), "<U+2007>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\xFFEF", angleEscape), "<U+FFEF>"); // Was NO-BREAK space but now isn't (BOM)
	EXPECT_EQ(EscapeNonGraphical(u8"\uFFEF"), "<U+FFEF>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u0100", angleEscape), u8"\u0100");
	EXPECT_EQ(EscapeNonGraphical(u8"\u0100"), u8"\u0100");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u1000", angleEscape), u8"\u1000");
	EXPECT_EQ(EscapeNonGraphical(u8"\u1000"), u8"\u1000");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\u2000", angleEscape), "<U+2000>"); // Space char (nqsp)
	EXPECT_EQ(EscapeNonGraphical(u8"\u2000"), "<U+2000>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\uFFFD", angleEscape), u8"\uFFFD");
	EXPECT_EQ(EscapeNonGraphical(u8"\uFFFD"), u8"\uFFFD");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\uFFFF", angleEscape), "<U+FFFF>");
	EXPECT_EQ(EscapeNonGraphical(u8"\uFFFF"), "<U+FFFF>");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\U00010000", angleEscape), u8"\U00010000");
	EXPECT_EQ(EscapeNonGraphical(u8"\U00010000"), u8"\U00010000");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"\xD800Z", angleEscape), "<0xED><0xA0><0x80>Z"); // Unpaired high surrogate
	EXPECT_EQ(EscapeNonGraphical("\xED\xA0\x80Z"), "<0xED><0xA0><0x80>Z");

	EXPECT_EQ(TextUtfEncoding::ToUtf8(L"A\xDC00", angleEscape), "A<0xED><0xB0><0x80>"); // Unpaired low surrogate
	EXPECT_EQ(EscapeNonGraphical("A\xED\xB0\x80"), "A<0xED><0xB0><0x80>");

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

TEST(TextUtfEncodingTest, FromUtf8)
{
	EXPECT_EQ(FromUtf8(u8"\U00010000"), L"\U00010000");
	EXPECT_EQ(FromUtf8(u8"\U00010FFF"), L"\U00010FFF");
	EXPECT_EQ(FromUtf8("A\xE8\x80\xBFG"), L"A\u803FG"); // U+803F

	EXPECT_EQ(FromUtf8("A\xE8\x80\xBF\x80G"), L"A\u803FG"); // Bad UTF-8 (extra continuation byte)
	EXPECT_EQ(FromUtf8("A\xE8\x80\xC0G"), L"AG");           // Bad UTF-8 (non-continuation byte)
	EXPECT_EQ(FromUtf8("A\xE8\x80G"), L"AG");               // Bad UTF-8 (missing continuation byte)
	EXPECT_EQ(FromUtf8("A\xE8G"), L"AG");                   // Bad UTF-8 (missing continuation bytes)
	EXPECT_EQ(FromUtf8("A\xED\xA0\x80G"), L"AG");           // Bad UTF-8 (unpaired high surrogate U+D800)
}
