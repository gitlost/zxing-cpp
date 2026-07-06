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

#include "GMDetector.h"

#include "BitMatrixCursor.h"
#include "DetectorResult.h"
#include "Diagnostics.h"
#include "Pattern.h"

namespace ZXing::GridMatrix {

constexpr auto PATTERN3       = FixedPattern<3, 18>{6, 6, 6};
constexpr int MIN_MODULES = 18; // Version 1 (6 * (1*2 + 1)) == 18
constexpr int MAX_MODULES = 162; // Version 13 (6 * (13*2 + 1)) == 162

/**
* This method detects a code in a "pure" image -- that is, pure monochrome image
* which contains only an unrotated, unskewed, image of a code, with some white border
* around it. This is a specialized method that works exceptionally fast in this special
* case.
*/
static DetectorResult DetectPure(const BitMatrix& image)
{
	using Pattern3 = std::array<PatternView::value_type, PATTERN3.size()>;

	int left, top, width, height;
	if (!image.findBoundingBox(left, top, width, height, MIN_MODULES)) {
		//fprintf(stderr, "FAIL !findBoundingBox\n");
		left = top = 0;
		width = image.width();
		height = image.height();
	}
	if (std::abs(width - height) > 1) {
		//fprintf(stderr, "%s(%d) %s: FAIL std::abs(width - height) %d, width %d, height %d, left %d, top %d\n", __FILE__, __LINE__, __func__, std::abs(width - height), width, height, left, top);
		if (width < height) {
			top += (height - width) / 2;
			height = width;
		} else {
			left += (width - height) / 2;
			width = height;
		}
		//fprintf(stderr, "%s(%d) %s: FAIL width %d, height %d, left %d, top %d\n", __FILE__, __LINE__, __func__, width, height, left, top);
		//return {};
	}

	if (int edges = BitMatrixCursorI(image, {left, top}, CURI_LEFT).countEdges(width - 1); !edges || (edges & 1)) {
		return {};
	}

	if (int edges = BitMatrixCursorI(image, {left, top}, CURI_DOWN).countEdges(height - 1); !edges || (edges & 1)) {
		return {};
	}

	Pattern3 topEdge = BitMatrixCursorI(image, {left, top}, CURI_LEFT).readPatternFromBlack<Pattern3>(1, width);
	if (!IsPattern(topEdge, PATTERN3)) {
		return {};
	}

	int bottom = top + height - 1;
	if (int edges = BitMatrixCursorI(image, {left, bottom}, CURI_LEFT).countEdges(width - 1); !edges || (edges & 1)) {
		return {};
	}

	int right = left + width - 1;
	if (int edges = BitMatrixCursorI(image, {right, top}, CURI_RIGHT).countEdges(height - 1); !edges || (edges & 1)) {
		return {};
	}

	Pattern3 bottomEdge = BitMatrixCursorI(image, {right, bottom}, CURI_RIGHT).readPatternFromBlack<Pattern3>(1, width);
	if (!IsPattern(bottomEdge, PATTERN3)) {
		return {};
	}

	float moduleSize = float(topEdge[0] + topEdge[1] + topEdge[2]) / (6 * 3);

	int dimension = (int) roundf(width / moduleSize);

	//fprintf(stderr, "%s(%d) %s: dimension %d, moduleSize %f\n", __FILE__, __LINE__, __func__, dimension, moduleSize);

	if (dimension < MIN_MODULES || dimension > MAX_MODULES) {
		fprintf(stderr, "FAIL dimension %d\n", dimension);
		return {};
	}
	if (!image.isIn(PointF{left + moduleSize / 2 + (dimension - 1) * moduleSize,
						   top + moduleSize / 2 + (dimension - 1) * moduleSize})) {
		fprintf(stderr, "FAIL !image.isIn\n");
		return {};
	}

	// Now just read off the bits (this is a crop + subsample)
	#if 0
	return {Deflate(image, dimension, dimension, top, left, moduleSize),
			{{left, top}, {right, top}, {right, bottom}, {left, bottom}}};
	#else
	return {Deflate(image, dimension, dimension, top + moduleSize / 2, left + moduleSize / 2, moduleSize),
			{{left, top}, {right, top}, {right, bottom}, {left, bottom}}};
	#endif
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

} // namespace ZXing::GridMatrix
