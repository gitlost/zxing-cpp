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
#include "CharacterSet.h"
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
#include "ZXStrConvWorkaround.h"
#include "ZXTestSupport.h"

#include <algorithm>
#include <array>
#include <list>
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

enum Mode {
	FORMAT_ERROR,
	PAD_ENCODE, // Not really a mode
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
	'/', ':', ';', '<', '=', '>', '?',  '@', '[', '\\', ']', '^', '_'
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

constexpr CharacterSet DEFAULT_ENCODING = CharacterSet::ISO8859_1;

struct StructuredAppend {
	int sequenceNumber = 0;
	int codeCount = 0; // Total count
	int id = 0;
	int eci = 0; // Final ECI in effect
};

struct State {
	CharacterSet encoding = DEFAULT_ENCODING;
	bool firstFNC1 = true;
	int firstFNC1Position = 1;
	int symbolIdMode = 1; // ECC 200 (Annex N Table N.1)
	bool readerInit = false;
	bool gs1 = false;
	struct StructuredAppend structuredAppend;
};

/**
* See ISO 16022:2006, 5.4.1, Table 6
*/
static int ParseECIValue(BitSource& bits)
{
	int firstByte = bits.readBits(8);
	if (firstByte <= 127) {
		return firstByte - 1;
	}
	int secondByte = bits.readBits(8);
	if (firstByte <= 191) {
		return (firstByte - 128) * 254 + 127 + secondByte - 1;
	}
	int thirdByte = bits.readBits(8);
	return (firstByte - 192) * 64516 + 16383 + (secondByte - 1) * 254 + thirdByte - 1;
}

/**
* See ISO 16022:2006, 5.6
*/
static void ParseStructuredAppend(BitSource& bits, StructuredAppend& structuredAppend, Diagnostics& diagnostics)
{
	int symbolSeq = bits.readBits(8); // Symbol sequence indicator
	structuredAppend.sequenceNumber = (symbolSeq >> 4) + 1;
	structuredAppend.codeCount = 17 - (symbolSeq & 0x0F);

	if (structuredAppend.codeCount == 17 || structuredAppend.codeCount < structuredAppend.sequenceNumber) {
		diagnostics.fmt("SASeqIndWarn(0x%X)", symbolSeq);
	}

	// Note this conversion not stated in Section 5.6.3
	// Following tec-it here but as long as info maintained it doesn't really matter
	structuredAppend.id = 0;
	int fileId1 = bits.readBits(8); // File identification 1
	if (fileId1 >= 1 && fileId1 <= 254) {
		structuredAppend.id += (fileId1 - 1) * 254;
	}
	else {
		diagnostics.fmt("SAFileId1Warn(%d)", fileId1);
	}
	int fileId2 = bits.readBits(8); // File identification 2
	if (fileId2 >= 1 && fileId2 <= 254) {
		structuredAppend.id += (fileId2 - 1);
	}
	else {
		diagnostics.fmt("SAFileId2Warn(%d)", fileId2);
	}
}

/**
* See ISO 16022:2006, 5.2.3 and Annex C, Table C.2
*/
static Mode DecodeAsciiSegment(BitSource& bits, std::string& result, std::string& resultTrailer,
							   std::wstring& resultEncoded, State& state, Diagnostics& diagnostics)
{
	int eci;
	int position = 1;
	bool upperShift = false;
	do {
		int oneByte = bits.readBits(8);
		switch (oneByte) {
		case 0:
			diagnostics.put("ASCError(0)");
			return Mode::FORMAT_ERROR;
		case 129: // Pad
			diagnostics.put("PAD");
			return Mode::PAD_ENCODE;
		case 230: // Latch to C40 encodation
			diagnostics.put("C40");
			return Mode::C40_ENCODE;
		case 231: // Latch to Base 256 encodation
			diagnostics.put("BAS");
			return Mode::BASE256_ENCODE;
		case 232: // FNC1
			if (state.firstFNC1 && (position == state.firstFNC1Position || position == state.firstFNC1Position + 1)) {
				// Note as converting ECI data ourselves, symbology identifier modes 4-6 (and A-C) signaling ECI
				// protocol not used
				if (position == state.firstFNC1Position) {
					state.symbolIdMode = 2;
					state.gs1 = true;
					diagnostics.put("FNC1(GS1)");
				} else {
					state.symbolIdMode = 3;
					diagnostics.put("FNC1(2ndPos)");
				}
			} else {
				result.push_back((char)29); // translate as ASCII 29
				diagnostics.put("FNC1(29)");
			}
			break;
		case 233: // Structured Append
			ParseStructuredAppend(bits, state.structuredAppend, diagnostics);
			if (state.firstFNC1) {
				state.firstFNC1Position = 5;
			}
			diagnostics.fmt("SA(%d,%d,%d)", state.structuredAppend.sequenceNumber, state.structuredAppend.codeCount,
							state.structuredAppend.id);
			break;
		case 234: // Reader Programming
			state.readerInit = true;
			diagnostics.put("RInit");
			break;
		case 235: // Upper Shift (shift to Extended ASCII)
			upperShift = true;
			diagnostics.put("UpSh");
			break;
		case 236: // 05 Macro
			result.append("[)>\x1E""05\x1D");
			resultTrailer.insert(0, "\x1E\x04");
			diagnostics.put("Macro05");
			break;
		case 237: // 06 Macro
			result.append("[)>\x1E""06\x1D");
			resultTrailer.insert(0, "\x1E\x04");
			diagnostics.put("Macro06");
			break;
		case 238: // Latch to ANSI X12 encodation
			diagnostics.put("X12");
			return Mode::ANSIX12_ENCODE;
		case 239: // Latch to Text encodation
			diagnostics.put("TEX");
			return Mode::TEXT_ENCODE;
		case 240: // Latch to EDIFACT encodation
			diagnostics.put("EDI");
			return Mode::EDIFACT_ENCODE;
		case 241:  // ECI Character
			eci = ParseECIValue(bits);
			state.structuredAppend.eci = eci;
			diagnostics.fmt("ECI(%d)", eci);
			if (eci >= 0 && eci <= 899) { // Character Set ECIs
				CharacterSet encoding = CharacterSetECI::CharsetFromValue(eci);
				if (encoding != CharacterSet::Unknown && encoding != state.encoding) {
					// Encode data so far in current encoding and reset
					TextDecoder::Append(resultEncoded, reinterpret_cast<const uint8_t*>(result.data()), result.size(),
										state.encoding);
					result.clear();
					state.encoding = encoding;
				}
			}
			break;
		default:
			if (oneByte <= 128) {  // ASCII data (ASCII value + 1)
				if (upperShift) {
					oneByte += 128;
				}
				result.push_back((char)(oneByte - 1));
				diagnostics.chr(result.back(), "A", upperShift /*appendHex*/);
				return Mode::ASCII_ENCODE;
			}
			else if (oneByte <= 229) {  // 2-digit data 00-99 (Numeric Value + 130)
				int value = oneByte - 130;
				if (value < 10) { // pad with '0' for single digit values
					result.push_back('0');
				}
				result.append(std::to_string(value));
				diagnostics.fmt("%02d", value);
			}
			else if (oneByte >= 242) {  // Not to be used in ASCII encodation
									// ... but work around encoders that end with 254, latch back to ASCII
				if (oneByte != 254 || bits.available() != 0) {
					diagnostics.fmt("ASCError(%d)", oneByte);
					return Mode::FORMAT_ERROR;
				}
				diagnostics.fmt("ASCWarn(%d)", oneByte);
			}
		}
		position++;
	} while (bits.available() > 0);
	diagnostics.put("ASC(EOD)");
	return Mode::ASCII_ENCODE;
}

static std::array<int, 3> ParseTwoBytes(int firstByte, int secondByte)
{
	int fullBitValue = (firstByte << 8) + secondByte - 1;
	int a = fullBitValue / 1600;
	fullBitValue -= a * 1600;
	int b = fullBitValue / 40;
	int c = fullBitValue - b * 40;
	return {a, b, c};
}

/**
* See ISO 16022:2006, 5.2.5 and Annex C, Table C.1 (C40)
* See ISO 16022:2006, 5.2.6 and Annex C, Table C.2 (TEXT)
*/
static bool DecodeC40TextSegment(const Mode mode, BitSource& bits, std::string& result, Diagnostics& diagnostics)
{
	// Three C40/TEXT values are encoded in a 16-bit value as
	// (1600 * C1) + (40 * C2) + C3 + 1
	// TODO(bbrown): The Upper Shift with C40 doesn't work in the 4 value scenario all the time
	bool upperShift = false;

	const bool isC40 = mode == C40_ENCODE;
	const char* const basic_set_chars = isC40 ? C40_BASIC_SET_CHARS : TEXT_BASIC_SET_CHARS;
	const char* const shift2_set_chars = isC40 ? C40_SHIFT2_SET_CHARS : TEXT_SHIFT2_SET_CHARS;
	const char* const prefixIfNonASCII = isC40 ? "C" : "T";
	const char* const modeName = isC40 ? "C40" : "TEX";

	int shift = 0;
	do {
		// If there is only one byte left then it will be either an unlatch or data encoded as ASCII
		if (bits.available() == 8) {
			if (bits.lookBits(8) == 254) {
				(void)bits.readBits(8);
                diagnostics.put("ASC(Un)");
				return true;
			}
            diagnostics.put("ASC(1)");
			return true;
		}
		int firstByte = bits.readBits(8);
		if (firstByte == 254) {  // Unlatch codeword
            diagnostics.put("ASC");
			return true;
		}

		for (int cValue : ParseTwoBytes(firstByte, bits.readBits(8))) {
			switch (shift) {
			case 0:
				if (cValue < 3) {
					shift = cValue + 1;
					diagnostics.fmt("Sh%d", shift);
				}
				else if (cValue < 40) {
					unsigned char ch = basic_set_chars[cValue];
					if (upperShift) {
						ch += 128;
						upperShift = false;
					}
					result.push_back(ch);
					diagnostics.chr(result.back(), prefixIfNonASCII);
				}
				else {
					diagnostics.fmt("%sErrorShift0(%d)", modeName, cValue);
					return false;
				}
				break;
			case 1:
				if (cValue < 32) {
					unsigned char ch = cValue;
					if (upperShift) {
						ch += 128;
						upperShift = false;
					}
					result.push_back(ch);
					diagnostics.chr(result.back(), prefixIfNonASCII);
				}
				else {
					diagnostics.fmt("%sErrorShift1(%d)", modeName, cValue);
					return false;
				}
				shift = 0;
				break;
			case 2:
				if (cValue < 27) {
					unsigned char ch = shift2_set_chars[cValue];
					if (upperShift) {
						ch += 128;
						upperShift = false;
					}
					result.push_back(ch);
					diagnostics.chr(result.back(), prefixIfNonASCII);
				}
				else if (cValue == 27) {  // FNC1
					result.push_back((char)29); // translate as ASCII 29
					diagnostics.put("FNC1(29)");
				}
				else if (cValue == 30) {  // Upper Shift
					upperShift = true;
					diagnostics.put("UpSh");
				}
				else {
					diagnostics.fmt("%sErrorShift2(%d)", modeName, cValue);
					return false;
				}
				shift = 0;
				break;
			case 3:
				if (cValue < 32) {
					unsigned char ch = isC40 ? cValue + 96 : TEXT_SHIFT3_SET_CHARS[cValue];
					if (upperShift) {
						ch += 128;
						upperShift = false;
					}
					result.push_back(ch);
					diagnostics.chr(result.back(), prefixIfNonASCII);
				}
				else {
					diagnostics.fmt("%sErrorShift3(%d)", modeName, cValue);
					return false;
				}
				shift = 0;
				break;
			default:
				diagnostics.fmt("%sError(Shift)", modeName);
				return false;
			}
		}
	} while (bits.available() > 0);
    diagnostics.put("ASC(EOD)");
	return true;
}

/**
* See ISO 16022:2006, 5.2.7
*/
static bool DecodeAnsiX12Segment(BitSource& bits, std::string& result, Diagnostics& diagnostics)
{
	// Three ANSI X12 values are encoded in a 16-bit value as
	// (1600 * C1) + (40 * C2) + C3 + 1

	do {
		// If there is only one byte left then it will be encoded as ASCII
		if (bits.available() == 8) {
            diagnostics.put("ASC(1)");
			return true;
		}
		int firstByte = bits.readBits(8);
		if (firstByte == 254) {  // Unlatch codeword
            diagnostics.put("ASC");
			return true;
		}

		for (auto cValue : ParseTwoBytes(firstByte, bits.readBits(8))) {
			static const char seg_chars[4] = { '\r', '*', '>', ' ' };
			if (cValue < 4) {
				result.push_back(seg_chars[cValue]);
			}
			else if (cValue < 14) {  // 0 - 9
				result.push_back((char)(cValue + 44));
			}
			else if (cValue < 40) {  // A - Z
				result.push_back((char)(cValue + 51));
			}
			else {
				diagnostics.put("X12Error");
				return false;
			}
			diagnostics.chr(result.back());
		}
	} while (bits.available() > 0);
    diagnostics.put("ASC(EOD)");
	return true;
}

/**
* See ISO 16022:2006, 5.2.8 and Annex C Table C.3
*/
static bool DecodeEdifactSegment(BitSource& bits, std::string& result, Diagnostics& diagnostics)
{
	do {
		// If there is only two or less bytes left then it will be encoded as ASCII
		if (bits.available() <= 16) {
            diagnostics.put(bits.available() <= 8 ? "ASC(1)" : "ASC(2)");
			return true;
		}

		for (int i = 0; i < 4; i++) {
			int edifactValue = bits.readBits(6);

			// Check for the unlatch character
			if (edifactValue == 0x1F) {  // 011111
										 // Read rest of byte, which should be 0, and stop
				int bitsLeft = 8 - bits.bitOffset();
				if (bitsLeft != 8) {
					bits.readBits(bitsLeft);
				}
                diagnostics.put("ASC");
				return true;
			}

			if ((edifactValue & 0x20) == 0) {  // no 1 in the leading (6th) bit
				edifactValue |= 0x40;  // Add a leading 01 to the 6 bit binary value
			}
			result.push_back((char)edifactValue);
			diagnostics.chr(result.back());
		}
	} while (bits.available() > 0);
    diagnostics.put("ASC(EOD)");
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
static bool DecodeBase256Segment(BitSource& bits, std::string& result, std::list<ByteArray>& byteSegments, Diagnostics& diagnostics)
{
	// Figure out how long the Base 256 Segment is.
	int codewordPosition = 1 + bits.byteOffset(); // position is 1-indexed
	int d1 = Unrandomize255State(bits.readBits(8), codewordPosition++);
	int count;
	if (d1 == 0) {  // Read the remainder of the symbol
		count = bits.available() / 8;
	}
	else if (d1 < 250) {
		count = d1;
	}
	else {
		count = 250 * (d1 - 249) + Unrandomize255State(bits.readBits(8), codewordPosition++);
	}

	// We're seeing NegativeArraySizeException errors from users.
	if (count < 0) {
		diagnostics.put("BASError(NegCount)");
		return false;
	}

	ByteArray bytes(count);
	for (int i = 0; i < count; i++) {
		// Have seen this particular error in the wild, such as at
		// http://www.bcgen.com/demo/IDAutomationStreamingDataMatrix.aspx?MODE=3&D=Fred&PFMT=3&PT=F&X=0.3&O=0&LM=0.2
		if (bits.available() < 8) {
			diagnostics.put("BASError(Incomplete)");
			return false;
		}
		bytes[i] = (uint8_t)Unrandomize255State(bits.readBits(8), codewordPosition++);
		diagnostics.chr(bytes[i], "B", true /*appendHex*/);
	}
	byteSegments.push_back(bytes);

	result.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	diagnostics.put("ASC");
	return true;
}

ZXING_EXPORT_TEST_ONLY
DecoderResult Decode(ByteArray&& bytes, const std::string& characterSet, Diagnostics& diagnostics, const bool isDMRE)
{
	BitSource bits(bytes);
	std::string result;
	result.reserve(100);
	std::string resultTrailer;
	std::wstring resultEncoded;
	std::list<ByteArray> byteSegments;
	Mode mode = Mode::ASCII_ENCODE;
	State state;

	if (!characterSet.empty()) {
		auto encodingInit = CharacterSetECI::CharsetFromName(characterSet.c_str());
		if (encodingInit != CharacterSet::Unknown) {
			state.encoding = encodingInit;
		}
	}

	diagnostics.put("  Decode:        ");
	do {
		if (mode == Mode::ASCII_ENCODE) {
			mode = DecodeAsciiSegment(bits, result, resultTrailer, resultEncoded, state, diagnostics);
			state.firstFNC1 = false;
		}
		else {
			bool decodeOK;
			switch (mode) {
			case C40_ENCODE:
			case TEXT_ENCODE:
				decodeOK = DecodeC40TextSegment(mode, bits, result, diagnostics);
				break;
			case ANSIX12_ENCODE:
				decodeOK = DecodeAnsiX12Segment(bits, result, diagnostics);
				break;
			case EDIFACT_ENCODE:
				decodeOK = DecodeEdifactSegment(bits, result, diagnostics);
				break;
			case BASE256_ENCODE:
				decodeOK = DecodeBase256Segment(bits, result, byteSegments, diagnostics);
				break;
			default:
				decodeOK = false;
				diagnostics.put("ModeError");
				break;
			}
			if (!decodeOK) {
				return DecodeStatus::FormatError;
			}
			mode = Mode::ASCII_ENCODE;
		}
	} while (mode != Mode::PAD_ENCODE && bits.available() > 0);
	if (bits.available() <= 0) diagnostics.put("EOD");

	if (resultTrailer.length() > 0) {
		result.append(resultTrailer);
	}
	TextDecoder::Append(resultEncoded, reinterpret_cast<const uint8_t*>(result.data()), result.size(), state.encoding);

	std::string symbologyIdentifier = "]d" + std::to_string(state.symbolIdMode + (isDMRE ? 6 : 0));

	DecoderResult decoderResult(std::move(bytes), std::move(resultEncoded));
	decoderResult.setByteSegments(std::move(byteSegments));
	decoderResult.setSymbologyIdentifier(std::move(symbologyIdentifier));

	if (state.structuredAppend.sequenceNumber) {
		decoderResult.setStructuredAppendSequenceNumber(state.structuredAppend.sequenceNumber);
	}
	if (state.structuredAppend.codeCount) {
		decoderResult.setStructuredAppendCodeCount(state.structuredAppend.codeCount);
	}
	if (state.structuredAppend.id) {
		decoderResult.setStructuredAppendId(std::to_string(state.structuredAppend.id));
	}
	if (state.structuredAppend.eci) {
		decoderResult.setStructuredAppendECI(state.structuredAppend.eci);
	}
	if (state.readerInit) {
		decoderResult.setReaderInit(state.readerInit);
	}
	if (diagnostics.enabled()) {
		decoderResult.setDiagnostics(std::move(diagnostics.get()));
	}

	return decoderResult;
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
CorrectErrors(ByteArray& codewordBytes, int numDataCodewords, Diagnostics& diagnostics)
{
	// First read into an array of ints
	std::vector<int> codewordsInts(codewordBytes.begin(), codewordBytes.end());
	int numECCodewords = Size(codewordBytes) - numDataCodewords;

	diagnostics.fmt("  DataCodewords: (%d)", numDataCodewords); diagnostics.put(codewordsInts, "\n", 0, numDataCodewords);
	diagnostics.fmt("  ECCodewords:   (%d)", numECCodewords); diagnostics.put(codewordsInts, "\n", numDataCodewords);

	if (!ReedSolomonDecode(GenericGF::DataMatrixField256(), codewordsInts, numECCodewords))
		return false;

	// Copy back into array of bytes -- only need to worry about the bytes that were data
	// We don't care about errors in the error-correction codewords
	std::copy_n(codewordsInts.begin(), numDataCodewords, codewordBytes.begin());

	return true;
}

static DecoderResult DoDecode(const BitMatrix& bits, const std::string& characterSet, const bool enableDiagnostics)
{
    Diagnostics diagnostics(enableDiagnostics);
	// Construct a parser and read version, error-correction level
	const Version* version = VersionForDimensionsOf(bits);
	if (version == nullptr) {
		return DecodeStatus::FormatError;
	}
	diagnostics.fmt("  Dimensions:    %dx%d (HxW)\n", bits.height(), bits.width());

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
		if (!CorrectErrors(codewordBytes, numDataCodewords, diagnostics))
			return DecodeStatus::ChecksumError;

		for (int i = 0; i < numDataCodewords; i++) {
			// De-interlace data blocks.
			resultBytes[i * dataBlocksCount + j] = codewordBytes[i];
		}
	}

	// Decode the contents of that stream of bytes
	return DecodedBitStreamParser::Decode(std::move(resultBytes), characterSet, diagnostics, version->isDMRE());
}

static BitMatrix FlippedL(const BitMatrix& bits)
{
	BitMatrix res(bits.height(), bits.width());
	for (int y = 0; y < res.height(); ++y)
		for (int x = 0; x < res.width(); ++x)
			res.set(x, y, bits.get(bits.width() - 1 - y, bits.height() - 1 - x));
	return res;
}

DecoderResult Decode(const BitMatrix& bits, const std::string& characterSet, const bool enableDiagnostics)
{
	auto res = DoDecode(bits, characterSet, enableDiagnostics);
	if (res.isValid())
		return res;

	//TODO:
	// * report mirrored state (see also QRReader)
	// * unify bit mirroring helper code with QRReader?
	// * rectangular symbols with the a size of 8 x Y are not supported a.t.m.
	if (auto mirroredRes = DoDecode(FlippedL(bits), characterSet, enableDiagnostics); mirroredRes.isValid())
		return mirroredRes;

	return res;
}

} // namespace ZXing::DataMatrix
