/*
* Copyright 2021 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "CharacterSet.h"
#include "ECI.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ZXing;
using namespace testing;

TEST(CharacterSetECITest, Charset2ECI)
{
	EXPECT_EQ(ToInt(ToECI(CharacterSet::ISO8859_1)), 3);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::ISO8859_2)), 4);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UnicodeBig)), 25);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::ASCII)), 27);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::GB2312)), 29);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::EUC_KR)), 30);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::GBK)), 31);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::GB18030)), 32);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UTF16LE)), 33);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UTF32BE)), 34);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UTF32LE)), 35);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::ISO646_Inv)), 170);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::BINARY)), 899);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::Unknown)), -1);
}

TEST(CharacterSetECITest, CharacterSetFromString)
{
	EXPECT_EQ(CharacterSet::ISO8859_1, CharacterSetFromString("ISO-8859-1"));
	EXPECT_EQ(CharacterSet::ISO8859_1, CharacterSetFromString("ISO8859_1"));
	EXPECT_EQ(CharacterSet::ISO8859_1, CharacterSetFromString("ISO 8859-1"));
	EXPECT_EQ(CharacterSet::ISO8859_1, CharacterSetFromString("iso88591"));
	EXPECT_EQ(CharacterSet::UnicodeBig, CharacterSetFromString("UTF16BE"));
	EXPECT_EQ(CharacterSet::UTF8, CharacterSetFromString("UTF8"));
	EXPECT_EQ(CharacterSet::GBK, CharacterSetFromString("GBK"));
	EXPECT_EQ(CharacterSet::EUC_KR, CharacterSetFromString("EUC_KR"));
	EXPECT_EQ(CharacterSet::UTF16LE, CharacterSetFromString("UTF16LE"));
	EXPECT_EQ(CharacterSet::UTF16LE, CharacterSetFromString("UTF-16LE"));
	EXPECT_EQ(CharacterSet::UTF32BE, CharacterSetFromString("UTF32BE"));
	EXPECT_EQ(CharacterSet::UTF32BE, CharacterSetFromString("UTF-32BE"));
	EXPECT_EQ(CharacterSet::UTF32LE, CharacterSetFromString("UTF32LE"));
	EXPECT_EQ(CharacterSet::UTF32LE, CharacterSetFromString("UTF-32LE"));
	EXPECT_EQ(CharacterSet::ISO646_Inv, CharacterSetFromString("ISO646_Inv"));
	EXPECT_EQ(CharacterSet::Unknown, CharacterSetFromString("invalid-name"));
}
