/*
* Copyright 2022 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "CBFReader.h"

#include "BinaryBitmap.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "../oned/ODCode128Patterns.h"
#include "../oned/ODCode128Reader.h"

namespace ZXing::CodablockF {

Reader::Reader(const ReaderOptions& options)
	: ZXing::Reader(options)
{
	_formatSpecified = options.hasFormat(BarcodeFormat::CodablockF);
}

static const float MAX_AVG_VARIANCE = 0.25f;
static const float MAX_INDIVIDUAL_VARIANCE = 0.7f;

static const int CODE_CODE_C = 99;
static const int CODE_CODE_B = 100;

static const int CODE_FNC_3 = 96;

static const int CODE_START_A = 103;
static const int CODE_START_B = 104;
static const int CODE_START_C = 105;
static const int CODE_STOP = 106;

constexpr int CHAR_LEN = 6;

template <typename C>
static bool DetectStartCode(const C& c)
{
	if (!c.isValid(CHAR_LEN)) {
		return false;
	}
	float variance = OneD::Code128Reader::PatternMatchVariance(c, OneD::Code128::CODE_PATTERNS[CODE_START_A], MAX_INDIVIDUAL_VARIANCE);
	return variance < MAX_AVG_VARIANCE;
}

template <typename C>
static bool DetectStopCode(const C& c)
{
	if (!c.isValid(CHAR_LEN + 1)) {
		return false;
	}
	float variance = OneD::Code128Reader::PatternMatchVariance(c, FixedPattern<CHAR_LEN + 1, 13>{2, 3, 3, 1, 1, 1, 2}, MAX_INDIVIDUAL_VARIANCE);
	return variance < MAX_AVG_VARIANCE;
}

template <typename C>
static int DecodeDigit(const C& c)
{
	if (!c.isValid(CHAR_LEN)) {
		return -1;
	}
	return OneD::RowReader::DecodeDigit(c, OneD::Code128::CODE_PATTERNS, MAX_AVG_VARIANCE, MAX_INDIVIDUAL_VARIANCE, false);
}

Barcode DetectSymbol(const BinaryBitmap& image)
{
	PointI tl, tr, br, bl;
	std::vector<std::vector <int>> rows;
	bool usePrevReaderInit = false;
	bool readerInit = false;
	AIFlag aiFlag = AIFlag::None;
	int k1 = 0, k2 = 0;

	std::vector<int> rawCodes;
	int xStart = -1, xEnd = -1, lastRowNumber = -1;
	int topBoundarySize = 1, bottomBoundarySize = 1; // TODO: calc properly
	for (int rowNumber = 0; rowNumber < image.height(); rowNumber++) {
		PatternRow bars;
		if (!image.getPatternRow(rowNumber, 0 /*rotate*/, bars)) {
			continue;
		}
		PatternView view(bars);

		PatternView next = view.subView(0, CHAR_LEN);
		if (!DetectStartCode(next)) {
			continue;
		}
		xStart = next.pixelsInFront();
		rawCodes.clear();
		rawCodes.push_back(CODE_START_A);
		for (;;) {
			if (!next.skipSymbol()) {
				return Barcode(DecoderResult(FormatError("Skip fail")), DetectorResult{}, BarcodeFormat::CodablockF);
			}
			int code = DecodeDigit(next);
			if (code == -1) {
				break;
			}
			if (code == CODE_STOP) {
				next = next.subView(0, CHAR_LEN + 1); // Extra double module bar at end
				if (!DetectStopCode(next)) {
					return Barcode(DecoderResult(FormatError("Stop terminator fail")), DetectorResult{}, BarcodeFormat::CodablockF);
				}
				xEnd = next.pixelsTillEnd();
				break;
			}

			rawCodes.push_back(narrow_cast<uint8_t>(code));
		}
		if (Size(rawCodes) < 7) { // subset selector + row indicator + min 4 data chars + check char
			continue;
		}

		int checksum = rawCodes.front();
		for (int i = 1; i < Size(rawCodes) - 1; ++i)
			checksum += i * rawCodes[i];
		checksum %= 103;
		if (checksum != rawCodes.back()) {
			//fprintf(stderr, "error checksum %d != %d (rawCodes.back)\n", checksum, rawCodes.back());
			continue;
		}

		if (rows.empty()) {
			usePrevReaderInit = checksum == CODE_FNC_3;
			// TODO: calc top boundary size
			tl = PointI(xStart, rowNumber - topBoundarySize);
			tr = PointI(xEnd, rowNumber - topBoundarySize);
			rows.push_back(rawCodes);

		} else if (rows.back() != rawCodes) {
			if (Size(rows.front()) != Size(rawCodes)) {
				return Barcode(DecoderResult(FormatError("Bad row size")), DetectorResult{}, BarcodeFormat::CodablockF);
			}
			rows.push_back(rawCodes);
		}
		lastRowNumber = rowNumber;
		k1 = rawCodes[Size(rawCodes) - 3];
		k2 = rawCodes[Size(rawCodes) - 2];
	}
	// TODO: calc bottom boundary size
	bl = PointI(xStart, lastRowNumber + bottomBoundarySize);
	br = PointI(xEnd, lastRowNumber + bottomBoundarySize);

#if 0
	for (int i = 0; i < Size(rows); i++) {
		fprintf(stderr, "row %d:", i);
		for (int j = 0; j < Size(rows[i]); j++) fprintf(stderr, " %d,", rows[i][j]);
		fprintf(stderr, "\n");
	}
#endif

	if (Size(rows) < 2) {
		return Barcode(DecoderResult(FormatError("< 2 rows")), DetectorResult{}, BarcodeFormat::CodablockF);
	}

	// Check row indicators, first row is total no. of rows
	if (rows[0][1] == CODE_CODE_C) {
		if (rows[0][2] + 2 != Size(rows)) {
			return Barcode(DecoderResult(FormatError("Bad row indicator total")), DetectorResult{}, BarcodeFormat::CodablockF);
		}
	} else {
		if ((rows[0][2] >= 64 && rows[0][2] - 64 + 2 != Size(rows)) || (rows[0][2] < 64 && rows[0][2] + 34 != Size(rows))) {
			return Barcode(DecoderResult(FormatError("Bad row indicator total")), DetectorResult{}, BarcodeFormat::CodablockF);
		}
	}
	for (int i = 1; i < Size(rows); i++) {
		if (rows[i][1] == CODE_CODE_C) {
			if (rows[i][2] - 42 != i) {
				return Barcode(DecoderResult(FormatError("Bad row indicator index")), DetectorResult{}, BarcodeFormat::CodablockF);
			}
		} else {
			if (rows[i][2] >= 26) {
				if (rows[i][2] - 26 + 6 != i) {
					return Barcode(DecoderResult(FormatError("Bad row indicator index")), DetectorResult{}, BarcodeFormat::CodablockF);
				}
			} else if (rows[i][2] - 10 != i) {
				return Barcode(DecoderResult(FormatError("Bad row indicator index")), DetectorResult{}, BarcodeFormat::CodablockF);
			}
		}
	}

	Diagnostics::fmt("  Dimensions: %dx%d (RowsxColumns)", Size(rows), Size(rows.front()));

	bool lastCodeSetC;
	std::string text;
	for (int i = 0; i < Size(rows); i++) {
		const std::vector<int>& row = rows[i];
		const int startCode = row[1] == CODE_CODE_B ? CODE_START_B : row[1] == CODE_CODE_C ? CODE_START_C : CODE_START_A;
		Diagnostics::fmt("\n  Row(%d) CodeStart%c", i, 'A' + (startCode - CODE_START_A));
		OneD::Code128Decoder rowText(startCode);
		for (int j = 3; j < Size(row) - (i + 1 == Size(rows) ? 2 : 0); j++) {
			if (!rowText.decode(row[j])) {
				return Barcode(DecoderResult(FormatError("Decode")), DetectorResult{}, BarcodeFormat::CodablockF);
			}
		}
		if (i == 0) {
			aiFlag = rowText.symbologyIdentifier().aiFlag;
			readerInit = usePrevReaderInit ? rowText.prevReaderInit() : rowText.readerInit();
		}
		text += rowText.text();
		lastCodeSetC = rowText.lastCodeSetC();
	}

	if (!lastCodeSetC) {
		k1 += k1 >= 64 ? -64 : k1 >= 26 ? 22 : 32;
		k2 += k2 >= 64 ? -64 : k2 >= 26 ? 22 : 32;
	}
	Diagnostics::fmt("K1:%d K2:%d", k1, k2);

	int check_k1 = 0, check_k2 = 0;
	for (int i = 0; i < Size(text); i++) {
		unsigned char ch = text[i];
		check_k1 = (check_k1 + (i + 1) * ch) % 86;
		check_k2 = (check_k2 + i * ch) % 86;
	}
	if (check_k1 != k1) {
		Diagnostics::fmt("\n  Warning: K1 %d != calculated %d", k1, check_k1);
	}
	if (check_k2 != k2) {
		Diagnostics::fmt("\n  Warning: K2 %d != calculated %d", k2, check_k2);
	}

	SymbologyIdentifier si{ 'O', aiFlag == AIFlag::GS1 ? '5' : '4', 0, aiFlag};

	DecoderResult decoderResult(Content(ByteArray(text), si, CharacterSet::ISO8859_1));
	decoderResult.setReaderInit(readerInit);

	return Barcode(std::move(decoderResult), DetectorResult({}, Position(tl, tr, br, bl)), BarcodeFormat::CodablockF);
}

static Barcode DecodePure(const BinaryBitmap& image)
{
	Barcode res = DetectSymbol(image);

	if (!res.isValid()) {
		fprintf(stderr, "ERROR: %s\n", ToString(res.error()).c_str());
		return {};
	}

	return res;
}

Barcode
Reader::decode(const BinaryBitmap& image) const
{
	if (!_formatSpecified) {
		(void)image;
		return {};
	}
	return DecodePure(image);
}

} // namespace ZXing::CodablockF
