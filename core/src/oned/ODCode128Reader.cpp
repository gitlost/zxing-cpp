/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODCode128Reader.h"

#include "Diagnostics.h"
#include "ODCode128Patterns.h"
#include "Barcode.h"
#include "ZXAlgorithms.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
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
static const int CODE_START_C = 105;
static const int CODE_STOP = 106;

Code128Decoder::Code128Decoder(int startCode) : codeSet(204 - startCode)
{
	txt.reserve(20);
}

void Code128Decoder::fnc1(const bool isCodeSetC)
{
	if (txt.empty()) {
		// ISO/IEC 15417:2007 Annex B.1 and GS1 General Specifications 21.0.1 Section 5.4.3.7
		// If the first char after the start code is FNC1 then this is GS1-128.
		_symbologyIdentifier.modifier = '1';
		// GS1 General Specifications Section 5.4.6.4
		// "Transmitted data ... is prefixed by the symbology identifier ]C1, if used."
		// Choosing not to use symbology identifier, i.e. to not prefix to data.
		Diagnostics::put("FNC1(GS1)");
		_symbologyIdentifier.aiFlag = AIFlag::GS1;
	}
	else if ((isCodeSetC && txt.size() == 2 && txt[0] >= '0' && txt[0] <= '9' && txt[1] >= '0' && txt[1] <= '9')
			|| (!isCodeSetC && txt.size() == 1 && ((txt[0] >= 'A' && txt[0] <= 'Z')
													|| (txt[0] >= 'a' && txt[0] <= 'z')))) {
		// ISO/IEC 15417:2007 Annex B.2
		// FNC1 in second position following Code Set C "00-99" or Code Set A/B "A-Za-z" - AIM
		_symbologyIdentifier.modifier = '2';
		Diagnostics::fmt("FNC1(AIM %s)", txt.c_str());
		_symbologyIdentifier.aiFlag = AIFlag::AIM;
	}
	else {
		// ISO/IEC 15417:2007 Annex B.3. Otherwise FNC1 is returned as ASCII 29 (GS)
		txt.push_back((char)29);
		Diagnostics::put("FNC1(29)");
	}
}

bool Code128Decoder::decode(int code)
{
	lastTxtSize = txt.size();

	if (codeSet == CODE_CODE_C) {
		if (code < 100) {
			txt.append(ToString(code, 2));
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
			_prevReaderInit = _readerInit; // Could be processing checksum so need to record previous state
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

std::string Code128Decoder::text() const
{
	// Need to pull out the check digit(s) from string (if the checksum code happened to
	// be a printable character).
	return txt.substr(0, lastTxtSize);
}

// all 3 start patterns share the same 2-1-1 prefix
constexpr auto START_PATTERN_PREFIX = FixedPattern<3, 4>{2, 1, 1};
constexpr int CHAR_LEN = 6;
constexpr float QUIET_ZONE = 5;	// quiet zone spec is 10 modules, real world examples ignore that, see #138
constexpr int CHAR_SUM = 11;

//TODO: make this a constexpr variable initialization
static auto E2E_PATTERNS = [] {
	// This creates an array of ints for fast IndexOf lookup of the edge-2-edge patterns (ISO/IEC 15417:2007(E) Table 2)
	// e.g. a code pattern of { 2, 1, 2, 2, 2, 2 } becomes the e2e pattern { 3, 3, 4, 4 } and the value 0b11100011110000.
	std::array<int, 107> res;
	for (int i = 0; i < Size(res); ++i) {
		const auto& a = Code128::CODE_PATTERNS[i];
		std::array<int, 4> e2e;
		for (int j = 0; j < 4; j++)
			e2e[j] = a[j] + a[j + 1];
		res[i] = ToInt(e2e);
	}
	return res;
}();

Barcode Code128Reader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	int minCharCount = 4; // start + payload + checksum + stop
	auto decodePattern = [](const PatternView& view, bool start = false) {
		// This is basically the reference algorithm from the specification
		int code = IndexOf(E2E_PATTERNS, ToInt(NormalizedE2EPattern<CHAR_LEN, CHAR_SUM>(view)));
		if (code == -1 && !start) // if the reference algo fails, give the original upstream version a try (required to decode a few samples)
			code = DecodeDigit(view, Code128::CODE_PATTERNS, MAX_AVG_VARIANCE, MAX_INDIVIDUAL_VARIANCE);
		return code;
	};

	next = FindLeftGuard(next, minCharCount * CHAR_LEN, START_PATTERN_PREFIX, QUIET_ZONE);
	if (!next.isValid()) {
		return {};
	}

	next = next.subView(0, CHAR_LEN);
	int startCode = decodePattern(next, true);
	if (!(CODE_START_A <= startCode && startCode <= CODE_START_C))
		return {};
	//printf("%s: startCode %d found\n", __func__, startCode);

	int xStart = next.pixelsInFront();
	ByteArray rawCodes;
	rawCodes.reserve(20);
	rawCodes.push_back(narrow_cast<uint8_t>(startCode));
	Diagnostics::fmt("  Decode: Start%c", startCode == CODE_START_A ? 'A' : startCode == CODE_START_C ? 'C' : 'B');

	Code128Decoder raw2txt(startCode);

	while (true) {
		if (!next.skipSymbol()) {
			Diagnostics::clear();
			//printf("next.skip fail\n");
			return {};
		}

		// Decode another code from image
		int code = decodePattern(next);
		if (code == -1) {
			Diagnostics::clear();
			#if 0
			if (Size(rawCodes)) {
				printf("decodePattern(next) fail Size(rawCodes) %d: ", Size(rawCodes));
				for (int i = 0; i < Size(rawCodes); i++) printf(" %d", rawCodes[i]);
				printf("\n");
			}
			#endif
			return {};
		}
		if (code == CODE_STOP) {
			Diagnostics::put("Stop");
			break;
		}
		if (code >= CODE_START_A) {
			Diagnostics::fmt("BadCodeError(%d)", code);
			return {};
		}
		if (!raw2txt.decode(code)) {
			Diagnostics::fmt("DecodeError(%d)", code);
			return {};
		}

		rawCodes.push_back(narrow_cast<uint8_t>(code));
	}

	if (Size(rawCodes) < minCharCount - 1) { // stop code is missing in rawCodes
		Diagnostics::fmt("NotFound(minCharCount(%d))", minCharCount);
		return {};
	}

	// check termination bar (is present and not wider than about 2 modules) and quiet zone (next is now 13 modules
	// wide, require at least 8)
	next = next.subView(0, CHAR_LEN + 1);
	if (!next.isValid() || next[CHAR_LEN] > next.sum(CHAR_LEN) / 4 || !next.hasQuietZoneAfter(QUIET_ZONE/13)) {
		Diagnostics::put("NotFound(QZ)");
		return {};
	}

	Diagnostics::fmt("\n  Codewords(%d):", rawCodes.size()); Diagnostics::dump(rawCodes, "\n");

	Error error;
	int checksum = rawCodes.front();
	for (int i = 1; i < Size(rawCodes) - 1; ++i)
		checksum += i * rawCodes[i];
	// the second last code is the checksum (last one is the stop code):
	checksum %= 103;
	Diagnostics::fmt("  CSum(%d)", checksum);
	if (checksum != rawCodes.back()) {
		Diagnostics::fmt("CSumError(%d)", rawCodes.back());
		error = ChecksumError();
	}
	bool readerInit = checksum == CODE_FNC_3 ? raw2txt.prevReaderInit() : raw2txt.readerInit(); // Allow for bogus CODE_FNC_3

	int xStop = next.pixelsTillEnd();
	return Barcode(raw2txt.text(), rowNumber, xStart, xStop, BarcodeFormat::Code128, raw2txt.symbologyIdentifier(), error,
				  readerInit);
}

} // namespace ZXing::OneD
