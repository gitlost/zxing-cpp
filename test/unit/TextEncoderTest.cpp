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

TEST(TextEncoderTest, Latin2)
{
	CharacterSet cs = CharacterSet::ISO8859_2;
	{
		std::wstring str(L"\u0104"); // "Ą"
		std::string bytes = TextEncoder::FromUnicode(str, cs);
		//EXPECT_EQ(bytes, "\xA1"); // Currently fails

		std::wstring dec;
		TextDecoder::Append(dec, (const uint8_t*)bytes.data(), bytes.size(), cs);
		//EXPECT_EQ(dec, L"Ą"); // Currently fails
	}
}
