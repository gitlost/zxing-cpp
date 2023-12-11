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

#include "HXDecoder.h"

#include "BitMatrix.h"
#include "BitSource.h"
#include "ByteArray.h"
#include "Content.h"
#include "DecoderResult.h"
#include "Diagnostics.h"
#include "GenericGF.h"
#include "HXBitMatrixParser.h"
#include "HXDataBlock.h"
#include "ReedSolomonDecoder.h"
#include "ZXTestSupport.h"

#include <vector>

namespace ZXing::HanXin {

namespace DecodedBitStreamParser {

// Table 1
constexpr int M_PAD       = 0x0;
constexpr int M_NUMERIC   = 0x1;
constexpr int M_TEXT      = 0x2;
constexpr int M_BINARY    = 0x3;
constexpr int M_REGION1   = 0x4;
constexpr int M_REGION2   = 0x5;
constexpr int M_BYTE2     = 0x6;
constexpr int M_BYTE4     = 0x7;
constexpr int M_ECI       = 0x8;
constexpr int M_UNIC      = 0x9;
constexpr int M_RESERVED2 = 0xA;
constexpr int M_RESERVED3 = 0xB;
constexpr int M_RESERVED4 = 0xC;
constexpr int M_RESERVED5 = 0xD;
constexpr int M_GS1_URI   = 0xE;
constexpr int M_RESERVED6 = 0xF;

const char text1[62] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
	'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
	'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};
const char text2[62] = {
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
	'\x08', '\x09', '\x0A', '\x0B', '\x0C', '\x0D', '\x0E', '\x0F',
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
	'\x18', '\x19', '\x1A', '\x1B',    ' ',    '!',    '"',    '#',
	   '$',    '%',    '&',   '\'',    '(',    ')',    '*',    '+',
	   ',',    '-',    '.',    '/',    ':',    ';',    '<',    '=',
	   '>',    '?',    '@',    '[',   '\\',    ']',    '^',    '_',
	   '`',    '{',    '|',    '}',    '~', '\x7F'
};

static ECI ParseECIValue(BitSource& bits)
{
	if (bits.available() < 8) {
		Diagnostics::put("Error(ECI)");
		throw std::runtime_error("ParseECIValue: first byte bits");
	}
	int firstByte = bits.readBits(8);
	if (firstByte < 0x80) {
		return ECI(firstByte);
	}
	if (bits.available() < 8) {
		Diagnostics::put("Error(ECI2)");
		throw std::runtime_error("ParseECIValue: second byte bits");
	}
	int secondByte = bits.readBits(8);
	if (firstByte < 0xc0) {
		return ECI((firstByte << 8) | secondByte);
	}
	if (bits.available() < 8) {
		Diagnostics::put("Error(ECI3)");
		throw std::runtime_error("ParseECIValue: third byte bits");
	}
	int thirdByte = bits.readBits(8);
	return ECI((firstByte << 16) | (secondByte << 8) | thirdByte);
}

ZXING_EXPORT_TEST_ONLY
DecoderResult Decode(ByteArray&& codewords, const CharacterSet optionsCharset, const int ecLevel)
{
	BitSource bits(codewords);
	Content result;
	result.optionsCharset = optionsCharset;
	result.defaultCharset = CharacterSet::ISO8859_1;
	bool gs1 = false, uri = false, uni = false;

	try
	{
		while (bits.available() >= 4) {
			int mode = bits.readBits(4);
			switch (mode) {
			case M_PAD: {
					int padding = 1;
					while (bits.available() >= 4) {
						mode = bits.readBits(4);
						if (mode != M_PAD) {
							Diagnostics::fmt("PERR(%d)", mode);
							return FormatError(); // Ignore for now
						}
						padding++;
					}
					Diagnostics::fmt("M_PAD(%d)", padding);
				}
				break;
			case M_RESERVED2:
			case M_RESERVED3:
			case M_RESERVED4:
			case M_RESERVED5:
			case M_RESERVED6:
				Diagnostics::fmt("RERR(0x%X)", mode);
				return FormatError();
				break;
			case M_NUMERIC: {
					Diagnostics::put("NUM");
					int prevNum = -1;
					for (;;) {
						if (bits.available() < 10) {
							Diagnostics::fmt("NERR(%d)", bits.available());
							return FormatError();
						}
						char buf[32];
						int num = bits.readBits(10);
						if (num >= 0x3FD) {
							if (prevNum == -1) {
								Diagnostics::put("NERR0");
								return FormatError();
							}
							snprintf(buf, sizeof(buf), num == 0x3FD ? "%d" : num == 0x3FE ? "%02d" : "%03d", prevNum);
							result.append(buf);
							Diagnostics::put("NTERM");
							break;
						}
						if (prevNum != -1) {
							snprintf(buf, sizeof(buf), "%03d", prevNum);
							result.append(buf);
						}
						prevNum = num;
					}
				}
				break;
			case M_TEXT: {
					Diagnostics::put("TXT");
					bool submode = false;
					for (;;) {
						if (bits.available() < 6) {
							Diagnostics::fmt("TERR(%d)", bits.available());
							return FormatError();
						}
						int code = bits.readBits(6);
						if (code == 0x3F) {
							Diagnostics::put("TTERM");
							break;
						}
						if (code == 0x3E) {
							submode = !submode;
						} else {
							result.push_back(submode ? text2[code] : text1[code]);
						}
					}
				}
				break;
			case M_BINARY: {
					if (bits.available() < 13) {
						Diagnostics::fmt("BCERR(%d)", bits.available());
						return FormatError();
					}
					int cnt = bits.readBits(13);
					Diagnostics::fmt("BIN(%d)", cnt);
					for (int i = 0; i < cnt; i++) {
						if (bits.available() < 8) {
							Diagnostics::fmt("BERR(%d)", bits.available());
							return FormatError();
						}
						result.push_back(bits.readBits(8));
					}
					Diagnostics::put("BTERM");
				}
				break;
			case M_REGION1:
			case M_REGION2: {
					Diagnostics::put(mode == M_REGION2 ? "RG2" : "RG1");
					bool submode = mode == M_REGION2;
					for (;;) {
						if (bits.available() < 12) {
							Diagnostics::fmt("BCERR(%d)", bits.available());
							return FormatError();
						}
						int code = bits.readBits(12);
						if (code == 0xFFF) {
							Diagnostics::put("RGTERM");
							break;
						}
						if (code == 0xFFE) {
							submode = !submode;
							Diagnostics::fmt("RGSW%d", submode ? 2 : 1);
						} else {
							if (submode) {
								result.push_back(code / 0x5E + 0xD8);
								result.push_back(code % 0x5E + 0xA1);
							} else {
								if (code < 0xEB0) {
									result.push_back(code / 0x5E + 0xB0);
									result.push_back(code % 0x5E + 0xA1);
								} else if (code < 0xFCA) {
									result.push_back((code - 0xEB0) / 0x5E + 0xA1);
									result.push_back((code - 0xEB0) % 0x5E + 0xA1);
								} else {
									code = code - 0xFCA + 0xA8A1;
									result.push_back(code >> 8);
									result.push_back(code & 0xFF);
								}
							}
						}
					}
				}
				break;
			case M_BYTE2: {
					Diagnostics::put("BY2");
					for (;;) {
						if (bits.available() < 15) {
							Diagnostics::fmt("BY2ERR(%d)", bits.available());
							return FormatError();
						}
						int code = bits.readBits(15);
						if (code == 0x7FFF) {
							Diagnostics::put("BY2TERM");
							break;
						}
						result.push_back(code / 0xBE + 0x81);
						int secondByte = code % 0xBe;
						result.push_back(secondByte + 0x40 + (int)(secondByte >= 0x3F));
					}
				}
				break;
			case M_BYTE4: {
					Diagnostics::put("BY4");
					if (bits.available() < 21) {
						Diagnostics::fmt("BY4ERR(%d)", bits.available());
						return FormatError();
					}
					int code = bits.readBits(21);
					result.push_back(code / 0x3138 + 0x81);
					result.push_back((code % 0x3138) / 0x4EC + 0x30);
					result.push_back((code % 0x4EC) / 0x0A + 0x81);
					result.push_back((code % 0x0A) + 0x30);
				}
				break;
			case M_ECI:
				result.switchEncoding(ParseECIValue(bits));
				break;
			case M_UNIC:
				Diagnostics::put("UNIC");
				uni = true;
				// Not implemented
				return FormatError();
				break;
			case M_GS1_URI: {
					Diagnostics::put("GS1URI");
					if (bits.available() < 4) {
						Diagnostics::fmt("GS1URIERR(%d)", bits.available());
						return FormatError();
					}
					int submode = bits.readBits(4);
					if (submode == 1) { // GS1
						gs1 = true;
						// Not implemented
						return FormatError();
					} else if (submode == 2) { // URI
						uri = true;
						// Not implemented
						return FormatError();
					} else {
						Diagnostics::fmt("GS1URISMERR(%d)", submode);
						return FormatError();
					}
				}
				break;
			default:
				Diagnostics::fmt("MERR(%d)", mode);
				return FormatError();
				break;
			}
		}
	}
	catch (const std::exception &)
	{
		// from readBits() calls
		return FormatError();
	}

	char modifier;
	if (gs1) {
		modifier = '2';
	} else if (uri) {
		modifier = '4';
	} else if (uni) {
		modifier = '8';
	} else {
		modifier = '0';
	}
	result.symbology = {'h', modifier, narrow_cast<char>('1' - modifier /*eciModifierOffset*/)}; // If ECI always '1'

	return DecoderResult(std::move(result))
			.setEcLevel("L" + std::to_string(ecLevel));
}

} // DecodedBitStreamParser

static bool
CorrectErrors(ByteArray& codewordBytes, int numDataCodewords)
{
	// First read into an array of ints
	std::vector<int> codewordsInts(codewordBytes.begin(), codewordBytes.end());
	int numECCodewords = Size(codewordBytes) - numDataCodewords;

	if (!ReedSolomonDecode(GenericGF::HanXinField256(), codewordsInts, numECCodewords)) {
		Diagnostics::put("Fail(RSDecode)");
		return false;
	}

	// Copy back into array of bytes -- only need to worry about the bytes that were data
	// We don't care about errors in the error-correction codewords
	std::copy_n(codewordsInts.begin(), numDataCodewords, codewordBytes.begin());

	return true;
}

DecoderResult
Decoder::Decode(const BitMatrix& bits, const CharacterSet optionsCharset)
{
	int version;
	int ecLevel;
	int mask;

	ByteArray codewords = BitMatrixParser::ReadCodewords(bits, version, ecLevel, mask);
	if (codewords.size() == 0) {
		return {};
	}

	Diagnostics::fmt("  Version:    %d (%dx%d)\n", version, bits.height(), bits.width());
	Diagnostics::fmt("  Mask:       %c%c\n", mask & 2 ? '1' : '0', mask & 1 ? '1' : '0');
	Diagnostics::fmt("  Codewords:  (%d)", codewords.size());
	Diagnostics::dump(codewords, "\n", -1, -1, true /*hex*/);

	auto dataBlocks = GetDataBlocks(codewords, version, ecLevel);

	// Count total number of data bytes
	ByteArray resultBytes(TransformReduce(dataBlocks, 0, [](const auto& db) { return db.numDataCodewords; }));

	auto resultIterator = resultBytes.begin();

	// Error-correct and copy data blocks together into a stream of bytes
	for (auto& dataBlock : dataBlocks) {
		ByteArray& codewordBytes = dataBlock.codewords;
		const int numDataCodewords = dataBlock.numDataCodewords;
		if (!CorrectErrors(codewordBytes, numDataCodewords)) {
			printf(" checksum fail\n");
			return ChecksumError();
		}
		resultIterator = std::copy_n(codewordBytes.begin(), numDataCodewords, resultIterator);
	}
	Diagnostics::fmt("  Datawords:  (%d)", resultBytes.size());
	Diagnostics::dump(resultBytes, "\n", -1, -1, true /*hex*/);

	Diagnostics::put("  Decode:     ");
	return DecodedBitStreamParser::Decode(std::move(resultBytes), optionsCharset, ecLevel);
}

} // namespace ZXing::HanXin
