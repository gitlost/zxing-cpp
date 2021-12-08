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

#include "DCDetector.h"

#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "BitMatrixCursor.h"
#include "ByteMatrix.h"
#include "DetectorResult.h"
#include "Diagnostics.h"
#include "Pattern.h"
#include "Point.h"
#include "RegressionLine.h"
#include "ResultPoint.h"
#include "WhiteRectDetector.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

namespace ZXing::DotCode {

#if 0
static void print_row(const PatternRow& row, const char *prefix = NULL) {
	if (prefix && *prefix) fputs(prefix, stdout);
	for (int i = 0; i < Size(row); i++) printf(" %d:%d,", i, row[i]);
	printf("\n");
}
#endif

/**
* This method detects a code in a "pure" image -- that is, pure monochrome image
* which contains only an unrotated, unskewed, image of a code, with some white border
* around it. This is a specialized method that works exceptionally fast in this special
* case.
*/
static DetectorResult DetectPure(const BitMatrix& image)
{
	int left, top, width, height;
	if (!image.findBoundingBox(left, top, width, height, 8)) {
		//printf("FAIL !findBoundingBox\n");
		left = top = 0;
		width = image.width();
		height = image.height();
	}

	PointI center(left + width / 2, top + height / 2);
	//printf("  findBoundingBox left %d, top %d, width %d, height %d, image.height %d, width %d, center.x %d, center.y %d\n", left, top, width, height, image.height(), image.width(), center.x, center.y);

	const int yHeight = height < image.height() ? height + 1 : image.height();

	int hMin = width, vMin = yHeight;
	int prev_y = -1;
	for (int y = top; y < yHeight; y++) {
		PatternRow row;
		image.getPatternRow(y, row);
		if (left) {
			row[0] -= left;
		}
		//printf("  row %d: ", y); print_row(row, "");
		if (Size(row) != 1) {
			if (prev_y == -1) {
				prev_y = y;
			} else {
				if (y - prev_y < vMin) {
					vMin = y - prev_y;
				}
			}
		}
		for (int x = 1; x < Size(row); x += 2) {
			if (x != 1) {
				if (row[x - 1] < hMin) {
					hMin = row[x - 1];
				}
			}
		}
	}
	#if 0
	if ((hMin & 1) && (vMin & 1)) {
		printf("FAIL both hMin %d and vMin %d odd\n", hMin, vMin);
		return {};
	}
	if (std::abs(hMin - vMin) != 1) {
		printf("FAIL hMin %d and vMin %d not within one of each other\n", hMin, vMin);
		return {};
	}
	#endif

	const int modSize = hMin < vMin ? hMin : vMin;
	const int bitsWidth = (width + modSize - 1) / modSize;
	const int bitsHeight = (height + modSize - 1) / modSize;
	//printf("  hMin %d, vMin %d, bitsWidth %d, bitsHeight %d, modSize %d\n", hMin, vMin, bitsWidth, bitsHeight, modSize);
	if (bitsWidth < 5 || bitsHeight < 5) {
		return {};
	}

	#if 0
	BitMatrix bits = Deflate(image, bitsWidth, bitsHeight, top + modSize / 2, left + modSize / 2, modSize);
	#else
	BitMatrix bits(bitsWidth, bitsHeight);
	for (int y = top, by = 0; y < image.height() && by < bitsHeight; y += modSize, by++) {
		for (int x = left, bx = 0; x < image.width() && bx < bitsWidth; x += modSize, bx++) {
			//printf("   %d", image.get(x, y));
			if (image.get(x, y)) {
				bits.set(bx, by);
			}
		}
		//printf("\n");
	}
	//printf("\n");
	#endif

	const int right  = left + width - 1;
	const int bottom = top + height - 1;

	return {std::move(bits), {{left, top}, {right, top}, {right, bottom}, {left, bottom}}};
}

DetectorResult Detect(const BitMatrix& image, bool tryHarder, bool isPure)
{
	(void)tryHarder; (void)isPure;
	return DetectPure(image);
	#if 0
	if (isPure) {
		return DetectPure(image);
	}

	auto result = Detect(image, tryHarder);
	if (!result.isValid() && tryHarder) {
		result = DetectPure(image);
	}
	return result;
	#endif
}

} // namespace ZXing::DotCode
