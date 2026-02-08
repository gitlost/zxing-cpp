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

#include "DCBitMatrixParser.h"

#include "BitMatrix.h"
#include "ByteArray.h"
#include "DCGField.h"
#include "Diagnostics.h"

namespace ZXing::DotCode {

static int lookupPattern(int pattern)
{
	static const unsigned short dot_patterns[GField::GF][2] = {
		{ 0x02f, 93 },
		{ 0x037, 94 },
		{ 0x03b, 95 },
		{ 0x03d, 96 },
		{ 0x04f, 97 },
		{ 0x057, 27 },
		{ 0x05b, 28 },
		{ 0x05d, 29 },
		{ 0x05e, 63 },
		{ 0x067, 98 },
		{ 0x06b, 30 },
		{ 0x06d, 31 },
		{ 0x06e, 64 },
		{ 0x073, 99 },
		{ 0x075, 32 },
		{ 0x076, 65 },
		{ 0x079, 100 },
		{ 0x07a, 66 },
		{ 0x08f, 101 },
		{ 0x097, 33 },
		{ 0x09b, 34 },
		{ 0x09d, 35 },
		{ 0x09e, 67 },
		{ 0x0a7, 36 },
		{ 0x0ab, 1 },
		{ 0x0ad, 2 },
		{ 0x0ae, 9 },
		{ 0x0b3, 37 },
		{ 0x0b5, 3 },
		{ 0x0b6, 10 },
		{ 0x0b9, 38 },
		{ 0x0ba, 11 },
		{ 0x0bc, 68 },
		{ 0x0c7, 102 },
		{ 0x0cb, 39 },
		{ 0x0cd, 40 },
		{ 0x0ce, 69 },
		{ 0x0d3, 41 },
		{ 0x0d5, 4 },
		{ 0x0d6, 12 },
		{ 0x0d9, 42 },
		{ 0x0da, 13 },
		{ 0x0dc, 70 },
		{ 0x0e3, 103 },
		{ 0x0e5, 43 },
		{ 0x0e6, 71 },
		{ 0x0e9, 44 },
		{ 0x0ea, 14 },
		{ 0x0ec, 72 },
		{ 0x0f1, 104 },
		{ 0x0f2, 73 },
		{ 0x0f4, 74 },
		{ 0x117, 75 },
		{ 0x11b, 76 },
		{ 0x11d, 77 },
		{ 0x11e, 105 },
		{ 0x127, 78 },
		{ 0x12b, 15 },
		{ 0x12d, 16 },
		{ 0x12e, 45 },
		{ 0x133, 79 },
		{ 0x135, 17 },
		{ 0x136, 46 },
		{ 0x139, 80 },
		{ 0x13a, 47 },
		{ 0x13c, 106 },
		{ 0x147, 81 },
		{ 0x14b, 18 },
		{ 0x14d, 19 },
		{ 0x14e, 48 },
		{ 0x153, 20 },
		{ 0x155, 0 },
		{ 0x156, 5 },
		{ 0x159, 21 },
		{ 0x15a, 6 },
		{ 0x15c, 49 },
		{ 0x163, 82 },
		{ 0x165, 22 },
		{ 0x166, 50 },
		{ 0x169, 23 },
		{ 0x16a, 7 },
		{ 0x16c, 51 },
		{ 0x171, 83 },
		{ 0x172, 52 },
		{ 0x174, 53 },
		{ 0x178, 107 },
		{ 0x18b, 84 },
		{ 0x18d, 85 },
		{ 0x18e, 108 },
		{ 0x193, 86 },
		{ 0x195, 24 },
		{ 0x196, 54 },
		{ 0x199, 87 },
		{ 0x19a, 55 },
		{ 0x19c, 109 },
		{ 0x1a3, 88 },
		{ 0x1a5, 25 },
		{ 0x1a6, 56 },
		{ 0x1a9, 26 },
		{ 0x1aa, 8 },
		{ 0x1ac, 57 },
		{ 0x1b1, 89 },
		{ 0x1b2, 58 },
		{ 0x1b4, 59 },
		{ 0x1b8, 110 },
		{ 0x1c5, 90 },
		{ 0x1c6, 111 },
		{ 0x1c9, 91 },
		{ 0x1ca, 60 },
		{ 0x1cc, 112 },
		{ 0x1d1, 92 },
		{ 0x1d2, 61 },
		{ 0x1d4, 62 },
	};

	int s = 0, e = GField::GF - 1;

	while (s <= e) {
		int m = (s + e) / 2;
		if (dot_patterns[m][0] < pattern) {
			s = m + 1;
		} else if (dot_patterns[m][0] > pattern) {
			e = m - 1;
		} else {
			return dot_patterns[m][1];
		}
	}

	return -1;
}

struct State {
	int pattern = 0;
	int pattern_i = 0;
	int count = 0;
};

static void addCodeword(struct State& state, bool val, ByteArray& result, std::vector<int>& erasureLocs)
{
	state.pattern <<= 1;
	if (val) {
		state.pattern |= 1;
	}
	if (++state.pattern_i == 9) {
		//fprintf(stderr, " %X", state.pattern);
		int codeword = lookupPattern(state.pattern);
		if (codeword == -1) {
			if (state.pattern != 0x1ff) { // Padding
				erasureLocs.push_back(state.count);
				Diagnostics::fmt("  %d: UnknownPattern: 0x%X", state.count, state.pattern);
			}
			codeword = 0;
		}
		result.push_back(codeword);
		state.pattern = state.pattern_i = 0;
		state.count++;
	}
}

ByteArray BitMatrixParser::ReadCodewords(const BitMatrix& image, std::vector<int>& erasureLocs)
{
	ByteArray result;
	int height = image.height();
	int width = image.width();
	struct State state;
	int mask = -1, mask1 = -1;
	constexpr int erasureEarlyCutoff = 20;

	if (width & 0x1) {
		for (int x = 0; x < width; x++) { // Left to right
			for (int y = x & 1 ? 1 : 0; y < height; y += 2) { // Down
				if ((x == 0 && (y == 0 || y == height - 2)) // 6 & 2
						|| (x == 1 && y == height - 1) // 4
						|| (x == width - 2 && y == height - 1) // 3
						|| (x == width - 1 && (y == 0 || y == height - 2))) { // 5 & 1
					continue;
				}
				if (mask == -1) {
					const int xy = image.get(x, y) ? 1 : 0;
					if (mask1 == -1) {
						mask1 = xy;
					} else {
						mask = (mask1 << 1) | xy;
						result.push_back(mask);
					}
					continue;
				}
				addCodeword(state, image.get(x, y), result, erasureLocs);
			}
			if (erasureLocs.size() > erasureEarlyCutoff && erasureLocs.size() * 2 > result.size()) {
				//fprintf(stderr, "BitMatrixParser::ReadCodewords: fail erasureLocs.size() %d, erasureEarlyCutoff %d, result.size() %d\n", (int) erasureLocs.size(), erasureEarlyCutoff, (int) result.size());
				return {};
			}
		}
		addCodeword(state, image.get(width - 1, height - 2), result, erasureLocs); // 1
		addCodeword(state, image.get(0, height - 2), result, erasureLocs); // 2
		addCodeword(state, image.get(width - 2, height - 1), result, erasureLocs); // 3
		addCodeword(state, image.get(1, height - 1), result, erasureLocs); // 4
		addCodeword(state, image.get(width - 1, 0), result, erasureLocs); // 5
		addCodeword(state, image.get(0, 0), result, erasureLocs); // 6
	} else {
		for (int y = height - 1; y >= 0; y--) { // Up
			for (int x = y & 1 ? 1 : 0; x < width; x += 2) { // Left to right
				if ((y == height - 1 && (x == 0 || x == width - 2)) // 6 & 2
						|| (y == height - 2 && x == width - 1) // 4
						|| (y == 1 && x == width - 1) // 3
						|| (y == 0 && (x == 0 || x == width - 2))) { // 5 & 1
					continue;
				}
				if (mask == -1) {
					const int xy = image.get(x, y) ? 1 : 0;
					if (mask1 == -1) {
						mask1 = xy;
					} else {
						mask = (mask1 << 1) | xy;
						result.push_back(mask);
					}
					continue;
				}
				addCodeword(state, image.get(x, y), result, erasureLocs);
			}
			if (erasureLocs.size() > erasureEarlyCutoff && erasureLocs.size() * 2 > result.size()) {
				#if 0
				fprintf(stderr, " BitMatrixParser::ReadCodewords: fail, erasureLocs.size() %d, erasureEarlyCutoff %d, result.size() %d\n",
						(int) erasureLocs.size(), erasureEarlyCutoff, (int) result.size());
				#endif
				return {};
			}
		}
		addCodeword(state, image.get(width - 2, 0), result, erasureLocs); // 1
		addCodeword(state, image.get(width - 2, height - 1), result, erasureLocs); // 2
		addCodeword(state, image.get(width - 1, 1), result, erasureLocs); // 3
		addCodeword(state, image.get(width - 1, height - 2), result, erasureLocs); // 4
		addCodeword(state, image.get(0, 0), result, erasureLocs); // 5
		addCodeword(state, image.get(0, height - 1), result, erasureLocs); // 6
	}

	if (result.size() % 3 == 0) { // Section 11.5 "total number of codewords ... shall NOT be a multiple of 3"
		result.pop_back(); // Discard padding
		// TODO: fix erasureLocs
	}
	if (erasureLocs.size() > 1 && erasureLocs.size() * 14 > result.size()) { // ~7%
		//fprintf(stderr, " BitMatrixParser::ReadCodewords: fail, erasureLocs.size() %d, result.size() %d\n", (int) erasureLocs.size(), (int) result.size());
		return {};
	}

	return result;
}

} // namespace ZXing::DotCode
