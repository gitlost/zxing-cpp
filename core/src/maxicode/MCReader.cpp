/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "MCReader.h"

#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "MCBitMatrixParser.h"
#include "MCDecoder.h"
#include "Barcode.h"

namespace ZXing::MaxiCode {

/**
* This method detects a code in a "pure" image -- that is, pure monochrome image
* which contains only an unrotated, unskewed, image of a code, with some white border
* around it. This is a specialized method that works exceptionally fast in this special
* case.
*/
static BitMatrix ExtractPureBits(const BitMatrix& image, Position &position)
{
	int left, top, width, height;
	if (!image.findBoundingBox(left, top, width, height, BitMatrixParser::MATRIX_WIDTH)) {
		//printf("!image.findBoundingBox\n");
		return {};
	}

	// Now just read off the bits
	BitMatrix result(BitMatrixParser::MATRIX_WIDTH, BitMatrixParser::MATRIX_HEIGHT);
	for (int y = 0; y < BitMatrixParser::MATRIX_HEIGHT; y++) {
		int iy = top + (y * height + height / 2) / BitMatrixParser::MATRIX_HEIGHT;
		for (int x = 0; x < BitMatrixParser::MATRIX_WIDTH; x++) {
			int ix = left + (x * width + width / 2 + (y & 0x01) *  width / 2) / BitMatrixParser::MATRIX_WIDTH;
			if (image.get(ix, iy)) {
				result.set(x, y);
			}
		}
	}

	//TODO: need to check position info
	position = Position(PointI{left, top}, PointI{left + width - 1, top}, PointI{left + width - 1, top + height - 1}, PointI{left, top + height - 1});

	return result;
}

Barcode Reader::decode(const BinaryBitmap& image) const
{
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr) {
		//printf("fail binImg null\n");
		return {};
	}

	//TODO: this only works with effectively 'pure' barcodes. Needs proper detector.
	Position position;
	BitMatrix bits = ExtractPureBits(*binImg, position);
	if (bits.empty()) {
		//printf("fail bits.empty\n");
		return {};
	}

	DecoderResult decRes = Decode(bits);
	// TODO: before we can meaningfully return a ChecksumError result, we need to check the center for the presence of the finder pattern
	if (!decRes.isValid())
		return {};

	auto res = Barcode(std::move(decRes), DetectorResult{}, BarcodeFormat::MaxiCode);
	res.setPosition(position);
	return res;
}

} // namespace ZXing::MaxiCode
