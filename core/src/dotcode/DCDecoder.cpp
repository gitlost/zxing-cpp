/*
* Copyright 2021 gitlost
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

#include "DCDecoder.h"

#include "ByteArray.h"
#include "CharacterSetECI.h"
#include "DecoderResult.h"
#include "DecodeStatus.h"
#include "Diagnostics.h"
#include "GenericGF.h"
#include "DCBitMatrixParser.h"
#include "DCDataBlock.h"
#include "DCGField.h"
#include "ReedSolomonDecoder.h"
#include "TextDecoder.h"
#include "ZXTestSupport.h"

#include <vector>

namespace ZXing::DotCode {

namespace DecodedBitStreamParser {

// Table 1
constexpr int CODESET_C	= 1;
constexpr int CODESET_A	= 2;
constexpr int CODESET_B	= 3;

constexpr int CC_1710 = 100;
constexpr int CC_LATCH_A = 101;
constexpr int CC_SHIFT_B = 102;
constexpr int CC_2SHIFT_B = 103;
constexpr int CC_3SHIFT_B = 104;
constexpr int CC_4SHIFT_B = 105;
constexpr int CC_LATCH_B = 106;

constexpr int CA_SHIFT_B = 96;
constexpr int CA_2SHIFT_B = 97;
constexpr int CA_3SHIFT_B = 98;
constexpr int CA_4SHIFT_B = 99;
constexpr int CA_5SHIFT_B = 100;
constexpr int CA_6SHIFT_B = 101;
constexpr int CA_LATCH_B = 102;
constexpr int CA_2SHIFT_C = 103;
constexpr int CA_3SHIFT_C = 104;
constexpr int CA_4SHIFT_C = 105;
constexpr int CA_LATCH_C = 106;

constexpr int CB_CRLF = 96;
constexpr int CB_HT = 97;
constexpr int CB_FS = 98;
constexpr int CB_GS = 99;
constexpr int CB_RS = 100;
constexpr int CB_SHIFT_A = 101;
constexpr int CB_LATCH_A = 102;
constexpr int CB_2SHIFT_C = 103;
constexpr int CB_3SHIFT_C = 104;
constexpr int CB_4SHIFT_C = 105;
constexpr int CB_LATCH_C = 106;

// Common to codesets C, A, B
constexpr int C_FNC1 = 107;
constexpr int C_FNC2 = 108;
constexpr int C_FNC3 = 109;
constexpr int C_UPPERSHIFT_A = 110;
constexpr int C_UPPERSHIFT_B = 111;
constexpr int C_BINARY_LATCH = 112;

// Binary, Table 3
constexpr int BIN_2SHIFT_C = 103;
constexpr int BIN_3SHIFT_C = 104;
constexpr int BIN_4SHIFT_C = 105;
constexpr int BIN_5SHIFT_C = 106;
constexpr int BIN_6SHIFT_C = 107;
constexpr int BIN_7SHIFT_C = 108;
constexpr int BIN_TERM_A = 109;
constexpr int BIN_TERM_B = 110;
constexpr int BIN_TERM_C = 111;
constexpr int BIN_TERM_C_SEP = 112;

static int ParseECIValue(const ByteArray& codewords, int& position)
{
	int length = Size(codewords);
	if (position + 1 == length) {
		Diagnostics::put("Error(ECI)");
		return 0;
	}
	int firstByte = codewords[++position];
	if (firstByte < 40) {
		return firstByte;
	}
	if (position + 2 >= length) {
		Diagnostics::put("Error(ECI40)");
		return 0;
	}
	int secondByte = codewords[++position];
	int thirdByte = codewords[++position];
	return (firstByte - 40) * 12769 + secondByte * 113 + thirdByte + 40;
}

static void ParseStructuredAppend(ByteArray& codewords, StructuredAppendInfo& sai)
{
	if (codewords.back() != C_FNC2 || codewords.size() < 3 + 1) { // 3 + initial mask codeword
		return;
	}
	int firstByte = codewords[codewords.size() - 3];
	int secondByte = codewords[codewords.size() - 2];

	codewords.resize(codewords.size() - 3); // Chop off

	// Assuming Code Set A or B (doesn't matter which)
	if (firstByte >= 16 && firstByte <= 25) { // 1-9
		sai.index = firstByte - 16;
	} else if (firstByte >= 33 && firstByte <= 58) { // A-Z
		sai.index = 9 + firstByte - 33;
	} else {
		Diagnostics::put("Error(SAIndexError)");
		return;
	}

	if (secondByte >= 16 && secondByte <= 25) { // 1-9
		sai.count = 1 + secondByte - 16;
	} else if (secondByte >= 33 && secondByte <= 58) { // A-Z
		sai.count = 10 + secondByte - 33;
	} else {
		Diagnostics::put("Error(SACountError)");
		return;
	}

	if (sai.count == 1) {
		Diagnostics::put("SASizeError");
	}
	if (sai.count == 1 || sai.count <= sai.index) { // If info doesn't make sense
		sai.count = 0; // Choose to mark count as unknown
	}
	Diagnostics::fmt("SA(%d,%d)", sai.index, sai.count);
	// No id
}

static void AppendBinaryArray(std::vector<uint16_t>& binary, uint64_t& b103, int& b103_cnt)
{
	int b259s[5];

	for (int i = b103_cnt - 2; i >= 0; i--) {
		b259s[i] = (int)(b103 % 259);
		b103 /= 259;
	}
	for (int i = 0; i < b103_cnt - 1; i++) {
		binary.push_back(b259s[i]);
	}
	b103 = 0;
	b103_cnt = 0;
}

static void ProcessBinaryArray(std::vector<uint16_t>& binary, uint64_t& b103, int& b103_cnt,
							   std::wstring& resultEncoded, std::string& result,
							   CharacterSet& encoding, StructuredAppendInfo& sai)
{
	AppendBinaryArray(binary, b103, b103_cnt);

	for (int i = 0, len = Size(binary); i < len; i++) {
		if (binary[i] < 256) {
			result.push_back((char)binary[i]);
			Diagnostics::chr(result.back(), "", true/*appendHex*/);
		} else {
			int cnt = 1 + binary[i] - 256;
			if (i + cnt >= len) {
				Diagnostics::put("Error(BinECI)");
			} else {
				int eci;
				if (cnt == 1) {
					eci = binary[i + 1];
				// Assuming big-endian byte order
				} else if (cnt == 2) {
					eci = (binary[i + 1] << 8) | binary[i + 2];
				} else {
					eci = (binary[i + 1] << 16) | (binary[i + 2] << 8) | binary[i + 3];
				}
				encoding = CharacterSetECI::OnChangeAppendReset(eci, resultEncoded, result, encoding, &sai.lastECI);
			}
			i += cnt;
		}
	}
	binary.clear();
}

static void ProcessBinary(const ByteArray& codewords, int& position, std::wstring& resultEncoded, std::string& result,
						 CharacterSet& encoding, StructuredAppendInfo& sai, int& codeset)
{
	int length = Size(codewords);
	uint64_t b103 = 0;
	int b103_cnt = 0;

	std::vector<uint16_t> binary;
	binary.reserve(length - position);

	while (++position < length) {
		int code = codewords[position];
		if (code <= 102) {
			b103 *= 103;
			b103 += code;
			if (++b103_cnt == 6) {
				AppendBinaryArray(binary, b103, b103_cnt);
			}
		} else {
			ProcessBinaryArray(binary, b103, b103_cnt, resultEncoded, result, encoding, sai);

			if (code <= BIN_7SHIFT_C) {
				Diagnostics::fmt("BIN_%dSHIFTC", 2 + code - BIN_2SHIFT_C);
				int end_position = std::min(position + 2 + code - BIN_2SHIFT_C + 1, length);
				while (++position < end_position) {
					int c_code = codewords[position];
					if (c_code < 10) {
						result.push_back('0');
					}
					result.append(std::to_string(c_code));
					Diagnostics::fmt("%02d", c_code);
				}
				position--;
			} else {
				switch (code) {
				case BIN_TERM_A:
					codeset = CODESET_A;
					Diagnostics::put("BIN_TERMA");
					break;
				case BIN_TERM_B:
					codeset = CODESET_B;
					Diagnostics::put("BIN_TERMB");
					break;
				case BIN_TERM_C:
					codeset = CODESET_C;
					Diagnostics::put("BIN_TERMC");
					break;
				case BIN_TERM_C_SEP:
					codeset = CODESET_C;
					// Symbol separation - ignore for now
					Diagnostics::put("BIN_TERMC_SEP");
					break;
				}
				break; // Exit while
			}
		}
	}
	ProcessBinaryArray(binary, b103, b103_cnt, resultEncoded, result, encoding, sai);
}

ZXING_EXPORT_TEST_ONLY
DecoderResult Decode(ByteArray&& codewords, const std::string& characterSet)
{
	std::string result;
	std::wstring resultEncoded;
	std::string macroEnd;
	int position;
	int codeset = CODESET_C;
	int shift = 0, shift_cnt = 0;
	int position_1710 = 0;
	int position_macro = 2; // Need LATCH_B so 2nd codeword (excluding initial mask codeword)
	bool set_position_macro;
	bool gs1 = true, aim = false;
	bool readerInit = false;
	StructuredAppendInfo sai;
	CharacterSet encoding = CharacterSetECI::InitEncoding(characterSet);

	ParseStructuredAppend(codewords, sai);
	int length = Size(codewords);

	for (position = 1; position < length; position++) {
		int code = codewords[position];
		if (code >= C_FNC1) { // Common to all codesets
			switch (code) {
			case C_FNC1:
				if (position == 1) {
					gs1 = false;
					position_macro++;
					Diagnostics::put("FNC1(Pos1)");
				} else if ((position == 2 && codewords[1] <= 99)
							|| (position == 3 && codewords[1] == CC_LATCH_A && codewords[2] >= 33 && codewords[2] <= 58)
							|| (position == 3 && codewords[1] >= CC_SHIFT_B && codewords[1] <= CC_LATCH_B
								&& ((codewords[2] >= 33 && codewords[2] <= 58)
									|| (codewords[2] >= 65 && codewords[2] <= 90)))) {
					// AIM Application Indicator format
					aim = true;
					Diagnostics::fmt("FNC1(Pos%d)", position);
					if (position == 2) {
						if (codewords[1] < 10) {
							result.push_back('0');
						}
						result.append(std::to_string(codewords[1]));
						Diagnostics::fmt("%02d", codewords[1]);
					} else if (position == 3 && codewords[1] == CC_LATCH_A) {
						result.push_back((char)(codewords[1] + 32));
						Diagnostics::chr(result.back());
					} else {
						result.push_back((char)(codewords[1] + 32));
						Diagnostics::chr(result.back());
					}
				} else {
					result.push_back('\x1D');
					Diagnostics::put("FNC1(<GS>)");
				}
				break;
			case C_FNC2:
				set_position_macro = position + 2 == position_macro;
				// Note Structured Append FNC2 removed above if present, so only ECI
				encoding = CharacterSetECI::OnChangeAppendReset(ParseECIValue(codewords, position),
																resultEncoded, result, encoding, &sai.lastECI);
				if (set_position_macro) {
					position_macro = position + 2; // Need LATCH_B so 2nd codeword (excluding initial mask codeword)
				}
				break;
			case C_FNC3:
				if (position == 1) {
					readerInit = true;
					Diagnostics::put("FNC3(ReaderInit)");
				} else {
					// Symbol separation - ignore for now
					Diagnostics::put("FNC3");
				}
				break;
			case C_UPPERSHIFT_A:
				if (position < length) {
					Diagnostics::put("UPSHA");
					code = codewords[++position];
					if (code < 64) {
						result.push_back((char)((code + 32) | 0x80));
						Diagnostics::chr(result.back());
					} else if (code < 96) {
						result.push_back((char)((code - 64) | 0x80));
						Diagnostics::chr(result.back());
					} else {
						Diagnostics::put("Error(UPSHA)");
					}
				}
				break;
			case C_UPPERSHIFT_B:
				if (position < length) {
					Diagnostics::put("UPSHB");
					code = codewords[++position];
					if (code < 96) {
						result.push_back((char)((code + 32) | 0x80));
						Diagnostics::chr(result.back());
					} else {
						Diagnostics::put("Error(UPSHB)");
					}
				}
				break;
			case C_BINARY_LATCH:
				Diagnostics::put("BIN_LATCH");
				if (position + 1 < length) {
					ProcessBinary(codewords, position, resultEncoded, result, encoding, sai, codeset);
				}
				break;
			}

		} else if ((shift == 0 && codeset == CODESET_C) || shift == CODESET_C) {
			if (position == position_1710) {
				result.append("10");
				position_1710 = 0;
				Diagnostics::put("10(1710)");
			}
			if (code < 100) {
				if (code < 10) {
					result.push_back('0');
				}
				result.append(std::to_string(code));
				Diagnostics::fmt("%02d", code);
			} else {
				if (position == 1 && code != CC_1710) {
					gs1 = false;
					Diagnostics::put("NotCNotGS1(Pos1)");
				}
				switch (code) {
				case CC_1710:
					result.append("17");
					position_1710 = position + 1 + 3;
					Diagnostics::put("17(1710)");
					break;
				case CC_LATCH_A:
					codeset = CODESET_A;
					shift = shift_cnt = position_1710 = 0;
					Diagnostics::put("LATCHA");
					break;
				case CC_SHIFT_B:
				case CC_2SHIFT_B:
				case CC_3SHIFT_B:
				case CC_4SHIFT_B:
					shift = CODESET_B;
					shift_cnt = 1 + code - CC_SHIFT_B + 1; // +1 to counter decrement below
					position_1710 = 0;
					Diagnostics::fmt("%dSHIFTB", shift_cnt - 1);
					break;
				case CC_LATCH_B:
					codeset = CODESET_B;
					shift = shift_cnt = position_1710 = 0;
					Diagnostics::put("LATCHB");
					break;
				}
			}

		} else if ((shift == 0 && codeset == CODESET_A) || shift == CODESET_A) {
			if (code < 96) {
				if (code < 64) {
					result.push_back((char)(code + 32));
				} else {
					result.push_back((char)(code - 64));
				}
				Diagnostics::chr(result.back());
			} else {
				switch (code) {
				case CA_SHIFT_B:
				case CA_2SHIFT_B:
				case CA_3SHIFT_B:
				case CA_4SHIFT_B:
				case CA_5SHIFT_B:
				case CA_6SHIFT_B:
					shift = CODESET_B;
					shift_cnt = 1 + code - CA_SHIFT_B + 1; // +1 to counter decrement below
					Diagnostics::fmt("%dSHIFTB", shift_cnt - 1);
					break;
				case CA_LATCH_B:
					codeset = CODESET_B;
					shift = shift_cnt = 0;
					Diagnostics::put("LATCHB");
					break;
				case CA_2SHIFT_C:
				case CA_3SHIFT_C:
				case CA_4SHIFT_C:
					shift = CODESET_C;
					shift_cnt = 2 + code - CA_2SHIFT_C + 1; // +1 to counter decrement below
					Diagnostics::fmt("SHIFTC%d", shift_cnt - 1);
					break;
				case CA_LATCH_C:
					codeset = CODESET_C;
					shift = shift_cnt = 0;
					Diagnostics::put("LATCHC");
					break;
				}
			}

		} else if ((shift == 0 && codeset == CODESET_B) || shift == CODESET_B) {
			if (code < 96) {
				result.push_back((char)(code + 32));
				Diagnostics::chr(result.back());
			} else {
				// Table 2 Code Set B dual-function macros/control chars
				static const char* macroBegins[] = { "[)>\03605\035", "[)>\03606\035", "[)>\03612\035", "[)>\036" };
				static const char* macroEnds[] = { "\036\004", "\036\004", "\036\004", "\004" };
				static const char ctrls[] = { '\x09', '\x1C', '\x1D', '\x1E' }; // HT, FS, GS, RS
				switch (code) {
				case CB_CRLF:
					result.append("\x0D\x0A");
					Diagnostics::put("CRLF");
					break;
				case CB_HT:
				case CB_FS:
				case CB_GS:
				case CB_RS:
					if (position == position_macro) {
						// Macros
						result.append(macroBegins[code - CB_HT]);
						macroEnd = macroEnds[code - CB_HT];
						Diagnostics::fmt("Macro%d", code);
					} else {
						result.push_back(ctrls[code - CB_HT]);
						Diagnostics::chr(result.back());
					}
					break;
				case CB_SHIFT_A:
					shift = CODESET_A;
					shift_cnt = 1 + 1; // +1 to counter decrement below
					Diagnostics::put("SHIFTA");
					break;
				case CB_LATCH_A:
					codeset = CODESET_A;
					shift = shift_cnt = 0;
					Diagnostics::put("LATCHA");
					break;
				case CB_2SHIFT_C:
				case CB_3SHIFT_C:
				case CB_4SHIFT_C:
					shift = CODESET_C;
					shift_cnt = 2 + code - CB_2SHIFT_C + 1; // +1 to counter decrement below
					Diagnostics::fmt("%dSHIFTC", shift_cnt - 1);
					break;
				case CB_LATCH_C:
					codeset = CODESET_C;
					shift = shift_cnt = 0;
					Diagnostics::put("LATCHC");
					break;
				}
			}
		}
		if (shift && shift_cnt) {
			shift_cnt--;
			if (shift_cnt == 0) {
				shift = 0;
				Diagnostics::put("SHEND");
			}
		}
	}
	if (position == position_1710) {
		result.append("10");
		position_1710 = 0;
		Diagnostics::put("10(1710)");
	}

	if (!macroEnd.empty()) {
		result.append(macroEnd);
	}

	TextDecoder::Append(resultEncoded, reinterpret_cast<const uint8_t*>(result.data()), result.size(), encoding);

	// As converting character set ECIs ourselves and ignoring/skipping non-character ECIs, not using modifiers
	// that indicate ECI protocol (ISO/IEC 16023:2000 Annexe E Table E1)
	std::string symbologyIdentifier;
	if (gs1) {
		symbologyIdentifier = "]U1";
	} else if (aim) {
		symbologyIdentifier = "]U2";
	} else {
		symbologyIdentifier = "]U0";
	}

	return DecoderResult(std::move(codewords), std::move(resultEncoded))
			.setSymbologyIdentifier(std::move(symbologyIdentifier))
			.setStructuredAppend(sai)
			.setReaderInit(readerInit);
}

} // DecodedBitStreamParser

static bool
CorrectErrors(const GField& field, ByteArray& codewordBytes, int numDataCodewords)
{
	// First read into an array of ints
	std::vector<int> codewordsInts(codewordBytes.begin(), codewordBytes.end());
	int numECCodewords = Size(codewordBytes) - numDataCodewords;

	Diagnostics::fmt("  DataCodewords: (%d)", numDataCodewords); Diagnostics::dump(codewordsInts, "\n", 0, numDataCodewords);
	Diagnostics::fmt("  ECCodewords:   (%d)", numECCodewords); Diagnostics::dump(codewordsInts, "\n", numDataCodewords);

	if (!ReedSolomonDecode(field, codewordsInts, numECCodewords)) {
		Diagnostics::put("Fail(RSDecode)");
		return false;
	}

	// Copy back into array of bytes -- only need to worry about the bytes that were data
	// We don't care about errors in the error-correction codewords
	std::copy_n(codewordsInts.begin(), numDataCodewords, codewordBytes.begin());

	return true;
}

static void
Unmask(ByteArray& codewords)
{
	int mask = codewords[0];

	if (mask) {
		int weight = 0;
		const int factor = mask == 1 ? 3 : mask == 2 ? 7 : 17;
		for (int i = 1; i < Size(codewords); i++) {
			if (codewords[i] < weight) {
				codewords[i] += GField::GF - weight;
			} else {
				codewords[i] -= weight;
			}
			weight = (weight + factor) % GField::GF;
		}
	}
}

DecoderResult
Decoder::Decode(const BitMatrix& bits, const std::string& characterSet)
{
	static GField field;
	std::vector<int> erasureLocs;

	ByteArray codewords = BitMatrixParser::ReadCodewords(bits, erasureLocs);
	if (codewords.size() == 0) {
		return DecodeStatus::NotFound;
	}

	Diagnostics::fmt("  Codewords:  (%d)", codewords.size()); Diagnostics::dump(codewords, "\n");

	auto dataBlocks = GetDataBlocks(codewords);

	// Count total number of data bytes
	ByteArray resultBytes(TransformReduce(dataBlocks, 0, [](const auto& db) { return db.numDataCodewords; }));

	// Error-correct and copy data blocks together into a stream of bytes
	const int dataBlocksCount = Size(dataBlocks);
	for (int j = 0; j < dataBlocksCount; j++) {
		auto& dataBlock = dataBlocks[j];
		ByteArray& codewordBytes = dataBlock.codewords;
		const int numDataCodewords = dataBlock.numDataCodewords;
		if (!CorrectErrors(field, codewordBytes, numDataCodewords)) {
			//printf(" checksum fail\n");
			//return DecodeStatus::ChecksumError;
			return DecodeStatus::NotFound;
		}

		for (int i = 0; i < numDataCodewords; i++) {
			// De-interlace data blocks.
			resultBytes[i * dataBlocksCount + j] = codewordBytes[i];
		}
	}

	Unmask(resultBytes);

	Diagnostics::fmt("  Unmasked:   (%d)", resultBytes.size()); Diagnostics::dump(resultBytes, "\n");
	Diagnostics::put("  Decode:     ");
	return DecodedBitStreamParser::Decode(std::move(resultBytes), characterSet);
}

} // namespace ZXing::DotCode
