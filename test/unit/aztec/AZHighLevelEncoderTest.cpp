/*
* Copyright 2017 Huy Cuong Nguyen
* Copyright 2013 ZXing authors
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

#include "aztec/AZHighLevelEncoder.h"
#include "BitArray.h"
#include "BitArrayUtility.h"
#include "StructuredAppend.h"
#include "TextDecoder.h"

#include "gtest/gtest.h"
#include <algorithm>

namespace ZXing {
	namespace Aztec {
		std::wstring GetEncodedData(const std::vector<bool>& correctedBits, const std::string& characterSet,
									std::string& symbologyIdentifier, StructuredAppendInfo& sai);
	}
}

using namespace ZXing;

namespace {
	
	std::vector<bool> ToBoolArray(const BitArray& arr) {
		std::vector<bool> result(arr.size());
		std::copy_n(arr.begin(), arr.size(), result.begin());
		return result;
	}

	std::string StripSpaces(std::string str) {
		str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
		return str;
	}

	void TestHighLevelEncodeString(const std::string& s, const std::string& expectedBits) {
		BitArray bits = Aztec::HighLevelEncoder::Encode(s);
		EXPECT_EQ(Utility::ToString(bits), StripSpaces(expectedBits)) << "highLevelEncode() failed for input string: " + s;
		std::string symbologyIdentifier;
		StructuredAppendInfo sai;
		EXPECT_EQ(TextDecoder::FromLatin1(s), Aztec::GetEncodedData(ToBoolArray(bits), "", symbologyIdentifier, sai));
	}

	void TestHighLevelEncodeString(const std::string& s, int expectedReceivedBits) {
		BitArray bits = Aztec::HighLevelEncoder::Encode(s);
		int receivedBitCount = Size(Utility::ToString(bits));
		EXPECT_EQ(receivedBitCount, expectedReceivedBits) << "highLevelEncode() failed for input string: " + s;
		std::string symbologyIdentifier;
		StructuredAppendInfo sai;
		EXPECT_EQ(TextDecoder::FromLatin1(s), Aztec::GetEncodedData(ToBoolArray(bits), "", symbologyIdentifier, sai));
	}
}

TEST(AZHighLevelEncoderTest, HighLevelEncode)
{
	TestHighLevelEncodeString("A. b.",
		// 'A'  P/S   '. ' L/L    b    D/L    '.'
		"...X. ..... ...XX XXX.. ...XX XXXX. XX.X");
	TestHighLevelEncodeString("Lorem ipsum.",
		// 'L'  L/L   'o'   'r'   'e'   'm'   ' '   'i'   'p'   's'   'u'   'm'   D/L   '.'
		".XX.X XXX.. X.... X..XX ..XX. .XXX. ....X .X.X. X...X X.X.. X.XX. .XXX. XXXX. XX.X");
	TestHighLevelEncodeString("Lo. Test 123.",
		// 'L'  L/L   'o'   P/S   '. '  U/S   'T'   'e'   's'   't'    D/L   ' '  '1'  '2'  '3'  '.'
		".XX.X XXX.. X.... ..... ...XX XXX.. X.X.X ..XX. X.X.. X.X.X  XXXX. ...X ..XX .X.. .X.X XX.X");
	TestHighLevelEncodeString("Lo...x",
		// 'L'  L/L   'o'   D/L   '.'  '.'  '.'  U/L  L/L   'x'
		".XX.X XXX.. X.... XXXX. XX.X XX.X XX.X XXX. XXX.. XX..X");
	TestHighLevelEncodeString(". x://abc/.",
		//P/S   '. '  L/L   'x'   P/S   ':'   P/S   '/'   P/S   '/'   'a'   'b'   'c'   P/S   '/'   D/L   '.'
		"..... ...XX XXX.. XX..X ..... X.X.X ..... X.X.. ..... X.X.. ...X. ...XX ..X.. ..... X.X.. XXXX. XX.X");
	// Uses Binary/Shift rather than Lower/Shift to save two bits.
	TestHighLevelEncodeString("ABCdEFG",
		//'A'   'B'   'C'   B/S    =1    'd'     'E'   'F'   'G'
		"...X. ...XX ..X.. XXXXX ....X .XX..X.. ..XX. ..XXX .X...");

	TestHighLevelEncodeString(
		// Found on an airline boarding pass.  Several stretches of Binary shift are
		// necessary to keep the bitcount so low.
		"09  UAG    ^160MEUCIQC0sYS/HpKxnBELR1uB85R20OoqqwFGa0q2uEi"
		"Ygh6utAIgLl1aBVM4EOTQtMQQYH9M2Z3Dp4qnA/fwWuQ+M8L3V8U=",
		823);
}

TEST(AZHighLevelEncoderTest, HighLevelEncodeBinary)
{
	// binary short form single byte
	TestHighLevelEncodeString(std::string("N\0N", 3),
		// 'N'  B/S    =1   '\0'      N
		".XXXX XXXXX ....X ........ .XXXX");   // Encode "N" in UPPER

	TestHighLevelEncodeString(std::string("N\0n", 3),
		// 'N'  B/S    =2   '\0'       'n'
		".XXXX XXXXX ...X. ........ .XX.XXX.");   // Encode "n" in BINARY

												// binary short form consecutive bytes
	TestHighLevelEncodeString(std::string("N\0\x80 A", 5),
		// 'N'  B/S    =2    '\0'    \u0080   ' '  'A'
		".XXXX XXXXX ...X. ........ X....... ....X ...X.");

	// binary skipping over single character
	TestHighLevelEncodeString(std::string("\0a\xff\x80 A", 6),
		// B/S  =4    '\0'      'a'     '\3ff'   '\200'   ' '   'A'
		"XXXXX ..X.. ........ .XX....X XXXXXXXX X....... ....X ...X.");

	// getting into binary mode from digit mode
	TestHighLevelEncodeString(std::string("1234\0", 5),
		//D/L   '1'  '2'  '3'  '4'  U/L  B/S    =1    \0
		"XXXX. ..XX .X.. .X.X .XX. XXX. XXXXX ....X ........"
	);

	// Create a string in which every character requires binary
	std::string sb;
	sb.reserve(3000);
	for (int i = 0; i <= 3000; i++) {
		sb.push_back((char)(128 + (i % 30)));
	}
	// Test the output generated by Binary/Switch, particularly near the
	// places where the encoding changes: 31, 62, and 2047+31=2078
	for (int i : { 1, 2, 3, 10, 29, 30, 31, 32, 33, 60, 61, 62, 63, 64, 2076, 2077, 2078, 2079, 2080, 2100 }) {
		// This is the expected length of a binary string of length "i"
		int expectedLength = (8 * i) +
			((i <= 31) ? 10 : (i <= 62) ? 20 : (i <= 2078) ? 21 : 31);
		// Verify that we are correct about the length.
		TestHighLevelEncodeString(sb.substr(0, i), expectedLength);
		if (i != 1 && i != 32 && i != 2079) {
			// The addition of an 'a' at the beginning or end gets merged into the binary code
			// in those cases where adding another binary character only adds 8 or 9 bits to the result.
			// So we exclude the border cases i=1,32,2079
			// A lower case letter at the beginning will be merged into binary mode
			TestHighLevelEncodeString('a' + sb.substr(0, i - 1), expectedLength);
			// A lower case letter at the end will also be merged into binary mode
			TestHighLevelEncodeString(sb.substr(0, i - 1) + 'a', expectedLength);
		}
		// A lower case letter at both ends will enough to latch us into LOWER.
		TestHighLevelEncodeString('a' + sb.substr(0, i) + 'b', expectedLength + 15);
	}

	sb.clear();
	for (int i = 0; i < 32; i++) {
		sb.push_back('\xA7'); // § forces binary encoding
	}
	sb[1] = 'A';
	// expect B/S(1) A B/S(30)
	TestHighLevelEncodeString(sb, 5 + 20 + 31 * 8);
	
	sb.clear();
	for (int i = 0; i < 31; i++) {
		sb.push_back('\xA7');
	}
	sb[1] = 'A';
	// expect B/S(31)
	TestHighLevelEncodeString(sb, 10 + 31 * 8);

	sb.clear();
	for (int i = 0; i < 34; i++) {
		sb.push_back('\xA7');
	}
	sb[1] = 'A';
	// expect B/S(31) B/S(3)
	TestHighLevelEncodeString(sb, 20 + 34 * 8);

	sb.clear();
	for (int i = 0; i < 64; i++) {
		sb.push_back('\xA7');
	}
	sb[30] = 'A';
	// expect B/S(64)
	TestHighLevelEncodeString(sb, 21 + 64 * 8);
}

TEST(AZHighLevelEncoderTest, HighLevelEncodePairs)
{
	// Typical usage
	TestHighLevelEncodeString("ABC. DEF\r\n",
		//  A     B    C    P/S   .<sp>   D    E     F    P/S   \r\n
		"...X. ...XX ..X.. ..... ...XX ..X.X ..XX. ..XXX ..... ...X.");

	// We should latch to PUNCT mode, rather than shift.  Also check all pairs
	TestHighLevelEncodeString("A. : , \r\n",
		// 'A'    M/L   P/L   ". "  ": "   ", " "\r\n"
		"...X. XXX.X XXXX. ...XX ..X.X  ..X.. ...X.");

	// Latch to DIGIT rather than shift to PUNCT
	TestHighLevelEncodeString("A. 1234",
		// 'A'  D/L   '.'  ' '  '1' '2'   '3'  '4'
		"...X. XXXX. XX.X ...X ..XX .X.. .X.X .X X."
	);
	// Don't bother leaving Binary Shift.
	TestHighLevelEncodeString("A\200. \200",
		// 'A'  B/S    =2    \200      "."     " "     \200
		"...X. XXXXX ..X.. X....... ..X.XXX. ..X..... X.......");
}
