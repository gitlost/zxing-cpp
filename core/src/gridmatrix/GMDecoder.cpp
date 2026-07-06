/*
* Copyright 2026 gitlost
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

#include "GMDecoder.h"

#include "Barcode.h"
#include "BitArray.h"
#include "BitMatrix.h"
#include "BitSource.h"
#include "ByteArray.h"
#include "Content.h"
#include "DecoderResult.h"
#include "Diagnostics.h"
#include "GMBitMatrixParser.h"
#include "GMDataBlock.h"
#include "JSON.h"
#include "ReedSolomon.h"
#include "ZXTestSupport.h"

#include <vector>

namespace ZXing::GridMatrix {

namespace DecodedBitStreamParser {

constexpr int GM_CHINESE  = 0x01;
constexpr int GM_NUMERAL  = 0x02;
constexpr int GM_LOWER    = 0x03;
constexpr int GM_UPPER    = 0x04;
constexpr int GM_MIXED    = 0x05;
constexpr int GM_BYTE     = 0x07;

constexpr int GM_FNC1_GS1 = 0x08;
constexpr int GM_FNC2     = 0x09;
constexpr int GM_FNC3     = 0x0A;
constexpr int GM_FNC1_AIM = 0x0B;
constexpr int GM_ECI      = 0x0C;

/* From Table 7 - Encoding of control characters */
static const char gm_shift_set[64] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* NULL -> SI */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* DLE -> US */
     '!',  '"',  '#',  '$',  '%',  '&', '\'',  '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',  ':',
     ';',  '<',  '=',  '>',  '?',  '@',  '[', '\\',  ']',  '^',  '_',  '`',  '{',  '|',  '}',  '~'
};

static ECI ParseECIValue(BitSource& bits)
{
	if (bits.available() < 11) {
		Diagnostics::put("Error(ECI)");
		throw FormatError("ParseECIValue: first bits");
	}
	int firstBits = bits.readBits(11);
	if (!(firstBits & 0x400)) {
		return ECI(firstBits);
	}
	if (!(firstBits & 0x200)) {
		if (bits.available() < 6) {
			Diagnostics::put("Error(ECI2)");
			throw FormatError("ParseECIValue: second 6 bits");
		}
		int secondBits = bits.readBits(6);
		return ECI(((firstBits & 0x1FF) << 6) | secondBits);
	}
	if (bits.available() < 11) {
		Diagnostics::put("Error(ECI3)");
		throw FormatError("ParseECIValue: second 11 bits");
	}
	int secondBits = bits.readBits(11);
	return ECI(((firstBits & 0x1FF) << 11) | secondBits);
}

static void DoNumeric(Content &result, int &last_d1, int last_d2, int last_d3, int &last_nondigit, int last_nondigit_posn, const int padding)
{
	if (last_nondigit != -1) {
		assert(last_nondigit_posn >= 0 && last_nondigit_posn <= 2);
		if (last_nondigit_posn == 0) {
			result.push_back(last_nondigit);
			if (last_nondigit == 13) {
				result.push_back('\n');
			}
		}
		result.push_back(last_d1 + '0');
		if (last_nondigit_posn == 1) {
			result.push_back(last_nondigit);
			if (last_nondigit == 13) {
				result.push_back('\n');
			}
		}
		if (padding != 2) {
			result.push_back(last_d2 + '0');
		}
		if (last_nondigit_posn == 2) {
			result.push_back(last_nondigit);
			if (last_nondigit == 13) {
				result.push_back('\n');
			}
		}
		if (!padding) {
			result.push_back(last_d3 + '0');
		}
	} else {
		result.push_back(last_d1 + '0');
		if (padding != 2) {
			result.push_back(last_d2 + '0');
			if (!padding) {
				result.push_back(last_d3 + '0');
			}
		}
	}
	last_d1 = -1;
	last_nondigit = -1;
}

ZXING_EXPORT_TEST_ONLY
DecoderResult Decode(ByteArray&& codewords, const CharacterSet optionsCharset, const int ecLevel)
{
	BitArray bits7;
	for (int i = 0, size = Size(codewords); i < size; i++) {
		bits7.appendBits(codewords[i], 7);
	}
	ByteArray bytes7 = bits7.toBytes();

	BitSource bits(bytes7);
	Content result;
	result.optionsCharset = optionsCharset;
	result.defaultCharset = CharacterSet::GB2312;
	Error error;

	struct StructuredAppendInfo sai;
	bool eci = false, gs1 = false, aim = false, readerInit = false, structApp = false;
	int mode = 0;

	try
	{
		while (mode != -1) {
			if (mode == 0 && bits.available() >= 4) {
				mode = bits.readBits(4);
			}
			switch (mode) {
			case GM_CHINESE: {
					Diagnostics::put("CHN");
					if (bits.available() < 13) {
						Diagnostics::fmt("CERR(%d)", bits.available());
						throw FormatError("Chinese: insufficient bits");
					}
					int val = bits.readBits(13);
					if (val < 7776) {
						int v1 = val / 0x60;
						int v2 = val - v1 * 0x60;
						if (v1 < 9) {
							result.push_back(v1 + 0xA1);
						} else {
							result.push_back(v1 - 9 + 0xB0);
						}
						result.push_back(v2 + 0xA0);
					} else {
						if (val == 7776) {
							result.push_back('\r');
							result.push_back('\n');
						} else if (val < 8033) {
							result.push_back(val - 7777);
						} else if (val < 8133) {
							int num = val - 8033;
							result.push_back((num / 10) + '0');
							result.push_back((num % 10) + '0');
						} else if (val < 8166) {
							if (val == 8160) {
								mode = 0;
							} else if (val < 8165) {
								mode = val - 8159; // GM_NUMERAL, GM_LOWER, GM_UPPER, GM_MIXED
							} else {
								mode = GM_BYTE;
							}
							break;
						} else {
							Diagnostics::fmt("CERR(%d)", val);
							throw FormatError("Chinese: unknown value");
						}
					}
				}
				break;
			case GM_NUMERAL: {
					Diagnostics::put("NUM");
					if (bits.available() < 2) {
						Diagnostics::fmt("NERR(%d)", bits.available());
						throw FormatError("Numeral: insufficient bits for counter prefix");
					}
					int padding = bits.readBits(2);
					if (padding == 3) {
						Diagnostics::fmt("DPADERR(%d)", padding);
						throw FormatError("Numeral: invalid counter prefix");
					}
					int last_d1 = -1, last_d2, last_d3;
					int last_nondigit = -1, last_nondigit_posn;
					for (;;) {
						if (bits.available() < 10) {
							Diagnostics::fmt("NERR(%d)", bits.available());
							throw FormatError("Numeral: insufficient bits for value");
						}
						int num = bits.readBits(10);
						if (num < 1000) {
							if (last_d1 != -1) {
								DoNumeric(result, last_d1, last_d2, last_d3, last_nondigit, last_nondigit_posn, 0 /*padding*/);
							}
							last_d1 = num / 100;
							last_d2 = (num - last_d1 * 100) / 10;
							last_d3 = num - (last_d1 * 100 + last_d2 * 10);
						} else if (num < 1018) {
							if (last_d1 != -1) {
								DoNumeric(result, last_d1, last_d2, last_d3, last_nondigit, last_nondigit_posn, 0 /*padding*/);
							}
							if (num < 1003) {
								last_nondigit = ' ';
								last_nondigit_posn = num - 1000;
							} else if (num < 1006) {
								last_nondigit = '+';
								last_nondigit_posn = num - 1003;
							} else if (num < 1009) {
								last_nondigit = '-';
								last_nondigit_posn = num - 1006;
							} else if (num < 1012) {
								last_nondigit = '.';
								last_nondigit_posn = num - 1009;
							} else if (num < 1015) {
								last_nondigit = ',';
								last_nondigit_posn = num - 1012;
							} else {
								last_nondigit = '\r';
								last_nondigit_posn = num - 1015;
							}
						} else {
							if (num == 1018) {
								mode = 0; /* End of data */
							} else if (num == 1019) {
								mode = GM_CHINESE;
							} else if (num < 1023) {
								mode = num - 1017; // GM_LOWER, GM_UPPER, GM_MIXED
							} else /* 1023 */ {
								mode = GM_BYTE;
							}
							break;
						}
					}
					if (last_d1 != -1) {
						DoNumeric(result, last_d1, last_d2, last_d3, last_nondigit, last_nondigit_posn, padding);
					}
				}
				break;
			case GM_LOWER:
			case GM_UPPER: {
					Diagnostics::put(mode == GM_UPPER ? "UPR" : "LWR");
					const char *err_prefix = mode == GM_UPPER ? "UP": "LW";
					const char base_ch = mode == GM_UPPER ? 'A' : 'a';
					for (;;) {
						if (bits.available() < 5) {
							Diagnostics::fmt("%sERR(%d)", err_prefix, bits.available());
							throw FormatError("Lower/Upper: insufficient bits");
						}
						int val = bits.readBits(5);
						if (val <= 26) {
							if (val == 26) {
								result.push_back(' ');
							} else {
								result.push_back(base_ch + val);
							}
						} else if (val < 30) {
							mode = val - 27; // End of data, GM_CHINESE, GM_NUMERAL
							break;
						} else if (val == 30) {
							mode = mode == GM_UPPER ? GM_LOWER : GM_UPPER;
							break;
						} else /* 31 */ {
							if (bits.available() < 2) {
								Diagnostics::fmt("%sERR(%d)", err_prefix, bits.available());
								throw FormatError("Lower/Upper: insufficient bits for shift or Mixed/Byte latch");
							}
							val = bits.readBits(2);
							if (val == 0 /*124*/) {
								mode = GM_MIXED;
								break;
							}
							if (val == 1 /*125*/) {
								if (bits.available() < 6) {
									throw FormatError("Lower/Upper: insufficient bits for shift");
								}
								int shift = bits.readBits(6);
								result.push_back(gm_shift_set[shift]);
								break;
							}
							if (val == 2 /*126*/) {
								mode = GM_BYTE;
								break;
							}
							if (val == 3 /*127*/) {
								throw FormatError("Lower/Upper: invalid shift or Mixed/Byte latch");
							}
						}
					}
				}
				break;
			case GM_MIXED: {
					Diagnostics::put("MXD");
					for (;;) {
						if (bits.available() < 6) {
							Diagnostics::fmt("MXERR(%d)", bits.available());
							throw FormatError("Mixed: insufficient bits");
						}
						int val = bits.readBits(6);
						if (val == 63) {
							if (bits.available() < 4) {
								Diagnostics::fmt("MXERR(%d)", bits.available());
								throw FormatError("Mixed: insufficient bits for latch/shift/EOD");
							}
							val = bits.readBits(4);
							if (val == 0 /*1008*/) {
								mode = 0; /* End of data */
								break;
							}
							if (val == 1 /*1009*/) {
								mode = GM_CHINESE;
								break;
							}
							if (val == 2 /*1010*/) {
								mode = GM_NUMERAL;
								break;
							}
							if (val == 3 /*1011*/) {
								mode = GM_LOWER;
								break;
							}
							if (val == 4 /*1012*/) {
								mode = GM_UPPER;
								break;
							}
							if (val == 5 /*1013*/) {
								Diagnostics::fmt("MXERR(%d)", val);
								throw FormatError("Mixed: invalid latch/shift");
							}
							if (val == 6 /*1014*/) {
								if (bits.available() < 6) {
									Diagnostics::fmt("MXERR(%d)", bits.available());
									throw FormatError("Mixed: insufficient bits for shift");
								}
								int shift = bits.readBits(6);
								result.push_back(gm_shift_set[shift]);
							} else if (val == 7 /*1015*/) {
								mode = GM_BYTE;
								break;
							}
						} else {
							if (val < 10) {
								result.push_back(val + '0');
							} else if (val < 36) {
								result.push_back(val - 10 + 'A');
							} else if (val < 62) {
								result.push_back(val - 36 + 'a');
							} else {
								result.push_back(' ');
							}
						}
					}
				}
				break;
			case GM_BYTE: {
					if (bits.available() < 9) {
						Diagnostics::fmt("BNERR(%d)", bits.available());
						throw FormatError("Byte: insufficient bits for count");
					}
					int cnt = bits.readBits(9) + 1;
					Diagnostics::fmt("BIN(%d)", cnt);
					for (int i = 0; i < cnt; i++) {
						if (bits.available() < 8) {
							Diagnostics::fmt("BERR(%d)", bits.available());
							throw FormatError("Byte: insufficient bits for bytes");
						}
						result.push_back(bits.readBits(8));
					}
					if (bits.available() >= 8 && bits.peekBits(8) == GM_ECI) {
						// Need to check for 4-bit 0 end of data preceding ECI */
						(void) bits.readBits(4);
						Diagnostics::put("BNEND(0)");
					} else {
						Diagnostics::put("BNEND");
					}
					mode = 0;
				}
				break;
			case GM_FNC1_GS1:
				if (!gs1) {
					if (!result.empty()) {
						Diagnostics::fmt("GS1ERR(%d)", Size(result.bytes));
						throw FormatError("GS1: not first codeword");
					}
					gs1 = true;
				} else {
					result.push_back(0x1D); // GS
				}
				mode = 0;
				break;
			case GM_FNC1_AIM:
				if (!aim) {
					if (gs1) {
						Diagnostics::put("AIMERR");
						throw FormatError("AIM: cannot have both AIM and GS1");
					}
					if (result.empty() || Size(result.bytes) > 2 || (Size(result.bytes) == 1 && !IsAlpha(result.bytes[0]))
							|| (!IsDigit(result.bytes[0]) || !IsDigit(result.bytes[1]))) {
						Diagnostics::put("AIM2ERR");
						throw FormatError("AIM: invalid ID");
					}
					aim = true;
				} else {
					result.push_back(0x1D); // GS
				}
				mode = 0;
				break;
			case GM_FNC2:
				if (!structApp) {
					if (bits.available() < 16) {
						Diagnostics::fmt("FNC2ERR(%d)", bits.available());
						throw FormatError("Structured Append: insufficient bits");
					}
					int id = bits.readBits(8);
					sai.id = std::to_string(id);
					sai.count = bits.readBits(4) + 1;
					sai.index = bits.readBits(4); // 0-based
					if (sai.index > sai.count) {
						Diagnostics::fmt("SAIERR(%d,%d)", sai.index, sai.count);
						throw FormatError("Structured Append: invalid index/count");
					}
					structApp = true;
				} else {
					Diagnostics::put("SAI2ERR");
					throw FormatError("Structured Append: can only have one");
				}
				mode = 0;
				break;
			case GM_FNC3:
				readerInit = true;
				mode = 0;
				break;
			case GM_ECI:
				result.switchEncoding(ParseECIValue(bits));
				eci = true;
				mode = 0;
				break;
			case 0:
				mode = -1;
				break;
			default:
				Diagnostics::fmt("MERR(%d)", mode);
				throw FormatError("Unknown mode");
				break;
			}
		}
	} catch (Error e) {
		Diagnostics::fmt("FMTError(%s)", e.msg().c_str());
		error = std::move(e);
	}
	if (bits.available()) {
		Diagnostics::fmt("Pad(%d)", bits.available());
	}

	char modifier = gs1 ? '2' : aim ? '4' : '0';
	if (eci) {
		modifier++;
	}
	result.symbology = {'g', modifier, 1};

	return DecoderResult(std::move(result))
			.setError(std::move(error))
			.setEcLevel("L" + std::to_string(ecLevel))
			.setStructuredAppend(sai)
			.setReaderInit(readerInit);
}

} // DecodedBitStreamParser

static bool
CorrectErrors(ByteArray& codewordBytes, int numDataCodewords)
{
	// First read into an array of ints
	std::vector<int> codewordsInts(codewordBytes.begin(), codewordBytes.end());
	int numECCodewords = Size(codewordBytes) - numDataCodewords;

	if (!ReedSolomonDecode(RSField::GridMatrix, codewordsInts, numECCodewords)) {
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

	ByteArray codewords = BitMatrixParser::ReadCodewords(bits, version, ecLevel);
	if (codewords.size() == 0) {
		return {};
	}

	Diagnostics::fmt("  Version:    %d (%dx%d)\n", version, bits.height(), bits.width());
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
	return DecodedBitStreamParser::Decode(std::move(resultBytes), optionsCharset, ecLevel)
			.setVersionNumber(version);
}

} // namespace ZXing::GridMatrix
