/*
* Copyright 2026 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "oned/ODTelepenReader.h"

#include "gtest/gtest.h"

using namespace ZXing;
using namespace ZXing::OneD;

static Barcode Parse(PatternRow row, std::initializer_list<ZXing::PatternType> tail = {0}, int start = 1)
{
	auto opts = ReaderOptions().formats(BarcodeFormat::Telepen);
	TelepenReader reader(opts);

	static const std::initializer_list<ZXing::PatternType> starts[3] = {
		{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3 },
		{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 3 },
		{ 0, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 3 }
	};
	static const std::initializer_list<ZXing::PatternType> stops[3] = {
		{ 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 3, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1 },
		{ 3, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1 }
	};

	row.insert(row.begin(), starts[start - 1]);
	row.insert(row.end(), stops[start - 1]);
	row.insert(row.end(), tail);

	PatternView next(row);
	std::unique_ptr<RowReader::DecodingState> state;
	return reader.decodePattern(0, next, state);
}

TEST(ODTelepenReaderTest, Alpha)
{
	{
		auto result = Parse(PatternRow({
			1, 1, 3, 1, 3, 1, 3, 3,            // A
			1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1 // checksum
		}));
		ASSERT_TRUE(result.isValid());
		EXPECT_EQ(result.format(), BarcodeFormat::TelepenAlpha);
		EXPECT_EQ(result.symbologyIdentifier(), "]B0");
		EXPECT_EQ(result.text(), "A");
	}

	{
		auto result = Parse(PatternRow({1, 1, 3, 1, 3, 1, 3, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1}), {1});
		EXPECT_EQ(result.text(), "A");
	}

	{
		auto result = Parse(PatternRow({1, 1, 3, 1, 3, 1, 3, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1}), {100});
		EXPECT_EQ(result.text(), "A");
	}

	{
		auto result = Parse(PatternRow({1, 1, 3, 1, 3, 1, 3, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1}), {100, 5});
		EXPECT_EQ(result.text(), "A");
	}
}

TEST(ODTelepenReaderTest, Numeric)
{
	auto result = Parse(PatternRow({
		1, 1, 3, 1, 1, 1, 3, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 3, 3, 3, 1,
		3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1,
		3, 3, 3, 1, 1, 1, 1, 1, 1, 1,
	}));
	ASSERT_TRUE(result.isValid());
	EXPECT_EQ(result.format(), BarcodeFormat::TelepenNumeric);
	EXPECT_EQ(result.symbologyIdentifier(), "]B1");
	EXPECT_EQ(result.text(), "466X33");
}

TEST(ODTelepenReaderTest, NumericControl)
{
	auto result = Parse(PatternRow({
		1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, // 12
		1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, // <SI> (ASCII 15)
		1, 1, 1, 3, 1, 1, 1, 1, 1, 3, 1, 1, // 34
		3, 1, 1, 1, 1, 1, 3, 1, 3, 1,       // checksum 12
	}));

	ASSERT_TRUE(result.isValid());
	EXPECT_EQ(result.format(), BarcodeFormat::TelepenNumeric);
	EXPECT_EQ(result.symbologyIdentifier(), "]B1");
	EXPECT_EQ(result.text(), "12<SI>34");
}

TEST(ODTelepenReaderTest, NumAlpha)
{
	auto result = Parse(PatternRow({
		1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, // 12
		3, 1, 3, 1, 1, 1, 3, 1, 1, 1,       // <DLE>
		1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 3, 1, // 3
		1, 1, 3, 3, 1, 1, 3, 1, 1, 1        // checksum 21
	}));

	ASSERT_TRUE(result.isValid());
	EXPECT_EQ(result.format(), BarcodeFormat::TelepenNumAlpha);
	EXPECT_EQ(result.symbologyIdentifier(), "]B2");
	EXPECT_EQ(result.text(), "123");
}

TEST(ODTelepenReaderTest, AIMAlphaNum)
{
	auto result = Parse(PatternRow({
		1, 1, 3, 1, 3, 1, 3, 3,            // A
		1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1 // checksum
	}), {0}, 3 /*start*/);

	ASSERT_TRUE(result.isValid());
	EXPECT_EQ(result.format(), BarcodeFormat::TelepenAlphaNum);
	EXPECT_EQ(result.symbologyIdentifier(), "]B4");
	EXPECT_EQ(result.text(), "A");
}

TEST(ODTelepenReaderTest, AIMNumAlpha)
{
	auto result = Parse(PatternRow({
		1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, // 12
		3, 1, 1, 3, 1, 3, 1, 1, 1, 1        // checksum (88)
	}), {0}, 2 /*start*/);

	ASSERT_TRUE(result.isValid());
	EXPECT_EQ(result.format(), BarcodeFormat::TelepenNumAlpha);
	EXPECT_EQ(result.symbologyIdentifier(), "]B2");
	EXPECT_EQ(result.text(), "12");
}

TEST(ODTelepenReaderTest, WrongChecksum)
{
	auto result = Parse(PatternRow({
		1, 1, 3, 1, 3, 1, 3, 3, // A
		1, 1, 3, 1, 3, 1, 3, 3  // wrong checksum
	}));
	EXPECT_EQ(result.format(), BarcodeFormat::TelepenAlpha);
	EXPECT_EQ(result.error().type(), Error::Checksum);
}

