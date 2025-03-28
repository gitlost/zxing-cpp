/*
* Copyright 2022 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "MicroPDFReader.h"

#include "BinaryBitmap.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Pattern.h"
#include "PDFCodewordDecoder.h"
#include "PDFScanningDecoder.h"
#include "../oned/ODRowReader.h"

namespace ZXing::MicroPdf417 {

Reader::Reader(const ReaderOptions& options)
	: ZXing::Reader(options)
{
	_formatSpecified = options.hasFormat(BarcodeFormat::MicroPDF417);
}

static const FixedPattern<6, 10> LRRAPs[] = {
	{ 2, 2, 1, 3, 1, 1 }, /* 1*/
	{ 3, 1, 1, 3, 1, 1 }, /* 2*/
	{ 3, 1, 2, 2, 1, 1 }, /* 3*/
	{ 2, 2, 2, 2, 1, 1 }, /* 4*/
	{ 2, 1, 3, 2, 1, 1 }, /* 5*/
	{ 2, 1, 4, 1, 1, 1 }, /* 6*/
	{ 2, 2, 3, 1, 1, 1 }, /* 7*/
	{ 3, 1, 3, 1, 1, 1 }, /* 8*/
	{ 3, 2, 2, 1, 1, 1 }, /* 9*/
	{ 4, 1, 2, 1, 1, 1 }, /*10*/
	{ 4, 2, 1, 1, 1, 1 }, /*11*/
	{ 3, 3, 1, 1, 1, 1 }, /*12*/
	{ 2, 4, 1, 1, 1, 1 }, /*13*/
	{ 2, 3, 2, 1, 1, 1 }, /*14*/
	{ 2, 3, 1, 2, 1, 1 }, /*15*/
	{ 3, 2, 1, 2, 1, 1 }, /*16*/
	{ 4, 1, 1, 2, 1, 1 }, /*17*/
	{ 4, 1, 1, 1, 2, 1 }, /*18*/
	{ 4, 1, 1, 1, 1, 2 }, /*19*/
	{ 3, 2, 1, 1, 1, 2 }, /*20*/
	{ 3, 1, 2, 1, 1, 2 }, /*21*/
	{ 3, 1, 1, 2, 1, 2 }, /*22*/
	{ 3, 1, 1, 2, 2, 1 }, /*23*/
	{ 3, 1, 1, 1, 3, 1 }, /*24*/
	{ 3, 1, 1, 1, 2, 2 }, /*25*/
	{ 3, 1, 1, 1, 1, 3 }, /*26*/
	{ 2, 2, 1, 1, 1, 3 }, /*27*/
	{ 2, 2, 1, 1, 2, 2 }, /*28*/
	{ 2, 2, 1, 1, 3, 1 }, /*29*/
	{ 2, 2, 1, 2, 2, 1 }, /*30*/
	{ 2, 2, 2, 1, 2, 1 }, /*31*/
	{ 3, 1, 2, 1, 2, 1 }, /*32*/
	{ 3, 2, 1, 1, 2, 1 }, /*33*/
	{ 2, 3, 1, 1, 2, 1 }, /*34*/
	{ 2, 3, 1, 1, 1, 2 }, /*35*/
	{ 2, 2, 2, 1, 1, 2 }, /*36*/
	{ 2, 1, 3, 1, 1, 2 }, /*37*/
	{ 2, 1, 2, 2, 1, 2 }, /*38*/
	{ 2, 1, 2, 2, 2, 1 }, /*39*/
	{ 2, 1, 2, 1, 3, 1 }, /*40*/
	{ 2, 1, 2, 1, 2, 2 }, /*41*/
	{ 2, 1, 2, 1, 1, 3 }, /*42*/
	{ 2, 1, 1, 2, 1, 3 }, /*43*/
	{ 2, 1, 1, 1, 2, 3 }, /*44*/
	{ 2, 1, 1, 1, 3, 2 }, /*45*/
	{ 2, 1, 1, 1, 4, 1 }, /*46*/
	{ 2, 1, 1, 2, 3, 1 }, /*47*/
	{ 2, 1, 1, 2, 2, 2 }, /*48*/
	{ 2, 1, 1, 3, 1, 2 }, /*49*/
	{ 2, 1, 1, 3, 2, 1 }, /*50*/
	{ 2, 1, 1, 4, 1, 1 }, /*51*/
	{ 2, 1, 2, 3, 1, 1 }, /*52*/
};

static const FixedPattern<6, 10> CRAPs[] = {
	{ 1, 1, 2, 2, 3, 1 }, /*01*/
	{ 1, 2, 1, 2, 3, 1 }, /*02*/
	{ 1, 2, 2, 1, 3, 1 }, /*03*/
	{ 1, 3, 1, 1, 3, 1 }, /*04*/
	{ 1, 3, 1, 2, 2, 1 }, /*05*/
	{ 1, 3, 2, 1, 2, 1 }, /*06*/
	{ 1, 4, 1, 1, 2, 1 }, /*07*/
	{ 1, 4, 1, 2, 1, 1 }, /*08*/
	{ 1, 4, 2, 1, 1, 1 }, /*09*/
	{ 1, 3, 3, 1, 1, 1 }, /*10*/
	{ 1, 3, 2, 2, 1, 1 }, /*11*/
	{ 1, 3, 1, 3, 1, 1 }, /*12*/
	{ 1, 2, 2, 3, 1, 1 }, /*13*/
	{ 1, 2, 3, 2, 1, 1 }, /*14*/
	{ 1, 2, 4, 1, 1, 1 }, /*15*/
	{ 1, 1, 5, 1, 1, 1 }, /*16*/
	{ 1, 1, 4, 2, 1, 1 }, /*17*/
	{ 1, 1, 4, 1, 2, 1 }, /*18*/
	{ 1, 2, 3, 1, 2, 1 }, /*19*/
	{ 1, 2, 3, 1, 1, 2 }, /*20*/
	{ 1, 2, 2, 2, 1, 2 }, /*21*/
	{ 1, 2, 2, 2, 2, 1 }, /*22*/
	{ 1, 2, 1, 3, 2, 1 }, /*23*/
	{ 1, 2, 1, 4, 1, 1 }, /*24*/
	{ 1, 1, 2, 4, 1, 1 }, /*25*/
	{ 1, 1, 3, 3, 1, 1 }, /*26*/
	{ 1, 1, 3, 2, 2, 1 }, /*27*/
	{ 1, 1, 3, 2, 1, 2 }, /*28*/
	{ 1, 1, 3, 1, 2, 2 }, /*29*/
	{ 1, 2, 2, 1, 2, 2 }, /*30*/
	{ 1, 3, 1, 1, 2, 2 }, /*31*/
	{ 1, 3, 1, 1, 1, 3 }, /*32*/
	{ 1, 2, 2, 1, 1, 3 }, /*33*/
	{ 1, 1, 3, 1, 1, 3 }, /*34*/
	{ 1, 1, 2, 2, 1, 3 }, /*35*/
	{ 1, 1, 2, 2, 2, 2 }, /*36*/
	{ 1, 1, 2, 3, 1, 2 }, /*37*/
	{ 1, 1, 2, 3, 2, 1 }, /*38*/
	{ 1, 1, 1, 4, 2, 1 }, /*39*/
	{ 1, 1, 1, 3, 3, 1 }, /*40*/
	{ 1, 1, 1, 3, 2, 2 }, /*41*/
	{ 1, 1, 1, 2, 3, 2 }, /*42*/
	{ 1, 1, 1, 2, 2, 3 }, /*43*/
	{ 1, 1, 1, 1, 3, 3 }, /*44*/
	{ 1, 1, 1, 1, 2, 4 }, /*45*/
	{ 1, 1, 1, 2, 1, 4 }, /*46*/
	{ 1, 1, 2, 1, 1, 4 }, /*47*/
	{ 1, 2, 1, 1, 1, 4 }, /*48*/
	{ 1, 2, 1, 1, 2, 3 }, /*49*/
	{ 1, 2, 1, 1, 3, 2 }, /*50*/
	{ 1, 1, 2, 1, 3, 2 }, /*51*/
	{ 1, 1, 2, 1, 4, 1 }, /*52*/
};

static const float MAX_AVG_VARIANCE = 0.1f;
static const float MAX_INDIVIDUAL_VARIANCE = 0.3f;

constexpr int RAP_CHAR_LEN = 6;
constexpr int CHAR_LEN = 8;

template <typename C>
static int DetectLRRAPCode(const C& c)
{
	if (!c.isValid(RAP_CHAR_LEN)) {
		return -1;
	}
	int bestCode = 0;
	float bestVariance = MAX_AVG_VARIANCE;
	for (int code = 0; code < Size(LRRAPs); code++) {
		float variance = OneD::RowReader::PatternMatchVariance(c, LRRAPs[code], MAX_INDIVIDUAL_VARIANCE);
		if (variance < bestVariance) {
			bestVariance = variance;
			bestCode = code;
		}
	}
	return bestVariance < MAX_AVG_VARIANCE ? bestCode : -1;
}

template <typename C>
static int DetectCRAPCode(const C& c)
{
	if (!c.isValid(RAP_CHAR_LEN)) {
		return -1;
	}
	int bestCode = 0;
	float bestVariance = MAX_AVG_VARIANCE;
	for (int code = 0; code < Size(CRAPs); code++) {
		float variance = OneD::RowReader::PatternMatchVariance(c, CRAPs[code], MAX_INDIVIDUAL_VARIANCE);
		if (variance < bestVariance) {
			bestVariance = variance;
			bestCode = code;
		}
	}
	//printf("bestVariance %g\n", bestVariance);
	return bestVariance < MAX_AVG_VARIANCE ? bestCode : -1;
}

template <typename C>
static int DecodeCodeword(const C& c, const int cluster)
{
	if (!c.isValid(CHAR_LEN)) {
		return -1;
	}
	const std::array<int, CHAR_LEN>& np = NormalizedPattern<CHAR_LEN, 17>(c);
	int patternCluster = (np[0] - np[2] + np[4] - np[6] + 9) % 9;
	//printf("0x%X: patternCluster %d, cluster %d\n", ToInt(np), patternCluster, cluster);
	if (patternCluster != cluster) {
		return -1;
	}
	return Pdf417::CodewordDecoder::GetCodeword(ToInt(np));
}

Barcode DetectSymbol(const BinaryBitmap& image)
{
	PointI tl, tr, br, bl;
	int nRows = 0, nCols = -1;

	std::vector<int> lraps;
	std::vector<int> craps;
	std::vector<int> rraps;
	std::vector<int> codeWords;
	int lastCluster = -1;
	int xStart = -1, xEnd = -1, lastRowNumber = -1;
	for (int rowNumber = 0; rowNumber < image.height(); rowNumber++) {
		int lrap, crap = -1, rrap;
		int offset = 0;
		PatternRow bars;
		if (!image.getPatternRow(rowNumber, 0 /*rotate*/, bars)) {
			continue;
		}
		PatternView view(bars);
		//printf("rowNumber %d, nRows %d\n", rowNumber, nRows);
		//printf("  view"); for (int i = 0; i < Size(view); i++) { printf(" %d", view[i]); } printf("\n");

		PatternView next = view.subView(0, RAP_CHAR_LEN);
		//printf("  next"); for (int i = 0; i < Size(next); i++) { printf(" %d", next[i]); } printf("\n");

		if ((lrap = DetectLRRAPCode(next)) == -1) {
			//printf("rowNumber %d, LRAP not detected\n", rowNumber);
			continue;
		}
		//printf("rowNumber: %d, LRAP %d detected, row cluster %d\n", rowNumber, lrap, (lrap % 3) * 3);
		if (!lraps.empty() && lraps.back() == lrap) {
			continue;
		}
		int cluster = (lrap % 3) * 3;
		if (lastCluster != -1 && (lastCluster + 3) % 9 != cluster) {
			//printf("  cluster %d, lastCluster %d, out of sync\n", cluster, lastCluster);
			continue;
		}

		xStart = view.pixelsInFront();
		offset = next.size();
		next = view.subView(offset, CHAR_LEN);
		//printf("  1st next offset %d", offset); for (int i = 0; i < Size(next); i++) { printf(" %d", next[i]); } printf("\n");

		int cw1, cw2 = -1, cw3 = -1, cw4 = -1;
		if ((cw1 = DecodeCodeword(next, cluster)) == -1) {
			printf("  cw1 read fail\n");
			continue;
		}
		offset += next.size();
		//printf("  1st cw1 %d, nCols %d, offset %d\n", cw1, nCols, offset);

		if (nCols == -1) {
			int calcOffset = offset;
			next = view.subView(calcOffset, CHAR_LEN);
			//printf("  1st calc next"); for (int i = 0; i < Size(next); i++) { printf(" %d", next[i]); } printf("\n");
			if ((cw2 = DecodeCodeword(next, cluster)) != -1) {
				calcOffset += next.size();
				next = view.subView(calcOffset, RAP_CHAR_LEN);
				if ((rrap = DetectLRRAPCode(next)) != -1) {
					nCols = 2;
				} else if ((crap = DetectCRAPCode(next)) != -1) {
					nCols = 4;
				} else {
					printf("  Calc 2/4 cols, no LRAP or CRAP fail\n");
					continue;
				}
			} else {
				next = view.subView(calcOffset, RAP_CHAR_LEN);
				if ((rrap = DetectLRRAPCode(next)) != -1) {
					//printf("  Calc 1/3 RRAP detected\n");
					nCols = 1;
				} else if ((crap = DetectCRAPCode(next)) != -1) {
					nCols = 3;
				} else {
					printf("  Calc 1/3 cols, no LRAP or CRAP fail\n");
					continue;
				}
			}
			//printf("  nCols %d\n", nCols);
		}
		if (nCols == 1) {
			next = view.subView(offset, RAP_CHAR_LEN);
			if ((rrap = DetectLRRAPCode(next)) == -1) {
				printf("  1-col RRAP read fail\n");
				continue;
			}
		} else if (nCols == 2) {
			next = view.subView(offset, CHAR_LEN);
			if ((cw2 = DecodeCodeword(next, cluster)) == -1) {
				printf("  2-col cw2 read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, RAP_CHAR_LEN);
			if ((rrap = DetectLRRAPCode(next)) == -1) {
				printf("  2-col RRAP read fail\n");
				continue;
			}
		} else if (nCols == 3) {
			next = view.subView(offset, RAP_CHAR_LEN);
			if ((crap = DetectCRAPCode(next)) == -1) {
				printf("  3-col CRAP read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, CHAR_LEN);
			if ((cw2 = DecodeCodeword(next, cluster)) == -1) {
				printf("  3-col cw2 read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, CHAR_LEN);
			if ((cw3 = DecodeCodeword(next, cluster)) == -1) {
				printf("  3-col cw3 read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, RAP_CHAR_LEN);
			if ((rrap = DetectLRRAPCode(next)) == -1) {
				printf("  3-col RRAP read fail\n");
				continue;
			}
		} else { /* nCols == 4 */
			next = view.subView(offset, CHAR_LEN);
			if ((cw2 = DecodeCodeword(next, cluster)) == -1) {
				printf("  4-col cw2 read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, RAP_CHAR_LEN);
			if ((crap = DetectCRAPCode(next)) == -1) {
				printf("  4-col CRAP read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, CHAR_LEN);
			if ((cw3 = DecodeCodeword(next, cluster)) == -1) {
				printf("  4-col cw3 read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, CHAR_LEN);
			if ((cw4 = DecodeCodeword(next, cluster)) == -1) {
				printf("  4-col cw4 read fail\n");
				continue;
			}
			offset += next.size();
			next = view.subView(offset, RAP_CHAR_LEN);
			if ((rrap = DetectLRRAPCode(next)) == -1) {
				printf("  4-col RRAP read fail\n");
				continue;
			}
		}
		offset += next.size();
		//printf("  lrap %d, crap %d, rrap %d, cw1 %d, cw2 %d, cw3 %d, cw4 %d\n", lrap, crap, rrap, cw1, cw2, cw3, cw4);
		lraps.push_back(lrap);
		if (crap != -1) {
			craps.push_back(crap);
		}
		rraps.push_back(rrap);
		codeWords.push_back(cw1);
		if (cw2 != -1) {
			codeWords.push_back(cw2);
		}
		if (cw3 != -1) {
			codeWords.push_back(cw3);
		}
		if (cw4 != -1) {
			codeWords.push_back(cw4);
		}
		lastRowNumber = rowNumber;
		lastCluster = cluster;
		next = view.subView(offset);
		xEnd = next.pixelsTillEnd();
		nRows++;
		if (nRows == 1) {
			tl = PointI(xStart, rowNumber);
			tr = PointI(xEnd, rowNumber);
		}
	}
	bl = PointI(xStart, lastRowNumber + 1); // Hack
	br = PointI(xEnd, lastRowNumber + 1);

#if 0
	printf("nCols %d, nRows %d, codeWords (%d):", nCols, nRows, Size(codeWords)); for (int i = 0; i < Size(codeWords); i++) { printf(" %d", codeWords[i]); } printf("\n");
#endif
	if (Size(codeWords) < 7) {
		return Barcode(DecoderResult(FormatError("< 7 codewords")), DetectorResult{}, BarcodeFormat::MicroPDF417);
	}

	int numECCodewords;
	if (nCols == 1) {
		if (nRows == 11) {
			numECCodewords = 7;
		} else if (nRows == 14) {
			numECCodewords = 7;
		} else if (nRows == 17) {
			numECCodewords = 7;
		} else if (nRows == 20) {
			numECCodewords = 8;
		} else if (nRows == 20) {
			numECCodewords = 8;
		} else if (nRows == 24) {
			numECCodewords = 8;
		} else if (nRows == 28) {
			numECCodewords = 8;
		} else {
			return Barcode(DecoderResult(FormatError("unknown 1-Col Rows combo")), DetectorResult{}, BarcodeFormat::MicroPDF417);
		}
	} else if (nCols == 2) {
		if (nRows == 8) {
			numECCodewords = 8;
		} else if (nRows == 11) {
			numECCodewords = 9;
		} else if (nRows == 14) {
			numECCodewords = 9;
		} else if (nRows == 17) {
			numECCodewords = 10;
		} else if (nRows == 20) {
			numECCodewords = 11;
		} else if (nRows == 23) {
			numECCodewords = 13;
		} else if (nRows == 26) {
			numECCodewords = 15;
		} else {
			return Barcode(DecoderResult(FormatError("unknown 2-Col Rows combo")), DetectorResult{}, BarcodeFormat::MicroPDF417);
		}
	} else if (nCols == 3) {
		if (nRows == 6) {
			numECCodewords = 12;
		} else if (nRows == 8) {
			numECCodewords = 14;
		} else if (nRows == 10) {
			numECCodewords = 16;
		} else if (nRows == 12) {
			numECCodewords = 18;
		} else if (nRows == 15) {
			numECCodewords = 21;
		} else if (nRows == 20) {
			numECCodewords = 26;
		} else if (nRows == 26) {
			numECCodewords = 32;
		} else if (nRows == 32) {
			numECCodewords = 38;
		} else if (nRows == 38) {
			numECCodewords = 44;
		} else if (nRows == 44) {
			numECCodewords = 50;
		} else {
			return Barcode(DecoderResult(FormatError("unknown 3-Col Rows combo")), DetectorResult{}, BarcodeFormat::MicroPDF417);
		}
	} else {
		if (nRows == 4) {
			numECCodewords = 8;
		} else if (nRows == 6) {
			numECCodewords = 12;
		} else if (nRows == 8) {
			numECCodewords = 14;
		} else if (nRows == 10) {
			numECCodewords = 16;
		} else if (nRows == 12) {
			numECCodewords = 18;
		} else if (nRows == 15) {
			numECCodewords = 21;
		} else if (nRows == 20) {
			numECCodewords = 26;
		} else if (nRows == 26) {
			numECCodewords = 32;
		} else if (nRows == 32) {
			numECCodewords = 38;
		} else if (nRows == 38) {
			numECCodewords = 44;
		} else if (nRows == 44) {
			numECCodewords = 50;
		} else {
			return Barcode(DecoderResult(FormatError("unknown 4-Col Rows combo")), DetectorResult{}, BarcodeFormat::MicroPDF417);
		}
	}

	codeWords.insert(codeWords.begin(), Size(codeWords) + 1);
	//printf("codeWords (%d)", Size(codeWords)); for (int i = 0; i < Size(codeWords); i++) { printf(" %d", codeWords[i]); } printf("\n");

	Diagnostics::fmt("  Dimensions: %dx%d (RowsxColumns)\n", nRows, nCols);
	DecoderResult decoderResult = Pdf417::DecodeCodewords(codeWords, numECCodewords);
	return Barcode(std::move(decoderResult), DetectorResult({}, Position(tl, tr, br, bl)), BarcodeFormat::MicroPDF417);
}

static Barcode DecodePure(const BinaryBitmap& image)
{
	Barcode res = DetectSymbol(image);

	if (!res.isValid()) {
		printf("ERROR: %s\n", ToString(res.error()).c_str());
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

} // namespace ZXing::MicroPdf417
