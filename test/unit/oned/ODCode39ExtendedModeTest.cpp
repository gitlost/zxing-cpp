/*
* Copyright 2017 Huy Cuong Nguyen
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "BitArray.h"
#include "BitArrayUtility.h"
#include "ReaderOptions.h"
#include "Barcode.h"
#include "oned/ODCode39Reader.h"

#include "gtest/gtest.h"

using namespace ZXing;
using namespace ZXing::OneD;

static std::string Decode(std::string_view encoded)
{
	auto opts = ReaderOptions().setTryCode39ExtendedMode(true);
	BitArray row = Utility::ParseBitArray(encoded, '1');
	auto result = DecodeSingleRow(Code39Reader(opts), row.range());
	return result.text(TextMode::Plain);
}

TEST(ODCode39FullASCIITest, Decode)
{
	EXPECT_EQ(Decode(
		"00000100101101101010100100100101100101010110100100100101011010100101101001001001"
		"01010110100101101001001001010110110100101010010010010101010110010110100100100101"
		"01101011001010100100100101010110110010101001001001010101010011011010010010010101"
		"10101001101010010010010101011010011010100100100101010101100110101001001001010110"
		"10101001101001001001010101101010011010010010010101101101010010100100100101010101"
		"10100110100100100101011010110100101001001001010101101101001010010010010101010101"
		"10011010010010010101101010110010100100100101010110101100101001001001010101011011"
		"00101001001001010110010101011010010010010101001101010110100100100101011001101010"
		"10100100100101010010110101101001001001010110010110101010010010010101001101101010"
		"10100100100101101010010110101001001001010110100101101010010010010110110100101010"
		"1001001001010101100101101010010010010110101100101010010110110100000"),
		std::string("\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f", 0x20));

	EXPECT_EQ(Decode(
		"00000100101101101010011010110101001001010010110101001011010010010100101011010010"
		"11010010010100101101101001010100100101001010101100101101001001010010110101100101"
		"01001001010010101101100101010010010100101010100110110100100101001011010100110101"
		"00100101001010110100110101001001010010101011001101010010010100101101010100110100"
		"10010100101011010100110100101011011011001010110101001001010010110101101001010100"
		"11011010110100101011010110010101101101100101010101001101011011010011010101011001"
		"10101010100101101101101001011010101100101101010010010100101001101101010101001001"
		"00101011011001010101001001001010101001101101010010010010110101001101010100100100"
		"1010110100110101010010010010101011001101010010110110100000"),
		" !\"#$%&'()*+,-./0123456789:;<=>?");

	EXPECT_EQ(Decode(
		"00001001011011010101001001001010011010101101101010010110101101001011011011010010"
		"10101011001011011010110010101011011001010101010011011011010100110101011010011010"
		"10101100110101101010100110101101010011011011010100101010110100110110101101001010"
		"11011010010101010110011011010101100101011010110010101011011001011001010101101001"
		"10101011011001101010101001011010110110010110101010011011010101010010010010110101"
		"01001101010010010010101101010011010100100100101101101010010101001001001010101101"
		"001101010010010010110101101001010010110110100000"),
		"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_");

	EXPECT_EQ(Decode(
		"00000100101101101010100100100101100110101010100101001001011010100101101001010010"
		"01010110100101101001010010010110110100101010010100100101010110010110100101001001"
		"01101011001010100101001001010110110010101001010010010101010011011010010100100101"
		"10101001101010010100100101011010011010100101001001010101100110101001010010010110"
		"10101001101001010010010101101010011010010100100101101101010010100101001001010101"
		"10100110100101001001011010110100101001010010010101101101001010010100100101010101"
		"10011010010100100101101010110010100101001001010110101100101001010010010101011011"
		"00101001010010010110010101011010010100100101001101010110100101001001011001101010"
		"10100101001001010010110101101001010010010110010110101010010100100101001101101010"
		"10100100100101011011010010101001001001010101011001101010010010010110101011001010"
		"1001001001010110101100101010010010010101011011001010010110110100000"),
		"`abcdefghijklmnopqrstuvwxyz{|}~");
}
