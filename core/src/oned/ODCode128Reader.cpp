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

#include "ODCode128Reader.h"

#include "DecodeHints.h"
#include "Diagnostics.h"
#include "ODCode128Patterns.h"
#include "Result.h"
#include "ZXContainerAlgorithms.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace ZXing::OneD {

static const float MAX_AVG_VARIANCE = 0.25f;
static const float MAX_INDIVIDUAL_VARIANCE = 0.7f;

static const int CODE_SHIFT = 98;

static const int CODE_CODE_C = 99;
static const int CODE_CODE_B = 100;
static const int CODE_CODE_A = 101;

static const int CODE_FNC_1 = 102;
static const int CODE_FNC_2 = 97;
static const int CODE_FNC_3 = 96;

static const int CODE_START_A = 103;
static const int CODE_START_B = 104;
static const int CODE_START_C = 105;
static const int CODE_STOP = 106;

class Raw2TxtDecoder
{
	int codeSet = 0;
	std::string _symbologyIdentifier = "]C0"; // ISO/IEC 15417:2007 Annex C Table C.1
	bool _readerInit = false;
	std::string txt;
	size_t lastTxtSize = 0;

	bool fnc4All = false;
	bool fnc4Next = false;
	bool shift = false;

	void fnc1(const bool isCodeSetC)
	{
		if (txt.empty()) {
			// ISO/IEC 15417:2007 Annex B.1 and GS1 General Specifications 21.0.1 Section 5.4.3.7
			// If the first char after the start code is FNC1 then this is GS1-128.
			_symbologyIdentifier = "]C1";
			// GS1 General Specifications Section 5.4.6.4
			// "Transmitted data ... is prefixed by the symbology identifier ]C1, if used."
			// Choosing not to use symbology identifier, i.e. to not prefix to data.
			Diagnostics::put("FNC1(GS1)");
		}
		else if ((isCodeSetC && txt.size() == 2 && txt[0] >= '0' && txt[0] <= '9' && txt[1] >= '0' && txt[1] <= '9')
				|| (!isCodeSetC && txt.size() == 1 && ((txt[0] >= 'A' && txt[0] <= 'Z')
														|| (txt[0] >= 'a' && txt[0] <= 'z')))) {
			// ISO/IEC 15417:2007 Annex B.2
			// FNC1 in second position following Code Set C "00-99" or Code Set A/B "A-Za-z" - AIM
			_symbologyIdentifier = "]C2";
			Diagnostics::fmt("FNC1(AIM %s)", txt.c_str());
		}
		else {
			// ISO/IEC 15417:2007 Annex B.3. Otherwise FNC1 is returned as ASCII 29 (GS)
			txt.push_back((char)29);
			Diagnostics::put("FNC1(29)");
		}
	};

public:
	Raw2TxtDecoder(int startCode) : codeSet(204 - startCode)
	{
		txt.reserve(20);
	}

	bool decode(int code)
	{
		lastTxtSize = txt.size();

		if (codeSet == CODE_CODE_C) {
			if (code < 100) {
				if (code < 10)
					txt.push_back('0');
				txt.append(std::to_string(code));
				Diagnostics::fmt("%02d", code);
			} else if (code == CODE_FNC_1) {
				fnc1(true /*isCodeSetC*/);
			} else {
				codeSet = code; // CODE_A / CODE_B
				Diagnostics::fmt("Code%c", codeSet == CODE_CODE_A ? 'A' : 'B');
			}
		} else { // codeSet A or B
			bool unshift = shift;

			switch (code) {
			case CODE_FNC_1: fnc1(false /*isCodeSetC*/); break;
			case CODE_FNC_2:
				// Message Append - do nothing?
				Diagnostics::put("FNC2");
				break;
			case CODE_FNC_3:
				_readerInit = true; // Can occur anywhere in the symbol (ISO/IEC 15417:2007 4.3.4.2 (c))
				Diagnostics::put("RInit");
				break;
			case CODE_SHIFT:
				if (shift) {
					Diagnostics::put("2ShiftsError");
					return false; // two shifts in a row make no sense
				}
				shift = true;
				codeSet = codeSet == CODE_CODE_A ? CODE_CODE_B : CODE_CODE_A;
				Diagnostics::fmt("Sh%c", codeSet == CODE_CODE_A ? 'A' : 'B');
				break;
			case CODE_CODE_A:
			case CODE_CODE_B:
				if (codeSet == code) {
					// FNC4
					if (fnc4Next)
						fnc4All = !fnc4All;
					fnc4Next = !fnc4Next;
					Diagnostics::put("FNC4");
				} else {
					codeSet = code;
					Diagnostics::fmt("Code%c", codeSet == CODE_CODE_A ? 'A' : 'B');
				}
				break;
			case CODE_CODE_C: codeSet = CODE_CODE_C;
				Diagnostics::put("CodeC");
				break;

			default: {
				// code < 96 at this point
				int offset;
				if (codeSet == CODE_CODE_A && code >= 64)
					offset = fnc4All == fnc4Next ? -64 : +64;
				else
					offset = fnc4All == fnc4Next ? ' ' : ' ' + 128;
				txt.push_back((char)(code + offset));
				fnc4Next = false;
				Diagnostics::chr(txt.back());
				break;
			}
			}

			// Unshift back to another code set if we were shifted
			if (unshift) {
				codeSet = codeSet == CODE_CODE_A ? CODE_CODE_B : CODE_CODE_A;
				shift = false;
			}
		}

		return true;
	}

	std::string text() const
	{
		// Need to pull out the check digit(s) from string (if the checksum code happened to
		// be a printable character).
		return txt.substr(0, lastTxtSize);
	}

	const std::string& symbologyIdentifier() const { return _symbologyIdentifier; }

	bool readerInit() const { return _readerInit; }
};

template <typename C>
static int DetectStartCode(const C& c)
{
	int bestCode = 0;
	float bestVariance = MAX_AVG_VARIANCE;
	for (int code : {CODE_START_A, CODE_START_B, CODE_START_C}) {
		float variance = RowReader::PatternMatchVariance(c, Code128::CODE_PATTERNS[code], MAX_INDIVIDUAL_VARIANCE);
		if (variance < bestVariance) {
			bestVariance = variance;
			bestCode = code;
		}
	}
	return bestVariance < MAX_AVG_VARIANCE ? bestCode : 0;
}

// all 3 start patterns share the same 2-1-1 prefix
constexpr auto START_PATTERN_PREFIX = FixedPattern<3, 4>{2, 1, 1};
constexpr int CHAR_LEN = 6;
constexpr float QUIET_ZONE = 5;	// quiet zone spec is 10 modules, real world examples ignore that, see #138

//#define USE_FAST_1_TO_4_BIT_PATTERN_DECODING
#ifdef USE_FAST_1_TO_4_BIT_PATTERN_DECODING
constexpr int CHAR_SUM = 11;
constexpr int CHARACTER_ENCODINGS[] = {
	0b11011001100, 0b11001101100, 0b11001100110, 0b10010011000, 0b10010001100, // 0
	0b10001001100, 0b10011001000, 0b10011000100, 0b10001100100, 0b11001001000, // 5
	0b11001000100, 0b11000100100, 0b10110011100, 0b10011011100, 0b10011001110, // 10
	0b10111001100, 0b10011101100, 0b10011100110, 0b11001110010, 0b11001011100, // 15
	0b11001001110, 0b11011100100, 0b11001110100, 0b11101101110, 0b11101001100, // 20
	0b11100101100, 0b11100100110, 0b11101100100, 0b11100110100, 0b11100110010, // 25
	0b11011011000, 0b11011000110, 0b11000110110, 0b10100011000, 0b10001011000, // 30
	0b10001000110, 0b10110001000, 0b10001101000, 0b10001100010, 0b11010001000, // 35
	0b11000101000, 0b11000100010, 0b10110111000, 0b10110001110, 0b10001101110, // 40
	0b10111011000, 0b10111000110, 0b10001110110, 0b11101110110, 0b11010001110, // 45
	0b11000101110, 0b11011101000, 0b11011100010, 0b11011101110, 0b11101011000, // 50
	0b11101000110, 0b11100010110, 0b11101101000, 0b11101100010, 0b11100011010, // 55
	0b11101111010, 0b11001000010, 0b11110001010, 0b10100110000, 0b10100001100, // 60
	0b10010110000, 0b10010000110, 0b10000101100, 0b10000100110, 0b10110010000, // 65
	0b10110000100, 0b10011010000, 0b10011000010, 0b10000110100, 0b10000110010, // 70
	0b11000010010, 0b11001010000, 0b11110111010, 0b11000010100, 0b10001111010, // 75
	0b10100111100, 0b10010111100, 0b10010011110, 0b10111100100, 0b10011110100, // 80
	0b10011110010, 0b11110100100, 0b11110010100, 0b11110010010, 0b11011011110, // 85
	0b11011110110, 0b11110110110, 0b10101111000, 0b10100011110, 0b10001011110, // 90
	0b10111101000, 0b10111100010, 0b11110101000, 0b11110100010, 0b10111011110, // 95
	0b10111101110, 0b11101011110, 0b11110101110, 0b11010000100, 0b11010010000, // 100
	0b11010011100, 0b11000111010,                                              // 105
};
#endif

Result Code128Reader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	int minCharCount = 4; // start + payload + checksum + stop
	auto decodePattern = [](const PatternView& view, bool start = false) {
	// TODO: the intention was to always use the way faster OneToFourBitPattern approach but it turned out
	// the old DecodeDigit currently detects more test samples. There could be gained another 20% in
	// in performance. This might work once the subpixel binarizer is in place.
#ifdef USE_FAST_1_TO_4_BIT_PATTERN_DECODING
		return IndexOf(CHARACTER_ENCODINGS, OneToFourBitPattern<CHAR_LEN, CHAR_SUM>(view));
#else
		return start ? DetectStartCode(view)
					 : DecodeDigit(view, Code128::CODE_PATTERNS, MAX_AVG_VARIANCE, MAX_INDIVIDUAL_VARIANCE);
#endif
	};

	next = FindLeftGuard(next, minCharCount * CHAR_LEN, START_PATTERN_PREFIX, QUIET_ZONE);
	if (!next.isValid())
		return Result(DecodeStatus::NotFound);

	next = next.subView(0, CHAR_LEN);
	int startCode = decodePattern(next, true);
	if (!(CODE_START_A <= startCode && startCode <= CODE_START_C))
		return Result(DecodeStatus::NotFound);

	int xStart = next.pixelsInFront();
	ByteArray rawCodes;
	rawCodes.reserve(20);
	rawCodes.push_back(static_cast<uint8_t>(startCode));
	Diagnostics::fmt("    Start%c", startCode == CODE_START_A ? 'A' : startCode == CODE_START_B ? 'B' : 'C');

	Raw2TxtDecoder raw2txt(startCode);

	while (true) {
		if (!next.skipSymbol()) {
			Diagnostics::clear();
			return Result(DecodeStatus::NotFound);
		}

		// Decode another code from image
		int code = decodePattern(next);
		if (code == -1) {
			Diagnostics::clear();
			return Result(DecodeStatus::NotFound);
		}
		if (code == CODE_STOP) {
			Diagnostics::put("Stop");
			break;
		}
		if (code >= CODE_START_A) {
			Diagnostics::fmt("BadCodeError(%d)", code);
			return Result(DecodeStatus::FormatError);
		}
		if (!raw2txt.decode(code)) {
			Diagnostics::fmt("DecodeError(%d)", code);
			return Result(DecodeStatus::FormatError);
		}

		rawCodes.push_back(static_cast<uint8_t>(code));
	}

	if (Size(rawCodes) < minCharCount - 1) { // stop code is missing in rawCodes
		Diagnostics::fmt("NotFound(minCharCount(%d))", minCharCount);
		return Result(DecodeStatus::NotFound);
	}

	// check termination bar (is present and not wider than about 2 modules) and quiet zone (next is now 13 modules
	// wide, require at least 8)
	next = next.subView(0, CHAR_LEN + 1);
	if (!next.isValid() || next[CHAR_LEN] > next.sum(CHAR_LEN) / 4 || !next.hasQuietZoneAfter(QUIET_ZONE/13)) {
		Diagnostics::put("NotFound(QZ)");
		return Result(DecodeStatus::NotFound);
	}

	int checksum = rawCodes.front();
	for (int i = 1; i < Size(rawCodes) - 1; ++i)
		checksum += i * rawCodes[i];
	// the second last code is the checksum (last one is the stop code):
	checksum %= 103;
	Diagnostics::fmt("CSum(%d)", checksum);
	if (checksum != rawCodes.back()) {
		Diagnostics::fmt("CSumError(%d)", rawCodes.back());
		return Result(DecodeStatus::ChecksumError);
	}

	int xStop = next.pixelsTillEnd();
	Result result(raw2txt.text(), rowNumber, xStart, xStop, BarcodeFormat::Code128, std::move(rawCodes),
					raw2txt.symbologyIdentifier(), raw2txt.readerInit());
	return result;
}

} // namespace ZXing::OneD
