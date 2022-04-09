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

#include "HXDetector.h"

#include "BitMatrixCursor.h"
#include "DetectorResult.h"
#include "Diagnostics.h"
#include "Pattern.h"

namespace ZXing::HanXin {

constexpr auto PATTERN    = FixedPattern<5, 7>{1, 1, 1, 1, 3};
constexpr auto PATTERN_BL = FixedPattern<5, 7>{3, 1, 1, 1, 1};
constexpr int MIN_MODULES = 1 * 2 + 21; // version 1 (23)
constexpr int MAX_MODULES = 84 * 2 + 21; // version 84 (189)

static double EstimateModuleSize(const BitMatrix& image, PointF a, PointF b)
{
	BitMatrixCursorF cur(image, a, b - a);
	//printf("%s(%d) %s: cur.p (%f,%f), cur.d (%f,%f), isBlack %d, distance(a, b) %f\n", __FILE__, __LINE__, __func__, cur.p.x, cur.p.y, cur.d.x, cur.d.y, cur.isBlack(), distance(a, b)); fflush(stdout);
	assert(cur.isBlack());

	if (!cur.stepToEdge(1, (int)(distance(a, b) / 3)))
		return -1;

	cur.turnBack();

	// the following is basically a simple "cur.step()" that reverts the very last step that crossed from back into
	// white, but due to a numerical instability near an integer boundary (think .999999999995) it might be required
	// to back up two steps. See issues #300 and #308.
	if (!cur.stepToEdge(1, 2))
		return -1;

	assert(cur.isBlack());

	auto pattern = cur.readPattern<std::array<int, 4>>();

	return Reduce(pattern) / 6.0 * length(cur.d);
}

struct DimensionEstimate
{
	int dim = 0;
	double ms = 0;
	int err = 0;
};

static DimensionEstimate EstimateDimension(const BitMatrix& image, PointF a, PointF b)
{
	auto ms_a = EstimateModuleSize(image, a, b);
	auto ms_b = EstimateModuleSize(image, b, a);

	if (ms_a < 0 || ms_b < 0)
		return {};

	auto moduleSize = (ms_a + ms_b) / 2;

	int dimension = std::lround(distance(a, b) / moduleSize) + 10;
	int error     = 1 - (dimension % 2);
	//printf("%s(%d), %s: moduleSize %f, dimension %d, error %d\n", __FILE__, __LINE__, __func__, moduleSize, dimension, error);

	return {dimension + error, moduleSize, std::abs(error)};
}

/**
* This method detects a code in a "pure" image -- that is, pure monochrome image
* which contains only an unrotated, unskewed, image of a code, with some white border
* around it. This is a specialized method that works exceptionally fast in this special
* case.
*/
static DetectorResult DetectPure(const BitMatrix& image)
{
	using Pattern = std::array<PatternView::value_type, PATTERN.size()>;
	using PatternBL = std::array<PatternView::value_type, PATTERN_BL.size()>;

	int left, top, width, height;
	if (!image.findBoundingBox(left, top, width, height, MIN_MODULES)) {
		//printf("FAIL !findBoundingBox\n");
		left = top = 0;
		width = image.width();
		height = image.height();
	}
	if (std::abs(width - height) > 1) {
		//printf("%s(%d) %s: FAIL std::abs(width - height) %d, width %d, height %d, left %d, top %d\n", __FILE__, __LINE__, __func__, std::abs(width - height), width, height, left, top);
		if (width < height) {
			top += (height - width) / 2;
			height = width;
		} else {
			left += (width - height) / 2;
			width = height;
		}
		//printf("%s(%d) %s: FAIL width %d, height %d, left %d, top %d\n", __FILE__, __LINE__, __func__, width, height, left, top);
		//return {};
	}

	int right  = left + width - 1;
	int bottom = top + height - 1;

	PointI tl{left, top}, tr{right, top}, bl{left, bottom}, br{right, bottom};
	Pattern diagonal;
	// allow corners be moved one pixel inside to accommodate for possible aliasing artifacts
	for (auto [p, d] : {std::pair(tl, PointI{1, 1}), {tr, {-1, 1}}, {br, {-1, -1}}}) {
		diagonal = BitMatrixCursorI(image, p, d).readPatternFromBlack<Pattern>(1, width / 3);
		if (!IsPattern(diagonal, PATTERN)) {
			printf("%s(%d) %s: FAIL !IsPattern\n", __FILE__, __LINE__, __func__);
			return {};
		}
	}
	if (!IsPattern(BitMatrixCursorI(image, bl, {1, -1}).readPatternFromBlack<PatternBL>(1, width / 3), PATTERN_BL)) {
		printf("%s(%d) %s: FAIL !IsPattern(diagonalBL)\n", __FILE__, __LINE__, __func__);
		return {};
	}

	auto fpWidth = Reduce(diagonal);
	auto dimension = EstimateDimension(image, tl + (fpWidth - fpWidth * 2 / 7) * PointF(1, 1), tr + (fpWidth - fpWidth * 2 / 7) * PointF(-1, 1)).dim;

	float moduleSize = float(width) / dimension;
	//printf("%s(%d) %s: dimension %d, moduleSize %f\n", __FILE__, __LINE__, __func__, dimension, moduleSize);

	if (dimension < MIN_MODULES || dimension > MAX_MODULES) {
		printf("FAIL dimension\n");
		return {};
	}
	if (!image.isIn(PointF{left + moduleSize / 2 + (dimension - 1) * moduleSize,
						   top + moduleSize / 2 + (dimension - 1) * moduleSize})) {
		printf("FAIL !image.isIn\n");
		return {};
	}

	// Now just read off the bits (this is a crop + subsample)
	return {Deflate(image, dimension, dimension, top + moduleSize / 2, left + moduleSize / 2, moduleSize),
			{{left, top}, {right, top}, {right, bottom}, {left, bottom}}};
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

} // namespace ZXing::HanXin
