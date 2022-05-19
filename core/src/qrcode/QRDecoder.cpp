/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
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

#include "QRDecoder.h"

#include "BitMatrix.h"
#include "BitSource.h"
#include "CharacterSet.h"
#include "CharacterSetECI.h"
#include "Content.h"
#include "DecodeStatus.h"
#include "DecoderResult.h"
#include "Diagnostics.h"
#include "GenericGF.h"
#include "QRBitMatrixParser.h"
#include "QRCodecMode.h"
#include "QRDataBlock.h"
#include "QRFormatInformation.h"
#include "QRVersion.h"
#include "ReedSolomonDecoder.h"
#include "StructuredAppend.h"
#include "TextDecoder.h"
#include "ZXContainerAlgorithms.h"
#include "ZXTestSupport.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ZXing::QRCode {

/**
* <p>Given data and error-correction codewords received, possibly corrupted by errors, attempts to
* correct the errors in-place using Reed-Solomon error correction.</p>
*
* @param codewordBytes data and error correction codewords
* @param numDataCodewords number of codewords that are data bytes
* @throws ChecksumException if error correction fails
*/
static bool CorrectErrors(ByteArray& codewordBytes, int numDataCodewords)
{
	// First read into an array of ints
	std::vector<int> codewordsInts(codewordBytes.begin(), codewordBytes.end());

	int numECCodewords = Size(codewordBytes) - numDataCodewords;
	if (!ReedSolomonDecode(GenericGF::QRCodeField256(), codewordsInts, numECCodewords))
		return false;

	// Copy back into array of bytes -- only need to worry about the bytes that were data
	// We don't care about errors in the error-correction codewords
	std::copy_n(codewordsInts.begin(), numDataCodewords, codewordBytes.begin());
	return true;
}


/**
* See specification GBT 18284-2000
*/
static void DecodeHanziSegment(BitSource& bits, int count, Content& content)
{
	Diagnostics::fmt("HAN(%d)", count);
	// Each character will require 2 bytes. Read the characters as 2-byte pairs
	// and decode as GB2312 afterwards
	content.SetSegmentType(SegmentType::HANZI);
	while (count > 0) {
		// Each 13 bits encodes a 2-byte character
		int twoBytes = bits.readBits(13);
		int assembledTwoBytes = ((twoBytes / 0x060) << 8) | (twoBytes % 0x060);
		if (assembledTwoBytes < 0x00A00) {
			// In the 0xA1A1 to 0xAAFE range
			assembledTwoBytes += 0x0A1A1;
		} else {
			// In the 0xB0A1 to 0xFAFE range
			assembledTwoBytes += 0x0A6A1;
		}
		content.Append(static_cast<uint8_t>((assembledTwoBytes >> 8) & 0xFF));
		content.Append(static_cast<uint8_t>(assembledTwoBytes & 0xFF));
		count--;
	}
}

static void DecodeKanjiSegment(BitSource& bits, int count, Content& content)
{
	Diagnostics::fmt("KAN(%d)", count);
	// Each character will require 2 bytes. Read the characters as 2-byte pairs
	// and decode as Shift_JIS afterwards
	content.SetSegmentType(SegmentType::KANJI);
	while (count > 0) {
		// Each 13 bits encodes a 2-byte character
		int twoBytes = bits.readBits(13);
		int assembledTwoBytes = ((twoBytes / 0x0C0) << 8) | (twoBytes % 0x0C0);
		if (assembledTwoBytes < 0x01F00) {
			// In the 0x8140 to 0x9FFC range
			assembledTwoBytes += 0x08140;
		} else {
			// In the 0xE040 to 0xEBBF range
			assembledTwoBytes += 0x0C140;
		}
		content.Append(static_cast<uint8_t>(assembledTwoBytes >> 8));
		content.Append(static_cast<uint8_t>(assembledTwoBytes));
		count--;
	}
}

static void DecodeByteSegment(BitSource& bits, int count, Content& content)
{
	Diagnostics::fmt("BYTE(%d)", count);
	ByteArray readBytes(count);
	for (int i = 0; i < count; i++)
		readBytes[i] = static_cast<uint8_t>(bits.readBits(8));

	content.SetSegmentType(SegmentType::BYTE);
	content.Append(readBytes.data(), Size(readBytes));
}

static char ToAlphaNumericChar(int value)
{
	/**
	* See ISO 18004:2006, 6.4.4 Table 5
	*/
	static const char ALPHANUMERIC_CHARS[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
		'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		' ', '$', '%', '*', '+', '-', '.', '/', ':'
	};

	if (value < 0 || value >= Size(ALPHANUMERIC_CHARS))
		throw std::out_of_range("ToAlphaNumericChar: out of range");

	return ALPHANUMERIC_CHARS[value];
}

static void DecodeAlphanumericSegment(BitSource& bits, int count, bool fc1InEffect, Content& content)
{
	Diagnostics::fmt("ANUM(%d)", count);
	// Read two characters at a time
	std::string buffer;
	while (count > 1) {
		int nextTwoCharsBits = bits.readBits(11);
		buffer += ToAlphaNumericChar(nextTwoCharsBits / 45);
		buffer += ToAlphaNumericChar(nextTwoCharsBits % 45);
		count -= 2;
	}
	if (count == 1) {
		// special case: one character left
		buffer += ToAlphaNumericChar(bits.readBits(6));
	}
	// See section 6.4.8.1, 6.4.8.2
	if (fc1InEffect) {
		// We need to massage the result a bit if in an FNC1 mode:
		for (size_t i = 0; i < buffer.length(); i++) {
			if (buffer[i] == '%') {
				if (i < buffer.length() - 1 && buffer[i + 1] == '%') {
					// %% is rendered as %
					buffer.erase(i + 1);
				} else {
					// In alpha mode, % should be converted to FNC1 separator 0x1D
					buffer[i] = static_cast<char>(0x1D);
				}
			}
		}
	}
	content.SetSegmentType(SegmentType::ASCII);
	content.Append(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
}

static void DecodeNumericSegment(BitSource& bits, int count, Content& content)
{
	Diagnostics::fmt("NUM(%d)", count);
	// Read three digits at a time
	content.SetSegmentType(SegmentType::ASCII);
	while (count >= 3) {
		// Each 10 bits encodes three digits
		int threeDigitsBits = bits.readBits(10);
		if (threeDigitsBits >= 1000)
			throw std::runtime_error("Invalid value in numeric segment");

		content.Append(ToAlphaNumericChar(threeDigitsBits / 100));
		content.Append(ToAlphaNumericChar((threeDigitsBits / 10) % 10));
		content.Append(ToAlphaNumericChar(threeDigitsBits % 10));
		count -= 3;
	}

	if (count == 2) {
		// Two digits left over to read, encoded in 7 bits
		int twoDigitsBits = bits.readBits(7);
		if (twoDigitsBits >= 100)
			throw std::runtime_error("Invalid value in numeric segment");

		content.Append(ToAlphaNumericChar(twoDigitsBits / 10));
		content.Append(ToAlphaNumericChar(twoDigitsBits % 10));
	}
	else if (count == 1) {
		// One digit left over to read
		int digitBits = bits.readBits(4);
		if (digitBits >= 10)
			throw std::runtime_error("Invalid value in numeric segment");

		content.Append(ToAlphaNumericChar(digitBits));
	}
}

static DecodeStatus ParseECIValue(BitSource& bits, int& outValue)
{
	int firstByte = bits.readBits(8);
	if ((firstByte & 0x80) == 0) {
		// just one byte
		outValue = firstByte & 0x7F;
		return DecodeStatus::NoError;
	}
	if ((firstByte & 0xC0) == 0x80) {
		// two bytes
		int secondByte = bits.readBits(8);
		outValue = ((firstByte & 0x3F) << 8) | secondByte;
		return DecodeStatus::NoError;
	}
	if ((firstByte & 0xE0) == 0xC0) {
		// three bytes
		int secondThirdBytes = bits.readBits(16);
		outValue = ((firstByte & 0x1F) << 16) | secondThirdBytes;
		return DecodeStatus::NoError;
	}
	return DecodeStatus::FormatError;
}

/**
 * QR codes encode mode indicators and terminator codes into a constant bit length of 4.
 * Micro QR codes have terminator codes that vary in bit length but are always longer than
 * the mode indicators.
 * M1 - 0 length mode code, 3 bits terminator code
 * M2 - 1 bit mode code, 5 bits terminator code
 * M3 - 2 bit mode code, 7 bits terminator code
 * M4 - 3 bit mode code, 9 bits terminator code
 * IsTerminator peaks into the bit stream to see if the current position is at the start of
 * a terminator code.  If true, then the decoding can finish. If false, then the decoding
 * can read off the next mode code.
 *
 * See ISO 18004:2006, 6.4.1 Table 2
 *
 * @param bits the stream of bits that might have a terminator code
 * @param version the QR or micro QR code version
 */
bool IsTerminator(const BitSource& bits, const Version& version)
{
	const int bitsRequired = TerminatorBitsLength(version);
	const int bitsAvailable = std::min(bits.available(), bitsRequired);
	return bits.peakBits(bitsAvailable) == 0;
}

/**
* <p>QR Codes can encode text as bits in one of several modes, and can use multiple modes
* in one QR Code. This method decodes the bits back into text.</p>
*
* <p>See ISO 18004:2006, 6.4.3 - 6.4.7</p>
*/
ZXING_EXPORT_TEST_ONLY
DecoderResult DecodeBitStream(ByteArray&& bytes, const Version& version, ErrorCorrectionLevel ecLevel,
							  const std::string& hintedCharset)
{
	BitSource bits(bytes);
	std::wstring resultEncoded;
	Content content(BarcodeFormat::QRCode, hintedCharset, CharacterSet::Unknown);
	int symbologyIdModifier = 1; // ISO/IEC 18004:2015 Annex F Table F.1
	int appIndValue = -1; // ISO/IEC 18004:2015 7.4.8.3 AIM Application Indicator (FNC1 in second position)
	StructuredAppendInfo structuredAppend;
	static const int GB2312_SUBSET = 1;
	const int modeBitLength = CodecModeBitsLength(version);
	const int minimumBitsRequired = modeBitLength + CharacterCountBits(CodecMode::NUMERIC, version);

	try
	{
		bool fc1InEffect = false;
		CodecMode mode;
		do {
			// While still another segment to read...
			if (bits.available() < minimumBitsRequired || IsTerminator(bits, version)) {
				// OK, assume we're done. Really, a TERMINATOR mode should have been recorded here
				mode = CodecMode::TERMINATOR;
				Diagnostics::put("AVAIL(<4)");
			} else if (modeBitLength == 0) {
				// MicroQRCode version 1 is always NUMERIC and modeBitLength is 0
				mode = CodecMode::NUMERIC;
			} else {
				mode = CodecModeForBits(bits.readBits(modeBitLength), version.isMicroQRCode());
			}
			switch (mode) {
			case CodecMode::TERMINATOR:
				Diagnostics::put("TERM");
				break;
			case CodecMode::FNC1_FIRST_POSITION:
				content.SetGS1();
				fc1InEffect = true; // In Alphanumeric mode undouble doubled percents and treat single percent as <GS>
				// As converting character set ECIs ourselves and ignoring/skipping non-character ECIs, not using
				// modifiers that indicate ECI protocol (ISO/IEC 18004:2015 Annex F Table F.1)
				symbologyIdModifier = 3;
				Diagnostics::put("FNC1(1st)");
				break;
			case CodecMode::FNC1_SECOND_POSITION:
				fc1InEffect = true; // As above
				symbologyIdModifier = 5; // As above
				// ISO/IEC 18004:2015 7.4.8.3 AIM Application Indicator "00-99" or "A-Za-z"
				appIndValue = bits.readBits(8); // Number 00-99 or ASCII value + 100; prefixed to data below
				Diagnostics::fmt("FNC1(2nd,%d)", appIndValue);
				break;
			case CodecMode::STRUCTURED_APPEND:
				// sequence number and parity is added later to the result metadata
				// Read next 4 bits of index, 4 bits of symbol count, and 8 bits of parity data, then continue
				structuredAppend.index = bits.readBits(4);
				structuredAppend.count = bits.readBits(4) + 1;
				structuredAppend.id    = std::to_string(bits.readBits(8));
				Diagnostics::fmt("SAI(%d,%d,%s)", structuredAppend.index, structuredAppend.count, structuredAppend.id.c_str());
				break;
			case CodecMode::ECI: {
				// Count doesn't apply to ECI
				int value = 0;
				auto status = ParseECIValue(bits, value);
				Diagnostics::fmt("ECI(%d)", value);
				if (StatusIsError(status)) {
					return status;
				}
				if (!content.SetECI(value)) {
					return DecodeStatus::FormatError;
				}
				break;
			}
			case CodecMode::HANZI: {
				// First handle Hanzi mode which does not start with character count
				// chinese mode contains a sub set indicator right after mode indicator
				int subset = bits.readBits(4);
				int countHanzi = bits.readBits(CharacterCountBits(mode, version));
				if (subset == GB2312_SUBSET)
					DecodeHanziSegment(bits, countHanzi, content);
				break;
			}
			default: {
				// "Normal" QR code modes:
				// How many characters will follow, encoded in this mode?
				int count = bits.readBits(CharacterCountBits(mode, version));
				switch (mode) {
				case CodecMode::NUMERIC:      DecodeNumericSegment(bits, count, content); break;
				case CodecMode::ALPHANUMERIC: DecodeAlphanumericSegment(bits, count, fc1InEffect, content); break;
				case CodecMode::BYTE:         DecodeByteSegment(bits, count, content); break;
				case CodecMode::KANJI:        DecodeKanjiSegment(bits, count, content); break;
				default:                      Diagnostics::put("FormatError"); return DecodeStatus::FormatError;
				}
				break;
			}
			}
		} while (mode != CodecMode::TERMINATOR);
	}
	catch (const std::exception &)
	{
		// from readBits() calls
		return DecodeStatus::FormatError;
	}

	if (appIndValue >= 0) {
		uint8_t buf[2];
		if (appIndValue < 100) { // "00-99"
			buf[0] = appIndValue / 10 + '0';
			buf[1] = appIndValue % 10 + '0';
			content.Prepend(buf, 2);
		}
		else if ((appIndValue >= 165 && appIndValue <= 190) || (appIndValue >= 197 && appIndValue <= 222)) { // "A-Za-z"
			buf[0] = appIndValue - 100;
			content.Prepend(buf, 1);
		}
		else {
			Diagnostics::fmt("BadAppInd(%d)", appIndValue);
			return DecodeStatus::FormatError;
        }
	}

	content.Finalize();

	std::string symbologyIdentifier("]Q" + std::to_string(symbologyIdModifier));

	return DecoderResult(std::move(bytes), std::move(content))
		.setEcLevel(ToString(ecLevel))
		.setSymbologyIdentifier(std::move(symbologyIdentifier))
		.setStructuredAppend(structuredAppend);
}

static DecoderResult DoDecode(const BitMatrix& bits, const Version& version, const std::string& hintedCharset, bool mirrored)
{
	auto formatInfo = ReadFormatInformation(bits, mirrored, version.isMicroQRCode());
	if (!formatInfo.isValid())
		return DecodeStatus::FormatError;
	Diagnostics::fmt("  Dimensions: %dx%d (HxW) (Version %d)\n", bits.height(), bits.width(), (bits.width() - 17) / 4);
	Diagnostics::fmt("  Mask:       %d\n", formatInfo.dataMask());

	// Read codewords
	ByteArray codewords = ReadCodewords(bits, version, formatInfo, mirrored);
	if (codewords.empty())
		return DecodeStatus::FormatError;

	// Separate into data blocks
	std::vector<DataBlock> dataBlocks = DataBlock::GetDataBlocks(codewords, version, formatInfo.errorCorrectionLevel());
	if (dataBlocks.empty())
		return DecodeStatus::FormatError;

	// Count total number of data bytes
	const auto op = [](auto totalBytes, const auto& dataBlock){ return totalBytes + dataBlock.numDataCodewords();};
	const auto totalBytes = std::accumulate(std::begin(dataBlocks), std::end(dataBlocks), int{}, op);
	ByteArray resultBytes(totalBytes);
	auto resultIterator = resultBytes.begin();

	// Error-correct and copy data blocks together into a stream of bytes
	for (auto& dataBlock : dataBlocks)
	{
		ByteArray& codewordBytes = dataBlock.codewords();
		int numDataCodewords = dataBlock.numDataCodewords();

		if (!CorrectErrors(codewordBytes, numDataCodewords))
			return DecodeStatus::ChecksumError;

		resultIterator = std::copy_n(codewordBytes.begin(), numDataCodewords, resultIterator);
	}

	// Decode the contents of that stream of bytes
	Diagnostics::put("  Decode:     ");
	return DecodeBitStream(std::move(resultBytes), version, formatInfo.errorCorrectionLevel(), hintedCharset);
}

DecoderResult Decode(const BitMatrix& bits, const std::string& hintedCharset)
{
	const Version* version = ReadVersion(bits);
	if (!version)
		return DecodeStatus::FormatError;

	auto res = DoDecode(bits, *version, hintedCharset, false);
	if (res.isValid())
		return res;

	if (auto resMirrored = DoDecode(bits, *version, hintedCharset, true); resMirrored.isValid()) {
		resMirrored.setIsMirrored(true);
		return resMirrored;
	}

	return res;
}

} // namespace ZXing::QRCode
