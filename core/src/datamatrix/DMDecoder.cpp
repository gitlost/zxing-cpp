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

#include "DMDecoder.h"

#include "BitMatrix.h"
#include "BitSource.h"
#include "CharacterSetECI.h"
#include "DMBitLayout.h"
#include "DMDataBlock.h"
#include "DMVersion.h"
#include "DecodeStatus.h"
#include "DecoderResult.h"
#include "Diagnostics.h"
#include "GenericGF.h"
#include "ReedSolomonDecoder.h"
#include "TextDecoder.h"
#include "ZXContainerAlgorithms.h"
#include "ZXTestSupport.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ZXing::DataMatrix {

/**
* <p>Data Matrix Codes can encode text as bits in one of several modes, and can use multiple modes
* in one Data Matrix Code. This class decodes the bits back into text.</p>
*
* <p>See ISO 16022:2006, 5.2.1 - 5.2.9.2</p>
*
* @author bbrown@google.com (Brian Brown)
* @author Sean Owen
*/
namespace DecodedBitStreamParser {

enum Mode
{
	FORMAT_ERROR,
	DONE, // reached end of code word sequence or a PAD codeword
	ASCII_ENCODE,
	C40_ENCODE,
	TEXT_ENCODE,
	ANSIX12_ENCODE,
	EDIFACT_ENCODE,
	BASE256_ENCODE
};

/**
* See ISO 16022:2006, Annex C Table C.1
* The C40 Basic Character Set (*'s used for placeholders for the shift values)
*/
static const char C40_BASIC_SET_CHARS[] = {
	'*', '*', '*', ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

static const char C40_SHIFT2_SET_CHARS[] = {
	'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*',  '+', ',', '-', '.',
	'/', ':', ';', '<', '=', '>', '?',  '@', '[', '\\', ']', '^', '_', 29 // FNC1->29
};

/**
* See ISO 16022:2006, Annex C Table C.2
* The Text Basic Character Set (*'s used for placeholders for the shift values)
*/
static const char TEXT_BASIC_SET_CHARS[] = {
	'*', '*', '*', ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};

// Shift 2 for Text is the same encoding as C40
#define TEXT_SHIFT2_SET_CHARS C40_SHIFT2_SET_CHARS

static const char TEXT_SHIFT3_SET_CHARS[] = {
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O',  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '~', 127
};

// Decoding state
struct State
{
	CharacterSet encoding;
	int symbologyIdModifier = 1; // ECC 200 (ISO 16022:2006 Annex N Table N.1)
	struct StructuredAppendInfo sai;
	bool readerInit = false;
	bool firstCodeword = true;
	int firstFNC1Position = 1;
};

struct Shift128
{
	bool set = false;
	char operator()(int val) { return static_cast<char>(val + std::exchange(set, false) * 128); }
};

/**
* See ISO 16022:2006, 5.4.1, Table 6
*/
static int ParseECIValue(BitSource& bits, int& position)
{
	int firstByte = bits.readBits(8);
	position++;
	if (firstByte <= 127)
		return firstByte - 1;

	int secondByte = bits.readBits(8);
	position++;
	if (firstByte <= 191)
		return (firstByte - 128) * 254 + 127 + secondByte - 1;

	int thirdByte = bits.readBits(8);
	position++;

	return (firstByte - 192) * 64516 + 16383 + (secondByte - 1) * 254 + thirdByte - 1;
}

/**
* See ISO 16022:2006, 5.6
*/
static void ParseStructuredAppend(BitSource& bits, StructuredAppendInfo& sai, int& position)
{
	// 5.6.2 Table 8
	int symbolSequenceIndicator = bits.readBits(8);
	position++;
	sai.index = symbolSequenceIndicator >> 4;
	sai.count = 17 - (symbolSequenceIndicator & 0x0F); // 2-16 permitted, 17 invalid

	if (sai.count == 17 || sai.count <= sai.index) { // If info doesn't make sense
		Diagnostics::fmt("SASeqIndWarn(0x%X)", symbolSequenceIndicator);
		sai.count = 0; // Choose to mark count as unknown
	}

	int fileId1 = bits.readBits(8); // File identification 1
	position++;
	int fileId2 = bits.readBits(8); // File identification 2
	position++;

	// There's no conversion method or meaning given to the 2 file id codewords in Section 5.6.3, apart from
	// saying that each value should be 1-254. Choosing here to represent them as base 256.
	sai.id = std::to_string((fileId1 << 8) | fileId2);
}

/**
* See ISO 16022:2006, 5.2.3 and Annex C, Table C.2
*/
static Mode DecodeAsciiSegment(BitSource& bits, std::string& result, std::string& resultTrailer,
							   std::wstring& resultEncoded, State& state)
{
	Shift128 upperShift;

	for (int position = 1; bits.available() >= 8; position++) {
		int oneByte = bits.readBits(8);
		switch (oneByte) {
		case 0:
			Diagnostics::put("ASCError(0)");
			return Mode::FORMAT_ERROR;
		case 129: // Pad
			Diagnostics::put("PAD");
			return Mode::DONE;
		case 230: // Latch to C40 encodation
			Diagnostics::put("C40");
			return Mode::C40_ENCODE;
		case 231: // Latch to Base 256 encodation
			Diagnostics::put("BAS");
			return Mode::BASE256_ENCODE;
		case 232: // FNC1
			if (position == state.firstFNC1Position || position == state.firstFNC1Position + 1) {
				// As converting character set ECIs ourselves and ignoring/skipping non-character ECIs, not using
				// modifiers that indicate ECI protocol (ISO 16022:2006 Annex N Table N.1, ISO 21471:2020 Annex G Table G.1)

				// Only recognizing an FNC1 as first/second by codeword position (aka symbol character position), not
				// by decoded character position, i.e. not recognizing a C40/Text encoded FNC1 (which requires a latch
				// and a shift)
				if (position == state.firstFNC1Position) {
					state.symbologyIdModifier = 2; // GS1
					Diagnostics::put("FNC1(GS1)");
				} else {
					state.symbologyIdModifier = 3; // AIM
					// Note no AIM Application Indicator format defined, ISO 16022:2006 11.2
					Diagnostics::put("FNC1(2ndPos)");
				}
			} else {
				result.push_back((char)29); // translate as ASCII 29 <GS>
				Diagnostics::put("FNC1(29)");
			}
			break;
		case 233: // Structured Append
			if (state.firstCodeword) { // Must be first ISO 16022:2006 5.6.1
				ParseStructuredAppend(bits, state.sai, position);
				state.firstFNC1Position = 5;
				Diagnostics::fmt("SA(%d,%d,%s)", state.sai.index, state.sai.count, state.sai.id.c_str());
			} else
				return Mode::FORMAT_ERROR;
			break;
		case 234: // Reader Programming
			if (state.firstCodeword) // Must be first ISO 16022:2006 5.2.4.9
				state.readerInit = true;
			else
				return Mode::FORMAT_ERROR;
			Diagnostics::put("RInit");
			break;
		case 235: // Upper Shift (shift to Extended ASCII)
			upperShift.set = true;
			Diagnostics::put("UpSh");
			break;
		case 236: // 05 Macro
			result.append("[)>\x1E""05\x1D");
			resultTrailer.insert(0, "\x1E\x04");
			Diagnostics::put("Macro05");
			break;
		case 237: // 06 Macro
			result.append("[)>\x1E""06\x1D");
			resultTrailer.insert(0, "\x1E\x04");
			Diagnostics::put("Macro06");
			break;
		case 238: // Latch to ANSI X12 encodation
			Diagnostics::put("X12");
			return Mode::ANSIX12_ENCODE;
		case 239: // Latch to Text encodation
			Diagnostics::put("TEX");
			return Mode::TEXT_ENCODE;
		case 240: // Latch to EDIFACT encodation
			Diagnostics::put("EDI");
			return Mode::EDIFACT_ENCODE;
		case 241: // ECI Character
			state.encoding = CharacterSetECI::OnChangeAppendReset(ParseECIValue(bits, position), resultEncoded,
																  result, state.encoding, &state.sai.lastECI);
			break;
		default:
			if (oneByte <= 128) { // ASCII data (ASCII value + 1)
				result.push_back(upperShift(oneByte) - 1);
				Diagnostics::chr(result.back(), "A", upperShift.set /*appendHex*/);
			} else if (oneByte <= 229) { // 2-digit data 00-99 (Numeric Value + 130)
				int value = oneByte - 130;
				if (value < 10) // pad with '0' for single digit values
					result.push_back('0');
				result.append(std::to_string(value));
				Diagnostics::fmt("%02d", value);
			} else if (oneByte >= 242) { // Not to be used in ASCII encodation
				// work around encoders that use unlatch to ASCII as last code word (ask upstream)
				if (oneByte == 254 && bits.available() == 0) {
					Diagnostics::fmt("ASCWarn(%d)", oneByte);
					break;
				}
				Diagnostics::fmt("ASCError(%d)", oneByte);
				return Mode::FORMAT_ERROR;
			}
		}
		state.firstCodeword = false;
	}

	return Mode::DONE;
}

std::optional<std::array<int, 3>> DecodeNextTriple(BitSource& bits)
{
	// Values are encoded in a 16-bit value as (1600 * C1) + (40 * C2) + C3 + 1
	// If there is less than 2 bytes left or the next byte is the unlatch codeword then the current segment has ended
	if (bits.available() < 16)
		return {};
	int firstByte = bits.readBits(8);
	if (firstByte == 254) // Unlatch codeword
		return {};

	int fullBitValue = (firstByte << 8) + bits.readBits(8) - 1;
	int a = fullBitValue / 1600;
	fullBitValue -= a * 1600;
	int b = fullBitValue / 40;
	int c = fullBitValue - b * 40;

	return {{a, b, c}};
}

/**
* See ISO 16022:2006, 5.2.5 and Annex C, Table C.1 (C40)
* See ISO 16022:2006, 5.2.6 and Annex C, Table C.2 (Text)
*/
static bool DecodeC40OrTextSegment(BitSource& bits, std::string& result, const Mode mode)
{
	// TODO(bbrown): The Upper Shift with C40 doesn't work in the 4 value scenario all the time
	Shift128 upperShift;
	int shift = 0;

	const bool isC40 = mode == C40_ENCODE;
	const char* const BASIC_SET_CHARS = isC40 ? C40_BASIC_SET_CHARS : TEXT_BASIC_SET_CHARS;
	const char* const SHIFT_SET_CHARS = isC40 ? C40_SHIFT2_SET_CHARS : TEXT_SHIFT2_SET_CHARS;
	const char* const prefixIfNonASCII = isC40 ? "C" : "T";
	const char* const modeName = isC40 ? "C40" : "TEX";

	while (auto triple = DecodeNextTriple(bits)) {
		for (int cValue : *triple) {
			switch (std::exchange(shift, 0)) {
			case 0:
				if (cValue < 3) {
					shift = cValue + 1;
					Diagnostics::fmt("Sh%d", shift);
				} else if (cValue < 40) { // Size(BASIC_SET_CHARS)
					result.push_back(upperShift(BASIC_SET_CHARS[cValue]));
					Diagnostics::chr(result.back(), prefixIfNonASCII);
				} else {
					Diagnostics::fmt("%sErrorShift0(%d)", modeName, cValue);
					return false;
				}
				break;
			case 1:
				if (cValue < 32) {
					result.push_back(upperShift(cValue));
					Diagnostics::chr(result.back(), prefixIfNonASCII);
				} else {
					Diagnostics::fmt("%sErrorShift1(%d)", modeName, cValue);
					return false;
				}
				break;
			case 2:
				if (cValue < 28) { // Size(SHIFT_SET_CHARS))
					result.push_back(upperShift(SHIFT_SET_CHARS[cValue]));
					Diagnostics::chr(result.back(), prefixIfNonASCII);
				} else if (cValue == 30) { // Upper Shift
					upperShift.set = true;
					Diagnostics::put("UpSh");
				} else {
					Diagnostics::fmt("%sErrorShift2(%d)", modeName, cValue);
					return false;
				}
				break;
			case 3:
				if (cValue < 32) {
					if (isC40)
						result.push_back(upperShift(cValue + 96));
					else
						result.push_back(upperShift(TEXT_SHIFT3_SET_CHARS[cValue]));
					Diagnostics::chr(result.back(), prefixIfNonASCII);
				} else {
					Diagnostics::fmt("%sErrorShift3(%d)", modeName, cValue);
					return false;
				}
				break;
			default:
				Diagnostics::fmt("%sError(Shift)", modeName);
				return false;
			}
		}
	}

	return true;
}

/**
* See ISO 16022:2006, 5.2.7
*/
static bool DecodeAnsiX12Segment(BitSource& bits, std::string& result)
{
	while (auto triple = DecodeNextTriple(bits)) {
		for (int cValue : *triple) {
			// X12 segment terminator <CR>, separator *, sub-element separator >, space
			static const char segChars[4] = {'\r', '*', '>', ' '};
			if (cValue < 0)
				return false;
			else if (cValue < 4)
				result.push_back(segChars[cValue]);
			else if (cValue < 14) // 0 - 9
				result.push_back((char)(cValue + 44));
			else if (cValue < 40) // A - Z
				result.push_back((char)(cValue + 51));
			else {
				Diagnostics::put("X12Error");
				return false;
			}
			Diagnostics::chr(result.back());
		}
	}

	return true;
}

/**
* See ISO 16022:2006, 5.2.8 and Annex C Table C.3
*/
static bool DecodeEdifactSegment(BitSource& bits, std::string& result)
{
	// If there are less than 3 bytes left then it will be encoded as ASCII
	while (bits.available() >= 24) {
		for (int i = 0; i < 4; i++) {
			char edifactValue = bits.readBits(6);

			// Check for the unlatch character
			if (edifactValue == 0x1F) {  // 011111
				// Read rest of byte, which should be 0, and stop
				if (bits.bitOffset())
					bits.readBits(8 - bits.bitOffset());
				return true;
			}

			if ((edifactValue & 0x20) == 0) // no 1 in the leading (6th) bit
				edifactValue |= 0x40;       // Add a leading 01 to the 6 bit binary value
			result.push_back(edifactValue);
			Diagnostics::chr(result.back());
		}
	}

	return true;
}

/**
* See ISO 16022:2006, Annex B, B.2
*/
static int Unrandomize255State(int randomizedBase256Codeword, int base256CodewordPosition)
{
	int pseudoRandomNumber = ((149 * base256CodewordPosition) % 255) + 1;
	int tempVariable = randomizedBase256Codeword - pseudoRandomNumber;
	return tempVariable >= 0 ? tempVariable : tempVariable + 256;
}

/**
* See ISO 16022:2006, 5.2.9 and Annex B, B.2
*/
static bool DecodeBase256Segment(BitSource& bits, std::string& result)
{
	// Figure out how long the Base 256 Segment is.
	int codewordPosition = 1 + bits.byteOffset(); // position is 1-indexed
	int d1 = Unrandomize255State(bits.readBits(8), codewordPosition++);
	int count;
	if (d1 == 0) // Read the remainder of the symbol
		count = bits.available() / 8;
	else if (d1 < 250)
		count = d1;
	else
		count = 250 * (d1 - 249) + Unrandomize255State(bits.readBits(8), codewordPosition++);

	// We're seeing NegativeArraySizeException errors from users.
	if (count < 0) {
		Diagnostics::put("BASError(NegCount)");
		return false;
	}

	ByteArray bytes(count);
	for (int i = 0; i < count; i++) {
		// Have seen this particular error in the wild, such as at
		// http://www.bcgen.com/demo/IDAutomationStreamingDataMatrix.aspx?MODE=3&D=Fred&PFMT=3&PT=F&X=0.3&O=0&LM=0.2
		if (bits.available() < 8) {
			Diagnostics::put("BASError(Incomplete)");
			return false;
		}

		bytes[i] = (uint8_t)Unrandomize255State(bits.readBits(8), codewordPosition++);
		Diagnostics::chr(bytes[i], "B", true /*appendHex*/);
	}

	result.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());

	return true;
}

ZXING_EXPORT_TEST_ONLY
DecoderResult Decode(ByteArray&& bytes, const std::string& characterSet, const bool isDMRE)
{
	BitSource bits(bytes);
	std::string result;
	result.reserve(100);
	std::string resultTrailer;
	std::wstring resultEncoded;
	Mode mode = Mode::ASCII_ENCODE;
	State state;
	state.encoding = CharacterSetECI::InitEncoding(characterSet);

	Diagnostics::put("  Decode:        ");
	while (mode != Mode::FORMAT_ERROR && mode != Mode::DONE) {
		if (mode == Mode::ASCII_ENCODE) {
			mode = DecodeAsciiSegment(bits, result, resultTrailer, resultEncoded, state);
			state.firstCodeword = false;
			state.firstFNC1Position = -1; // Only recognize in first segment
		} else {
			bool decodeOK;
			switch (mode) {
			case C40_ENCODE: [[fallthrough]];
			case TEXT_ENCODE: decodeOK = DecodeC40OrTextSegment(bits, result, mode); break;
			case ANSIX12_ENCODE: decodeOK = DecodeAnsiX12Segment(bits, result); break;
			case EDIFACT_ENCODE: decodeOK = DecodeEdifactSegment(bits, result); break;
			case BASE256_ENCODE: decodeOK = DecodeBase256Segment(bits, result); break;
			default: decodeOK = false; Diagnostics::put("ModeError"); break;
			}
			mode = decodeOK ? Mode::ASCII_ENCODE : Mode::FORMAT_ERROR;
			if (decodeOK) {
				Diagnostics::put("ASC");
			}
		}
	}
	if (bits.available() <= 0) Diagnostics::put("EOD");

	if (mode == Mode::FORMAT_ERROR)
		return DecodeStatus::FormatError;

	if (state.readerInit && state.sai.index > -1) // Not allowed together ISO 16022:2006 5.2.4.9
		return DecodeStatus::FormatError;

	if (resultTrailer.length() > 0)
		result.append(resultTrailer);

	TextDecoder::Append(resultEncoded, reinterpret_cast<const uint8_t*>(result.data()), result.size(), state.encoding);

	std::string symbologyIdentifier("]d" + std::to_string(state.symbologyIdModifier + (isDMRE ? 6 : 0)));

	return DecoderResult(std::move(bytes), std::move(resultEncoded))
			.setSymbologyIdentifier(std::move(symbologyIdentifier))
			.setStructuredAppend(state.sai)
			.setReaderInit(state.readerInit);
}

} // namespace DecodedBitStreamParser

/**
* <p>Given data and error-correction codewords received, possibly corrupted by errors, attempts to
* correct the errors in-place using Reed-Solomon error correction.</p>
*
* @param codewordBytes data and error correction codewords
* @param numDataCodewords number of codewords that are data bytes
* @throws ChecksumException if error correction fails
*/
static bool
CorrectErrors(ByteArray& codewordBytes, int numDataCodewords)
{
	// First read into an array of ints
	std::vector<int> codewordsInts(codewordBytes.begin(), codewordBytes.end());
	int numECCodewords = Size(codewordBytes) - numDataCodewords;

	Diagnostics::fmt("  DataCodewords: (%d)", numDataCodewords); Diagnostics::dump(codewordsInts, "\n", 0, numDataCodewords);
	Diagnostics::fmt("  ECCodewords:   (%d)", numECCodewords); Diagnostics::dump(codewordsInts, "\n", numDataCodewords);

	if (!ReedSolomonDecode(GenericGF::DataMatrixField256(), codewordsInts, numECCodewords))
		return false;

	// Copy back into array of bytes -- only need to worry about the bytes that were data
	// We don't care about errors in the error-correction codewords
	std::copy_n(codewordsInts.begin(), numDataCodewords, codewordBytes.begin());

	return true;
}

static DecoderResult DoDecode(const BitMatrix& bits, const std::string& characterSet)
{
	// Construct a parser and read version, error-correction level
	const Version* version = VersionForDimensionsOf(bits);
	if (version == nullptr)
		return DecodeStatus::FormatError;
	Diagnostics::fmt("  Dimensions:    %dx%d (HxW)\n", bits.height(), bits.width());

	// Read codewords
	ByteArray codewords = CodewordsFromBitMatrix(bits);
	if (codewords.empty())
		return DecodeStatus::FormatError;

	// Separate into data blocks
	std::vector<DataBlock> dataBlocks = GetDataBlocks(codewords, *version);
	if (dataBlocks.empty())
		return DecodeStatus::FormatError;

	// Count total number of data bytes
	ByteArray resultBytes(TransformReduce(dataBlocks, 0, [](const auto& db) { return db.numDataCodewords; }));

	// Error-correct and copy data blocks together into a stream of bytes
	const int dataBlocksCount = Size(dataBlocks);
	for (int j = 0; j < dataBlocksCount; j++) {
		auto& dataBlock = dataBlocks[j];
		ByteArray& codewordBytes = dataBlock.codewords;
		int numDataCodewords = dataBlock.numDataCodewords;
		if (!CorrectErrors(codewordBytes, numDataCodewords))
			return DecodeStatus::ChecksumError;

		for (int i = 0; i < numDataCodewords; i++) {
			// De-interlace data blocks.
			resultBytes[i * dataBlocksCount + j] = codewordBytes[i];
		}
	}

	// Decode the contents of that stream of bytes
	return DecodedBitStreamParser::Decode(std::move(resultBytes), characterSet, version->isDMRE());
}

static BitMatrix FlippedL(const BitMatrix& bits)
{
	BitMatrix res(bits.height(), bits.width());
	for (int y = 0; y < res.height(); ++y)
		for (int x = 0; x < res.width(); ++x)
			res.set(x, y, bits.get(bits.width() - 1 - y, bits.height() - 1 - x));
	return res;
}

DecoderResult Decode(const BitMatrix& bits, const std::string& characterSet)
{
	auto res = DoDecode(bits, characterSet);
	if (res.isValid())
		return res;

	//TODO:
	// * report mirrored state (see also QRReader)
	// * unify bit mirroring helper code with QRReader?
	// * rectangular symbols with the a size of 8 x Y are not supported a.t.m.
	if (auto mirroredRes = DoDecode(FlippedL(bits), characterSet); mirroredRes.isValid())
		return mirroredRes;

	return res;
}

} // namespace ZXing::DataMatrix
