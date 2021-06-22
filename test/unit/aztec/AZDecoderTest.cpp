/*
* Copyright 2017 Huy Cuong Nguyen
* Copyright 2014 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "aztec/AZDecoder.h"
#include "BitMatrixIO.h"
#include "DecodeStatus.h"
#include "DecoderResult.h"
#include "Diagnostics.h"
#include "aztec/AZDetectorResult.h"

#include "gtest/gtest.h"
#include <utility>

namespace ZXing::Aztec {

std::wstring GetEncodedData(const std::vector<bool>& correctedBits, const std::string& characterSet,
							std::string& symbologyIdentifier, StructuredAppendInfo& sai);

}

using namespace ZXing;

// Shorthand to call Decode()
static DecoderResult parse(BitMatrix&& bits, bool compact, int nbDatablocks, int nbLayers)
{
	return Aztec::Decoder::Decode({{std::move(bits), {}}, compact, nbDatablocks, nbLayers, false /*readerInit*/}, "");
}

TEST(AZDecoderTest, AztecResult)
{
	auto bits = ParseBitMatrix(
		"X X X X X     X X X       X X X     X X X     \n"
		"X X X     X X X     X X X X     X X X     X X \n"
		"  X   X X       X   X   X X X X     X     X X \n"
		"  X   X X     X X     X     X   X       X   X \n"
		"  X X   X X         X               X X     X \n"
		"  X X   X X X X X X X X X X X X X X X     X   \n"
		"  X X X X X                       X   X X X   \n"
		"  X   X   X   X X X X X X X X X   X X X   X X \n"
		"  X   X X X   X               X   X X       X \n"
		"  X X   X X   X   X X X X X   X   X X X X   X \n"
		"  X X   X X   X   X       X   X   X   X X X   \n"
		"  X   X   X   X   X   X   X   X   X   X   X   \n"
		"  X X X   X   X   X       X   X   X X   X X   \n"
		"  X X X X X   X   X X X X X   X   X X X   X X \n"
		"X X   X X X   X               X   X   X X   X \n"
		"  X       X   X X X X X X X X X   X   X     X \n"
		"  X X   X X                       X X   X X   \n"
		"  X X X   X X X X X X X X X X X X X X   X X   \n"
		"X     X     X     X X   X X               X X \n"
		"X   X X X X X   X X X X X     X   X   X     X \n"
		"X X X   X X X X           X X X       X     X \n"
		"X X     X X X     X X X X     X X X     X X   \n"
		"    X X X     X X X       X X X     X X X X   \n"
		, 'X', true);

	DecoderResult result = parse(std::move(bits), false, 30, 2);
	EXPECT_EQ(result.isValid(), true);
	EXPECT_EQ(result.text(), L"88888TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT");
	EXPECT_EQ(result.rawBytes(), ByteArray({
		0xf5, 0x55, 0x55, 0x75, 0x6b, 0x5a, 0xd6, 0xb5, 0xad, 0x6b, 
		0x5a, 0xd6, 0xb5, 0xad, 0x6b, 0x5a, 0xd6, 0xb5, 0xad, 0x6b, 
		0x5a, 0xd6, 0xb0 }));
	EXPECT_EQ(result.numBits(), 180);
	EXPECT_EQ(result.symbologyIdentifier(), "]z0");
}

TEST(AZDecoderTest, DecodeTooManyErrors)
{
	auto bits = ParseBitMatrix(
		"X X . X . . . X X . . . X . . X X X . X . X X X X X . \n"
		"X X . . X X . . . . . X X . . . X X . . . X . X . . X \n"
		"X . . . X X . . X X X . X X . X X X X . X X . . X . . \n"
		". . . . X . X X . . X X . X X . X . X X X X . X . . X \n"
		"X X X . . X X X X X . . . . . X X . . . X . X . X . X \n"
		"X X . . . . . . . . X . . . X . X X X . X . . X . . . \n"
		"X X . . X . . . . . X X . . . . . X . . . . X . . X X \n"
		". . . X . X . X . . . . . X X X X X X . . . . . . X X \n"
		"X . . . X . X X X X X X . . X X X . X . X X X X X X . \n"
		"X . . X X X . X X X X X X X X X X X X X . . . X . X X \n"
		". . . . X X . . . X . . . . . . . X X . . . X X . X . \n"
		". . . X X X . . X X . X X X X X . X . . X . . . . . . \n"
		"X . . . . X . X . X . X . . . X . X . X X . X X . X X \n"
		"X . X . . X . X . X . X . X . X . X . . . . . X . X X \n"
		"X . X X X . . X . X . X . . . X . X . X X X . . . X X \n"
		"X X X X X X X X . X . X X X X X . X . X . X . X X X . \n"
		". . . . . . . X . X . . . . . . . X X X X . . . X X X \n"
		"X X . . X . . X . X X X X X X X X X X X X X . . X . X \n"
		"X X X . X X X X . . X X X X . . X . . . . X . . X X X \n"
		". . . . X . X X X . . . . X X X X . . X X X X . . . . \n"
		". . X . . X . X . . . X . X X . X X . X . . . X . X . \n"
		"X X . . X . . X X X X X X X . . X . X X X X X X X . . \n"
		"X . X X . . X X . . . . . X . . . . . . X X . X X X . \n"
		"X . . X X . . X X . X . X . . . . X . X . . X . . X . \n"
		"X . X . X . . X . X X X X X X X X . X X X X . . X X . \n"
		"X X X X . . . X . . X X X . X X . . X . . . . X X X . \n"
		"X X . X . X . . . X . X . . . . X X . X . . X X . . . \n"
		, 'X', true);

	DecoderResult result = parse(std::move(bits), true, 16, 4);
	EXPECT_EQ(result.errorCode(), DecodeStatus::FormatError);
}

TEST(AZDecoderTest, DecodeTooManyErrors2)
{
	auto bits = ParseBitMatrix(
		". X X . . X . X X . . . X . . X X X . . . X X . X X . \n"
		"X X . X X . . X . . . X X . . . X X . X X X . X . X X \n"
		". . . . X . . . X X X . X X . X X X X . X X . . X . . \n"
		"X . X X . . X . . . X X . X X . X . X X . . . . . X . \n"
		"X X . X . . X . X X . . . . . X X . . . . . X . . . X \n"
		"X . . X . . . . . . X . . . X . X X X X X X X . . . X \n"
		"X . . X X . . X . . X X . . . . . X . . . . . X X X . \n"
		". . X X X X . X . . . . . X X X X X X . . . . . . X X \n"
		"X . . . X . X X X X X X . . X X X . X . X X X X X X . \n"
		"X . . X X X . X X X X X X X X X X X X X . . . X . X X \n"
		". . . . X X . . . X . . . . . . . X X . . . X X . X . \n"
		". . . X X X . . X X . X X X X X . X . . X . . . . . . \n"
		"X . . . . X . X . X . X . . . X . X . X X . X X . X X \n"
		"X . X . . X . X . X . X . X . X . X . . . . . X . X X \n"
		"X . X X X . . X . X . X . . . X . X . X X X . . . X X \n"
		"X X X X X X X X . X . X X X X X . X . X . X . X X X . \n"
		". . . . . . . X . X . . . . . . . X X X X . . . X X X \n"
		"X X . . X . . X . X X X X X X X X X X X X X . . X . X \n"
		"X X X . X X X X . . X X X X . . X . . . . X . . X X X \n"
		". . X X X X X . X . . . . X X X X . . X X X . X . X . \n"
		". . X X . X . X . . . X . X X . X X . . . . X X . . . \n"
		"X . . . X . X . X X X X X X . . X . X X X X X . X . . \n"
		". X . . . X X X . . . . . X . . . . . X X X X X . X . \n"
		"X . . X . X X X X . X . X . . . . X . X X . X . . X . \n"
		"X . . . X X . X . X X X X X X X X . X X X X . . X X . \n"
		". X X X X . . X . . X X X . X X . . X . . . . X X X . \n"
		"X X . . . X X . . X . X . . . . X X . X . . X . X . X \n"
		, 'X', true);

	DecoderResult result = parse(std::move(bits), true, 16, 4);
	EXPECT_EQ(result.errorCode(), DecodeStatus::FormatError);
}

// Helper taking bit string to call GetEncodedData()
static std::wstring getData(std::string bitStr, std::string& symbologyIdentifier, StructuredAppendInfo& sai)
{
	std::vector<bool> correctedBits;
	correctedBits.reserve(bitStr.size());

	for (int i = 0; i < bitStr.size(); i++) {
		correctedBits.push_back(bitStr[i] == '1');
	}

	return Aztec::GetEncodedData(correctedBits, "", symbologyIdentifier, sai);
}

// Shorthand to return symbology identifier given bit string
static std::string id(const char* const bitStr)
{
	std::string symbologyIdentifier;
	StructuredAppendInfo sai;
	(void)getData(bitStr, symbologyIdentifier, sai);
	return symbologyIdentifier;
}

// Shorthand to return string result given bit string
static std::wstring data(const char* const bitStr)
{
	Diagnostics::setEnabled(true);
	std::string symbologyIdentifier;
	StructuredAppendInfo sai;
	auto result = getData(bitStr, symbologyIdentifier, sai);

	#if 0
	for (auto str : Diagnostics::get()) {
		printf("%s ", str.c_str());
	}
	printf("\n");
	#endif

	return result;
}

// Shorthand to return structured append given given bit string
static StructuredAppendInfo info(const char* const bitStr)
{
	std::string symbologyIdentifier;
	StructuredAppendInfo sai;
	(void)getData(bitStr, symbologyIdentifier, sai);
	return sai;
}

TEST(AZDecoderTest, SymbologyIdentifier)
{
	// Plain
	EXPECT_EQ(id("00010"), "]z0");
	EXPECT_EQ(data("00010"), L"A");

	{
		// GS1 ("PS FLGN(0) DL (20)01")
		const char bitStr[] = "0000000000000111100100001000100011";
		EXPECT_EQ(id(bitStr), "]z1");
		EXPECT_EQ(data(bitStr), L"2001");
	}

	{
		// AIM ("A PS FLGN(0) B")
		const char bitStr[] = "00010000000000000000011";
		EXPECT_EQ(id(bitStr), "]z2");
		EXPECT_EQ(data(bitStr), L"AB");
	}

	{
		// AIM ("DL 99 UL PS FLGN(0) B")
		const char bitStr[] = "11110101110111110000000000000000011";
		EXPECT_EQ(id(bitStr), "]z2");
		EXPECT_EQ(data(bitStr), L"99B");
	}

	{
		// Structured Append ("UL ML A D A")
		const char bitStr[] = "1110111101000100010100010";
		EXPECT_EQ(id(bitStr), "]z6");
		EXPECT_EQ(data(bitStr), L"A");
		EXPECT_EQ(info(bitStr).index, 0);
		EXPECT_EQ(info(bitStr).count, 4);
	}

	{
		// Structured Append with GS1 ("UL ML A D PS FLGN(0) DL (20)01")
		const char bitStr[] = "111011110100010001010000000000000111100100001000100011";
		EXPECT_EQ(id(bitStr), "]z7");
		EXPECT_EQ(data(bitStr), L"2001");
		EXPECT_EQ(info(bitStr).index, 0);
		EXPECT_EQ(info(bitStr).count, 4);
	}

	{
		// Structured Append with AIM ("UL ML A D A PS FLGN(0) B")
		const char bitStr[] = "1110111101000100010100010000000000000000011";
		EXPECT_EQ(id(bitStr), "]z8");
		EXPECT_EQ(data(bitStr), L"AB");
		EXPECT_EQ(info(bitStr).index, 0);
		EXPECT_EQ(info(bitStr).count, 4);
	}

	{
		// Plain with FNC1 not in first/second position ("A B PS FLGN(0) C")
		const char bitStr[] = "0001000011000000000000000100";
		EXPECT_EQ(id(bitStr), "]z0");
		EXPECT_EQ(data(bitStr), L"AB\u001DC"); // "AB<GS>C"
	}

	{
		// Plain with FNC1 not in first/second position ("A B C PS FLGN(0) D")
		const char bitStr[] = "000100001100100000000000000000101";
		EXPECT_EQ(id(bitStr), "]z0");
		EXPECT_EQ(data(bitStr), L"ABC\u001DD"); // "ABC<GS>D"
	}

	{
		// Plain with FNC1 not in first/second position ("DL 1 UL PS FLGN(0) A")
		const char bitStr[] = "1111000111110000000000000000010";
		EXPECT_EQ(id(bitStr), "]z0");
		EXPECT_EQ(data(bitStr), L"1\u001DA"); // "1<GS>D"
	}
}

// Helper taking 5-bit word array to call GetEncodedData()
static std::wstring getData(const ByteArray& bytes, std::string& symbologyIdentifier, StructuredAppendInfo& sai)
{
	std::vector<bool> correctedBits;
	correctedBits.reserve(bytes.size() * 5); // 5-bit words (assuming no digits/binary)

	for (int i = 0; i < bytes.size(); i++) {
		for (int j = 5 - 1; j >= 0; j--) {
			correctedBits.push_back((bytes[i] & (1 << j)) != 0);
		}
	}

	return Aztec::GetEncodedData(correctedBits, "", symbologyIdentifier, sai);
}

// Shorthand to return Structured Append given 5-bit word array
static StructuredAppendInfo info(ByteArray bytes)
{
	std::string symbologyIdentifier;
	StructuredAppendInfo sai;
	(void)getData(bytes, symbologyIdentifier, sai);
	return sai;
}

// Shorthand to return string result given 5-bit word array
static std::wstring data(ByteArray bytes)
{
	std::string symbologyIdentifier;
	StructuredAppendInfo sai;
	return getData(bytes, symbologyIdentifier, sai);
}

TEST(AZDecoderTest, StructuredAppend)
{
	// Null
	EXPECT_EQ(info({2}).index, -1);
	EXPECT_EQ(info({2}).count, -1);
	EXPECT_TRUE(info({2}).id.empty());
	EXPECT_EQ(data({2}), L"A");

	// Example from ISO/IEC 24778:2008 Section 8
	EXPECT_EQ(info({29, 29, 2, 5, 2}).index, 0); // AD
	EXPECT_EQ(info({29, 29, 2, 5, 2}).count, 4);
	EXPECT_TRUE(info({29, 29, 2, 5, 2}).id.empty());
	EXPECT_EQ(data({29, 29, 2, 5, 2}), L"A");

	EXPECT_EQ(info({29, 29, 3, 5, 2}).index, 1); // BD
	EXPECT_EQ(info({29, 29, 3, 5, 2}).count, 4);
	EXPECT_TRUE(info({29, 29, 3, 5, 2}).id.empty());
	EXPECT_EQ(data({29, 29, 3, 5, 2}), L"A");

	EXPECT_EQ(info({29, 29, 4, 5, 2}).index, 2); // CD
	EXPECT_EQ(info({29, 29, 4, 5, 2}).count, 4);
	EXPECT_TRUE(info({29, 29, 4, 5, 2}).id.empty());
	EXPECT_EQ(data({29, 29, 4, 5, 2}), L"A");

	EXPECT_EQ(info({29, 29, 5, 5, 2}).index, 3); // DD
	EXPECT_EQ(info({29, 29, 5, 5, 2}).count, 4);
	EXPECT_TRUE(info({29, 29, 5, 5, 2}).id.empty());
	EXPECT_EQ(data({29, 29, 5, 5, 2}), L"A");

	// Sequencing field
	EXPECT_EQ(info({29, 29, 2, 27, 2}).index, 0); // AZ
	EXPECT_EQ(info({29, 29, 2, 27, 2}).count, 26);

	EXPECT_EQ(info({29, 29, 14, 27, 2}).index, 12); // MZ
	EXPECT_EQ(info({29, 29, 14, 27, 2}).count, 26);

	EXPECT_EQ(info({29, 29, 27, 27, 2}).index, 25); // ZZ
	EXPECT_EQ(info({29, 29, 27, 27, 2}).count, 26);

	// Id
	EXPECT_EQ(info({29, 29, 1, 10, 5, 1, 2, 5, 2}).id, "ID");
	EXPECT_EQ(data({29, 29, 1, 10, 5, 1, 2, 5, 2}), L"A");

	// Invalid sequencing
	EXPECT_EQ(info({29, 29, 2, 2, 2}).index, 0); // AA
	EXPECT_EQ(info({29, 29, 2, 2, 2}).count, 0); // Count 1 so set to 0
	EXPECT_EQ(data({29, 29, 2, 2, 2}), L"A");

	EXPECT_EQ(info({29, 29, 6, 5, 2}).index, 4); // ED
	EXPECT_EQ(info({29, 29, 6, 5, 2}).count, 0); // Count 4 <= index 4 so set to 0
	EXPECT_EQ(data({29, 29, 6, 5, 2}), L"A");

	EXPECT_EQ(info({29, 29, 1, 5, 2}).index, -1); // Index < 'A'
	EXPECT_EQ(info({29, 29, 1, 5, 2}).count, -1);
	EXPECT_EQ(data({29, 29, 1, 5, 2}), L" DA"); // Bad sequencing left in result

	EXPECT_EQ(info({29, 29, 28, 5, 2}).index, -1); // Index > 'Z' (LL)
	EXPECT_EQ(info({29, 29, 28, 5, 2}).count, -1);
	EXPECT_EQ(data({29, 29, 28, 5, 2}), L"da");

	EXPECT_EQ(info({29, 29, 2, 1, 2}).index, -1); // Count < 'A'
	EXPECT_EQ(info({29, 29, 2, 1, 2}).count, -1);
	EXPECT_EQ(data({29, 29, 2, 1, 2}), L"A A");

	EXPECT_EQ(info({29, 29, 2, 28, 2}).index, -1); // Count > 'Z'
	EXPECT_EQ(info({29, 29, 2, 28, 2}).count, -1);
	EXPECT_EQ(data({29, 29, 2, 28, 2}), L"Aa");

	EXPECT_EQ(info({29, 29, 2, 5}).index, -1); // Sequencing but no data
	EXPECT_EQ(info({29, 29, 2, 5}).count, -1);
	EXPECT_EQ(data({29, 29, 2, 5}), L"AD");

	// Invalid Ids
	{
		ByteArray bytes({29, 29, 1, 10, 5, 2, 5, 2}); // No terminating space
		EXPECT_TRUE(info(bytes).id.empty());
		EXPECT_EQ(info(bytes).index, -1); // Not recognized as sequence
		EXPECT_EQ(info(bytes).count, -1);
		EXPECT_EQ(data(bytes), L" IDADA"); // Bad ID and sequencing left in result
	}
	{
		ByteArray bytes({29, 29, 1, 1, 2, 5, 2}); // Blank
		EXPECT_TRUE(info(bytes).id.empty());
		EXPECT_EQ(info(bytes).index, 0); // Recognized as sequence
		EXPECT_EQ(info(bytes).count, 4);
		EXPECT_EQ(data(bytes), L"A");
	}
	{
		ByteArray bytes({29, 29, 1, 10, 1, 5, 1, 2, 5, 2}); // Space in "I D"
		EXPECT_TRUE(info(bytes).id.empty());
		EXPECT_EQ(info(bytes).index, -1); // Not recognized as sequence as sequence count invalid (space)
		EXPECT_EQ(info(bytes).count, -1);
		EXPECT_EQ(data(bytes), L" I D ADA"); // Bad ID and sequencing left in result
	}
	{
		ByteArray bytes({29, 29, 1, 10, 1, 2, 5, 1, 2, 5, 2}); // "I AD" (happens to have valid sequencing at end)
		EXPECT_EQ(info(bytes).id, "I");
		EXPECT_EQ(info(bytes).index, 0);
		EXPECT_EQ(info(bytes).count, 4);
		EXPECT_EQ(data(bytes), L" ADA"); // Trailing space and "real" sequencing left in result
	}
}
