/*
* Copyright 2021 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "CharacterSetECI.h"
#include "ECI.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ZXing;
using namespace ZXing::CharacterSetECI;
using namespace testing;

TEST(CharacterSetECITest, Charset2ECI)
{
	EXPECT_EQ(ToInt(ToECI(CharacterSet::ISO8859_1)), 3);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::ISO8859_2)), 4);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UTF16BE)), 25);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::ASCII)), 27);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::GB2312)), 29);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::EUC_KR)), 30);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::GBK)), 31);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::GB18030)), 32);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UTF16LE)), 33);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UTF32BE)), 34);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::UTF32LE)), 35);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::BINARY)), 899);
	EXPECT_EQ(ToInt(ToECI(CharacterSet::Unknown)), -1);
}
