/*
* Copyright 2022 gitlost
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

#include "DecoderResult.h"
#include "DecoderResult.h"
#include "DecodeStatus.h"
#include "Diagnostics.h"
#include "pdf417/PDFScanningDecoder.h"

#include "gtest/gtest.h"

namespace ZXing { namespace Pdf417 {
DecoderResult DecodeCodewords(std::vector<int>& codewords, int ecLevel, const std::vector<int>& erasures,
							  const std::string& characterSet);
}}

using namespace ZXing;
using namespace ZXing::Pdf417;

// Shorthand for DecodeCodewords()
static DecoderResult decode(std::vector<int>& codewords, int ecLevel = 0, const std::string& characterSet = "")
{
	std::vector<int> erasures;
	auto result = DecodeCodewords(codewords, ecLevel, erasures, characterSet);

	return result;
}

TEST(PDF417ScanningDecoderTest, BadSymbolLengthDescriptor)
{
	{
		std::vector<int> codewords = { 4, 1, 683, 675 }; // 4 should be 2

		auto result = decode(codewords);

		EXPECT_TRUE(result.isValid());
		EXPECT_EQ(result.text(), L"AB");
		EXPECT_EQ(codewords[0], 2);
	}
	{
		std::vector<int> codewords = { 1, 1, 683, 675 }; // 1 should be 2

		auto result = decode(codewords);

		EXPECT_TRUE(result.isValid());
		EXPECT_EQ(result.text(), L"AB");
		EXPECT_EQ(codewords[0], 2);
	}
}
