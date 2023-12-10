/*
* Copyright 2022 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "C16KReader.h"

#include "BinaryBitmap.h"
#include "DecoderResult.h"
#include "../oned/ODCode128Patterns.h"
#include "../oned/ODCode128Reader.h"

namespace ZXing::Code16K {

Reader::Reader(const ReaderOptions& options)
	: ZXing::Reader(options)
{
	_formatSpecified = options.hasFormat(BarcodeFormat::Code16K);
}

constexpr float MAX_AVG_VARIANCE = 0.25f;
constexpr float MAX_INDIVIDUAL_VARIANCE = 0.7f;

constexpr int CODE_AB_SHIFT1 = 98;
constexpr int CODE_AB_SHIFT2 = 104;
constexpr int CODE_AB_SHIFT2C = 105;
constexpr int CODE_AB_SHIFT3C = 106;

constexpr int CODE_C_SHIFT1B = 104;
constexpr int CODE_C_SHIFT2B = 105;

constexpr int CODE_CODE_C = 99;
constexpr int CODE_CODE_B = 100;
constexpr int CODE_CODE_A = 101;

constexpr int CODE_FNC_1 = 102;
constexpr int CODE_FNC_2 = 97;
constexpr int CODE_FNC_3 = 96;

constexpr int CODE_PAD = 103;

class C16KDecoder {
public:
	int codeSet = 0;
	bool _readerInit = false;
	std::string txt;

	bool fnc4All = false;
	bool fnc4Next = false;
	int shift = 0;
	int shift_from = 0;

public:
	C16KDecoder(int _codeSet, int impliedShiftB) : codeSet(_codeSet)
	{
		if (impliedShiftB) {
			shift = impliedShiftB - CODE_C_SHIFT1B + 1;
			shift_from = codeSet;
			codeSet = CODE_CODE_B;
		}
	}

	bool decode(int code);
	bool readerInit() const { return _readerInit; }
	const std::string& text() const { return txt; }
};

bool C16KDecoder::decode(int code)
{
	// Unshift back to another code set if we were shifted
	//printf("code %d, codeSet %d, shift_from %d, shift %d\n", code, codeSet, shift_from, shift);
	if (shift_from) {
		if (shift-- == 0) {
			//printf("Shift change\n");
			codeSet = shift_from;
			shift_from = 0;
		}
	}

	if (codeSet == CODE_CODE_C) {
		if (code < 100) {
			txt.append(ToString(code, 2));
			Diagnostics::fmt("%02d", code);
		} else if (code == CODE_CODE_A || code == CODE_CODE_B) {
			codeSet = code; // CODE_A / CODE_B
			Diagnostics::fmt("Code%c", codeSet == CODE_CODE_A ? 'A' : 'B');
		} else if (code == CODE_FNC_1) {
			txt.push_back((char)29);
			Diagnostics::put("FNC1(29)");
		} else if (code == CODE_PAD) {
			Diagnostics::put("PAD");
		} else if (code >= CODE_C_SHIFT1B) {
			if (shift_from) {
				Diagnostics::put("ShiftInShift");
				return false; // Shift within shift makes no sense
			}
			shift = code - CODE_C_SHIFT1B + 1;
			shift_from = CODE_CODE_C;
			codeSet = CODE_CODE_B;
			Diagnostics::fmt("Sh%dB", shift);
		}
	} else { // codeSet A or B
		switch (code) {
		case CODE_FNC_1:
			txt.push_back((char)29);
			Diagnostics::put("FNC1(29)");
			break;
		case CODE_FNC_2:
			// Message Append - do nothing?
			Diagnostics::put("FNC2");
			break;
		case CODE_FNC_3:
			_readerInit = true; // Can occur anywhere in the symbol (EN 12323:2005 4.3.4.4 (c))
			Diagnostics::put("RInit");
			break;
		case CODE_CODE_A:
		case CODE_CODE_B:
			if (codeSet == code) {
				// FNC4
				if (fnc4Next) // Double FNC4 latch not mentioned in EN 12323:2005, but part of Code 128
					fnc4All = !fnc4All;
				fnc4Next = !fnc4Next;
				Diagnostics::put("FNC4");
			} else {
				codeSet = code;
				Diagnostics::fmt("Code%c", codeSet == CODE_CODE_A ? 'A' : 'B');
			}
			break;
		case CODE_CODE_C:
			codeSet = CODE_CODE_C;
			Diagnostics::put("CodeC");
			break;
		case CODE_PAD:
			Diagnostics::put("PAD");
			break;
		case CODE_AB_SHIFT1:
		case CODE_AB_SHIFT2:
			if (shift_from) {
				Diagnostics::put("ShiftInShift");
				return false; // Shift within shift makes no sense
			}
			shift = code == CODE_AB_SHIFT1 ? 1 : 2;
			shift_from = codeSet;
			codeSet = codeSet == CODE_CODE_A ? CODE_CODE_B : CODE_CODE_A;
			Diagnostics::fmt("Sh%d%c", shift, codeSet == CODE_CODE_A ? 'A' : 'B');
			break;
		case CODE_AB_SHIFT2C:
		case CODE_AB_SHIFT3C:
			if (shift_from) {
				Diagnostics::put("ShiftInShift");
				return false; // Shift within shift makes no sense
			}
			shift = code - CODE_AB_SHIFT2C + 2;
			shift_from = codeSet;
			codeSet = CODE_CODE_C;
			Diagnostics::fmt("Sh%dC", shift);
			break;

		default:
			{
				// code < 96 at this point
				int offset;
				if (codeSet == CODE_CODE_A && code >= 64)
					offset = fnc4All == fnc4Next ? -64 : +64;
				else
					offset = fnc4All == fnc4Next ? ' ' : ' ' + 128;
				txt.push_back((char)(code + offset));
				fnc4Next = false;
				Diagnostics::chr(txt.back());
			}
			break;
		}
	}

	return true;
}

constexpr int CHAR_LEN = 6;

constexpr int START_STOP_CHAR_LEN = 4;

const std::array<std::array<int, START_STOP_CHAR_LEN>, 8> START_STOP_CODE_PATTERNS = { {
	/* EN 12323 Table 3 and Table 4 - Start patterns and stop patterns */
	{ 3, 2, 1, 1 }, { 2, 2, 2, 1 }, { 2, 1, 2, 2 }, { 1, 4, 1, 1 },
	{ 1, 1, 3, 2 }, { 1, 2, 3, 1 }, { 1, 1, 1, 4 }, { 3, 1, 1, 2 }
} };

/* EN 12323 Table 5 - Start and stop values defining row numbers */
static const int StartValues[16] = {
	0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7
};

static const int StopValues[16] = {
	0, 1, 2, 3, 4, 5, 6, 7, 4, 5, 6, 7, 0, 1, 2, 3
};

const std::array<std::array<int, CHAR_LEN>, 107> CODE_PATTERNS = { {
	{ 2, 1, 2, 2, 2, 2 }, // 0
	{ 2, 2, 2, 1, 2, 2 },
	{ 2, 2, 2, 2, 2, 1 },
	{ 1, 2, 1, 2, 2, 3 },
	{ 1, 2, 1, 3, 2, 2 },
	{ 1, 3, 1, 2, 2, 2 }, // 5
	{ 1, 2, 2, 2, 1, 3 },
	{ 1, 2, 2, 3, 1, 2 },
	{ 1, 3, 2, 2, 1, 2 },
	{ 2, 2, 1, 2, 1, 3 },
	{ 2, 2, 1, 3, 1, 2 }, // 10
	{ 2, 3, 1, 2, 1, 2 },
	{ 1, 1, 2, 2, 3, 2 },
	{ 1, 2, 2, 1, 3, 2 },
	{ 1, 2, 2, 2, 3, 1 },
	{ 1, 1, 3, 2, 2, 2 }, // 15
	{ 1, 2, 3, 1, 2, 2 },
	{ 1, 2, 3, 2, 2, 1 },
	{ 2, 2, 3, 2, 1, 1 },
	{ 2, 2, 1, 1, 3, 2 },
	{ 2, 2, 1, 2, 3, 1 }, // 20
	{ 2, 1, 3, 2, 1, 2 },
	{ 2, 2, 3, 1, 1, 2 },
	{ 3, 1, 2, 1, 3, 1 },
	{ 3, 1, 1, 2, 2, 2 },
	{ 3, 2, 1, 1, 2, 2 }, // 25
	{ 3, 2, 1, 2, 2, 1 },
	{ 3, 1, 2, 2, 1, 2 },
	{ 3, 2, 2, 1, 1, 2 },
	{ 3, 2, 2, 2, 1, 1 },
	{ 2, 1, 2, 1, 2, 3 }, // 30
	{ 2, 1, 2, 3, 2, 1 },
	{ 2, 3, 2, 1, 2, 1 },
	{ 1, 1, 1, 3, 2, 3 },
	{ 1, 3, 1, 1, 2, 3 },
	{ 1, 3, 1, 3, 2, 1 }, // 35
	{ 1, 1, 2, 3, 1, 3 },
	{ 1, 3, 2, 1, 1, 3 },
	{ 1, 3, 2, 3, 1, 1 },
	{ 2, 1, 1, 3, 1, 3 },
	{ 2, 3, 1, 1, 1, 3 }, // 40
	{ 2, 3, 1, 3, 1, 1 },
	{ 1, 1, 2, 1, 3, 3 },
	{ 1, 1, 2, 3, 3, 1 },
	{ 1, 3, 2, 1, 3, 1 },
	{ 1, 1, 3, 1, 2, 3 }, // 45
	{ 1, 1, 3, 3, 2, 1 },
	{ 1, 3, 3, 1, 2, 1 },
	{ 3, 1, 3, 1, 2, 1 },
	{ 2, 1, 1, 3, 3, 1 },
	{ 2, 3, 1, 1, 3, 1 }, // 50
	{ 2, 1, 3, 1, 1, 3 },
	{ 2, 1, 3, 3, 1, 1 },
	{ 2, 1, 3, 1, 3, 1 },
	{ 3, 1, 1, 1, 2, 3 },
	{ 3, 1, 1, 3, 2, 1 }, // 55
	{ 3, 3, 1, 1, 2, 1 },
	{ 3, 1, 2, 1, 1, 3 },
	{ 3, 1, 2, 3, 1, 1 },
	{ 3, 3, 2, 1, 1, 1 },
	{ 3, 1, 4, 1, 1, 1 }, // 60
	{ 2, 2, 1, 4, 1, 1 },
	{ 4, 3, 1, 1, 1, 1 },
	{ 1, 1, 1, 2, 2, 4 },
	{ 1, 1, 1, 4, 2, 2 },
	{ 1, 2, 1, 1, 2, 4 }, // 65
	{ 1, 2, 1, 4, 2, 1 },
	{ 1, 4, 1, 1, 2, 2 },
	{ 1, 4, 1, 2, 2, 1 },
	{ 1, 1, 2, 2, 1, 4 },
	{ 1, 1, 2, 4, 1, 2 }, // 70
	{ 1, 2, 2, 1, 1, 4 },
	{ 1, 2, 2, 4, 1, 1 },
	{ 1, 4, 2, 1, 1, 2 },
	{ 1, 4, 2, 2, 1, 1 },
	{ 2, 4, 1, 2, 1, 1 }, // 75
	{ 2, 2, 1, 1, 1, 4 },
	{ 4, 1, 3, 1, 1, 1 },
	{ 2, 4, 1, 1, 1, 2 },
	{ 1, 3, 4, 1, 1, 1 },
	{ 1, 1, 1, 2, 4, 2 }, // 80
	{ 1, 2, 1, 1, 4, 2 },
	{ 1, 2, 1, 2, 4, 1 },
	{ 1, 1, 4, 2, 1, 2 },
	{ 1, 2, 4, 1, 1, 2 },
	{ 1, 2, 4, 2, 1, 1 }, // 85
	{ 4, 1, 1, 2, 1, 2 },
	{ 4, 2, 1, 1, 1, 2 },
	{ 4, 2, 1, 2, 1, 1 },
	{ 2, 1, 2, 1, 4, 1 },
	{ 2, 1, 4, 1, 2, 1 }, // 90
	{ 4, 1, 2, 1, 2, 1 },
	{ 1, 1, 1, 1, 4, 3 },
	{ 1, 1, 1, 3, 4, 1 },
	{ 1, 3, 1, 1, 4, 1 },
	{ 1, 1, 4, 1, 1, 3 }, // 95
	{ 1, 1, 4, 3, 1, 1 },
	{ 4, 1, 1, 1, 1, 3 },
	{ 4, 1, 1, 3, 1, 1 },
	{ 1, 1, 3, 1, 4, 1 },
	{ 1, 1, 4, 1, 3, 1 }, // 100
	{ 3, 1, 1, 1, 4, 1 },
	{ 4, 1, 1, 1, 3, 1 },
	{ 2, 1, 1, 4, 1, 2 },
	{ 2, 1, 1, 2, 1, 4 },
	{ 2, 1, 1, 2, 3, 2 }, // 105
	{ 2, 1, 1, 1, 3, 3 }
} };

template <typename C>
static bool DetectRowStartCode(const C& c, int row)
{
	float variance = OneD::Code128Reader::PatternMatchVariance(c, START_STOP_CODE_PATTERNS[StartValues[row]], MAX_INDIVIDUAL_VARIANCE);
	return variance < MAX_AVG_VARIANCE;
}

template <typename C>
static bool DetectRowStopCode(const C& c, int row)
{
	float variance = OneD::Code128Reader::PatternMatchVariance(c, START_STOP_CODE_PATTERNS[StopValues[row]], MAX_INDIVIDUAL_VARIANCE);
	return variance < MAX_AVG_VARIANCE;
}

template <typename C>
static int DecodeDigit(const C& c)
{
	return OneD::RowReader::DecodeDigit(c, CODE_PATTERNS, MAX_AVG_VARIANCE, MAX_INDIVIDUAL_VARIANCE, false);
}

Result DetectSymbol(const BinaryBitmap& image)
{
	PointI tl, tr, br, bl;
	std::vector<std::vector <int>> rows;

	std::vector<int> rawCodes;
	int xStart = -1, xEnd = -1, lastRowNumber = -1;
	for (int rowNumber = 0; rowNumber < image.height(); rowNumber++) {
		//printf("rowNumber %d\n", rowNumber);
		PatternRow bars;
		if (!image.getPatternRow(rowNumber, 0 /*rotate*/, bars)) {
			continue;
		}
		PatternView view(bars);
		//printf("initial view:"); for (int i = 0; i < view.size(); i++) { printf(" %d", view[i]); } printf("\n");

		PatternView next = view.subView(0, START_STOP_CHAR_LEN);
		if (!DetectRowStartCode(next, Size(rows))) {
			//printf("!DetectRowStartCode %d\n", rowNumber);
			continue;
		}
		xStart = next.pixelsInFront();
		rawCodes.clear();
		for (;;) {
			if (!next.skipSymbol()) {
				if (next.size() != CHAR_LEN) {
					return Result(DecoderResult(FormatError("Skip fail")), {}, BarcodeFormat::Code16K);
				}
				next = next.subView(0, START_STOP_CHAR_LEN);
				if (!DetectRowStopCode(next, Size(rows))) {
					if (Size(rawCodes) == 5) {
						return Result(DecoderResult(FormatError("DetectRowStopCode fail")), {}, BarcodeFormat::Code16K);
					}
					rawCodes.clear();
				} else {
					xEnd = next.pixelsTillEnd();
				}
				break;
			}
			if (next.size() == START_STOP_CHAR_LEN) {
				next.shift(1); // Skip 1X guard
				next = next.subView(0, CHAR_LEN);
			}
			int code = DecodeDigit(next);
			if (code == -1) {
				printf("code -1\n");
				next = next.subView(0, START_STOP_CHAR_LEN);
				if (!DetectRowStopCode(next, Size(rows))) {
					if (Size(rawCodes) == 5) {
						return Result(DecoderResult(FormatError("DetectRowStopCode fail")), {}, BarcodeFormat::Code16K);
					}
					rawCodes.clear();
				} else {
					xEnd = next.pixelsTillEnd();
				}
				break;
			}

			rawCodes.push_back(narrow_cast<uint8_t>(code));
		}
		if (Size(rawCodes) != 5) {
			continue;
		}

		if (rows.empty()) {
			tl = PointI(xStart, rowNumber);
			tr = PointI(xEnd, rowNumber);
			rows.push_back(rawCodes);
		} else {
			rows.push_back(rawCodes);
		}
		lastRowNumber = rowNumber;
	}
	br = PointI(xEnd, lastRowNumber);
	bl = PointI(xStart, lastRowNumber);

#if 0
	for (int i = 0; i < Size(rows); i++) {
		printf("row %d:", i);
		for (int j = 0; j < Size(rows[i]); j++) printf(" %d,", rows[i][j]);
		printf("\n");
	}
#endif

	if (Size(rows) < 2) {
		return Result(DecoderResult(FormatError("< 2 rows")), {}, BarcodeFormat::Code16K);
	}

	Diagnostics::fmt("  Dimensions: %dx%d (RowsxColumns)", Size(rows), Size(rows.front()));

	int mode = rows[0][0] % 7;
	int numberRows = (rows[0][0] - mode) / 7 + 2;
	//printf("mode %d, numberRows %d\n", mode, numberRows);
	if (numberRows != Size(rows)) {
		return Result(DecoderResult(FormatError("number of rows mismatch")), {}, BarcodeFormat::Code16K);
	}

	int codeSet = CODE_CODE_A;
	int impliedShiftB = 0;
	AIFlag aiFlag = AIFlag::None;

	if (mode == 1 || mode == 3) {
		codeSet = CODE_CODE_B;
		if (mode == 3) {
			aiFlag = AIFlag::GS1;
		}
	} else if (mode == 2 || mode >= 4) {
		codeSet = CODE_CODE_C;
		if (mode == 4) {
			aiFlag = AIFlag::GS1;
		} else if (mode == 5) {
			impliedShiftB = CODE_C_SHIFT1B;
		} else if (mode == 6) {
			impliedShiftB = CODE_C_SHIFT2B;
		}
	}
	Diagnostics::fmt("Mode(%d,%d,%d,%d)", mode, codeSet, impliedShiftB, (int)aiFlag);

	C16KDecoder decoder(codeSet, impliedShiftB);
	//printf("codeSet %d, shift %d, shift_from %d\n", decoder.codeSet, decoder.shift, decoder.shift_from);

	std::string text;
	int rowStart = 1;
	bool haveD1Pad = false;
	if (rows[0][1] == CODE_FNC_1) { // D1
		if (aiFlag != AIFlag::None) { // Was implied already
			// TODO: warning
		}
		aiFlag = AIFlag::GS1;
		rowStart = 2;
	} else if (rows[0][2] == CODE_FNC_1) { // D2 AIM
		if (aiFlag != AIFlag::None) { // Was implied already
			// TODO: error
		} else {
			aiFlag = AIFlag::AIM;
			rowStart = 3;
		}
	} else if (rows[0][2] == CODE_FNC_2) { // D2 message append
		// TODO: D1 gives index, number of symbols
		rowStart = 3;
	} else if (rows[0][1] == CODE_PAD) { // Mysterious meaning
		haveD1Pad = true;
		rowStart = 2;
	}

	for (int i = 0; i < Size(rows); i++) {
		const std::vector<int>& row = rows[i];
		for (int j = rowStart; j < Size(row) - (i + 1 == Size(rows) ? 2 : 0); j++) {
			if (!decoder.decode(row[j])) {
				return Result(DecoderResult(FormatError("Decode")), {}, BarcodeFormat::Code16K);
			}
		}
		rowStart = 0;
	}

	// EN 12323:2005 Annex E Table E.1
	SymbologyIdentifier si = {'K', '0', 0, aiFlag};
	if (aiFlag == AIFlag::GS1) {
		si.modifier = '1';
	} else if (aiFlag == AIFlag::AIM) {
		si.modifier = '2';
	} else if (haveD1Pad) {
		si.modifier = '4';
	}

	DecoderResult decoderResult(Content(ByteArray(decoder.text()), si, CharacterSet::ISO8859_1));
	decoderResult.setReaderInit(decoder.readerInit());

	return Result(std::move(decoderResult), Position(tl, tr, br, bl), BarcodeFormat::Code16K);
}

static Result DecodePure(const BinaryBitmap& image)
{
	Result res = DetectSymbol(image);

	if (!res.isValid()) {
		printf("ERROR: %s\n", ToString(res.error()).c_str());
		return {};
	}

	return res;
}

Result
Reader::decode(const BinaryBitmap& image) const
{
	if (!_formatSpecified) {
		(void)image;
		return {};
	}
	return DecodePure(image);
}

} // namespace ZXing::Code16K
