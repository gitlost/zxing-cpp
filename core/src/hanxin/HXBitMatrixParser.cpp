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

#include "HXBitMatrixParser.h"

#include "BitMatrix.h"
#include "ByteArray.h"
#include "Diagnostics.h"
#include "GenericGF.h"
#include "ReedSolomonDecoder.h"

namespace ZXing::HanXin {

/* Avoid plotting outside symbol or over finder patterns */
static void safePlot(unsigned char *grid, const int size, const int x, const int y, const int value)
{
	if (x >= 0 && x < size && y >= 0 && y < size && grid[(y * size) + x] == 0) {
		grid[(y * size) + x] = value;
	}
}

/* Finder pattern */
static void placeFinder(unsigned char *grid, const int size, const int x, const int y)
{
	for (int xp = 0; xp < 7; xp++) {
		for (int yp = 0; yp < 7; yp++) {
			safePlot(grid, size, xp + x, yp + y, 0x10);
		}
	}
}

/* Plot an alignment pattern around top and right of a module */
static void plotAlignment(unsigned char *grid, const int size, const int x, const int y, const int w, const int h)
{
	safePlot(grid, size, x, y, 0x10);
	safePlot(grid, size, x - 1, y + 1, 0x10);

	for (int i = 1; i <= w; i++) {
		/* Top */
		safePlot(grid, size, x - i, y, 0x10);
		safePlot(grid, size, x - i - 1, y + 1, 0x10);
	}

	for (int i = 1; i < h; i++) {
		/* Right */
		safePlot(grid, size, x, y + i, 0x10);
		safePlot(grid, size, x - 1, y + i + 1, 0x10);
	}
}

/* Plot assistant alignment patterns */
static void plotAssistant(unsigned char *grid, const int size, const int x, const int y)
{
	safePlot(grid, size, x - 1, y - 1, 0x10);
	safePlot(grid, size, x, y - 1, 0x10);
	safePlot(grid, size, x + 1, y - 1, 0x10);
	safePlot(grid, size, x - 1, y, 0x10);
	safePlot(grid, size, x, y, 0x10);
	safePlot(grid, size, x + 1, y, 0x10);
	safePlot(grid, size, x - 1, y + 1, 0x10);
	safePlot(grid, size, x, y + 1, 0x10);
	safePlot(grid, size, x + 1, y + 1, 0x10);
}

// Bogus gcc warning
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#endif

/* Put static elements in the grid */
static void setupGrid(unsigned char *grid, const int size, const int version)
{
	/* Value 'k' from Annex A */
	static const char kModule[] = {
		0, 0, 0, 14, 16, 16, 17, 18, 19, 20,
		14, 15, 16, 16, 17, 17, 18, 19, 20, 20,
		21, 16, 17, 17, 18, 18, 19, 19, 20, 20,
		21, 17, 17, 18, 18, 19, 19, 19, 20, 20,
		17, 17, 18, 18, 18, 19, 19, 19, 17, 17,
		18, 18, 18, 18, 19, 19, 19, 17, 17, 18,
		18, 18, 18, 19, 19, 17, 17, 17, 18, 18,
		18, 18, 19, 19, 17, 17, 17, 18, 18, 18,
		18, 18, 17, 17
	};

	/* Value 'r' from Annex A */
	static const char rModule[] = {
		0, 0, 0, 15, 15, 17, 18, 19, 20, 21,
		15, 15, 15, 17, 17, 19, 19, 19, 19, 21,
		21, 17, 16, 18, 17, 19, 18, 20, 19, 21,
		20, 17, 19, 17, 19, 17, 19, 21, 19, 21,
		18, 20, 17, 19, 21, 18, 20, 22, 17, 19,
		15, 17, 19, 21, 17, 19, 21, 18, 20, 15,
		17, 19, 21, 16, 18, 17, 19, 21, 15, 17,
		19, 21, 15, 17, 18, 20, 22, 15, 17, 19,
		21, 23, 17, 19
	};

	/* Value of 'm' from Annex A */
	static const char mModule[] = {
		0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 3, 3, 3,
		3, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 7, 7, 7,
		7, 7, 7, 7, 7, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 9, 9, 9, 9, 9, 9,
		9, 9, 10, 10
	};

	memset(grid, 0, (size_t) size * size);

	/* Add finder patterns */
	placeFinder(grid, size, 0, 0);
	placeFinder(grid, size, 0, size - 7);
	placeFinder(grid, size, size - 7, 0);
	placeFinder(grid, size, size - 7, size - 7);

	/* Add finder pattern separator region */
	for (int i = 0; i < 8; i++) {
		/* Top left */
		grid[(7 * size) + i] = 0x10;
		grid[(i * size) + 7] = 0x10;

		/* Top right */
		grid[(7 * size) + (size - i - 1)] = 0x10;
		grid[((size - i - 1) * size) + 7] = 0x10;

		/* Bottom left */
		grid[(i * size) + (size - 8)] = 0x10;
		grid[((size - 8) * size) + i] = 0x10;

		/* Bottom right */
		grid[((size - 8) * size) + (size - i - 1)] = 0x10;
		grid[((size - i - 1) * size) + (size - 8)] = 0x10;
	}

	/* Reserve function information region */
	for (int i = 0; i < 9; i++) {
		/* Top left */
		grid[(8 * size) + i] = 0x10;
		grid[(i * size) + 8] = 0x10;

		/* Top right */
		grid[(8 * size) + (size - i - 1)] = 0x10;
		grid[((size - i - 1) * size) + 8] = 0x10;

		/* Bottom left */
		grid[(i * size) + (size - 9)] = 0x10;
		grid[((size - 9) * size) + i] = 0x10;

		/* Bottom right */
		grid[((size - 9) * size) + (size - i - 1)] = 0x10;
		grid[((size - i - 1) * size) + (size - 9)] = 0x10;
	}

	if (version > 3) {
		const int k = kModule[version - 1];
		const int r = rModule[version - 1];
		const int m = mModule[version - 1];
		int x, y, row_switch, column_switch;
		int module_height, module_width;
		int mod_x, mod_y;

		/* Add assistant alignment patterns to left and right */
		y = 0;
		mod_y = 0;
		do {
			if (mod_y < m) {
				module_height = k;
			} else {
				module_height = r - 1;
			}

			if ((mod_y & 1) == 0) {
				if ((m & 1) == 1) {
					plotAssistant(grid, size, 0, y);
				}
			} else {
				if ((m & 1) == 0) {
					plotAssistant(grid, size, 0, y);
				}
				plotAssistant(grid, size, size - 1, y);
			}

			mod_y++;
			y += module_height;
		} while (y < size);

		/* Add assistant alignment patterns to top and bottom */
		x = (size - 1);
		mod_x = 0;
		do {
			if (mod_x < m) {
				module_width = k;
			} else {
				module_width = r - 1;
			}

			if ((mod_x & 1) == 0) {
				if ((m & 1) == 1) {
					plotAssistant(grid, size, x, (size - 1));
				}
			} else {
				if ((m & 1) == 0) {
					plotAssistant(grid, size, x, (size - 1));
				}
				plotAssistant(grid, size, x, 0);
			}

			mod_x++;
			x -= module_width;
		} while (x >= 0);

		/* Add alignment pattern */
		column_switch = 1;
		y = 0;
		mod_y = 0;
		do {
			if (mod_y < m) {
				module_height = k;
			} else {
				module_height = r - 1;
			}

			if (column_switch == 1) {
				row_switch = 1;
				column_switch = 0;
			} else {
				row_switch = 0;
				column_switch = 1;
			}

			x = (size - 1);
			mod_x = 0;
			do {
				if (mod_x < m) {
					module_width = k;
				} else {
					module_width = r - 1;
				}

				if (row_switch == 1) {
					if (!(y == 0 && x == (size - 1))) {
						plotAlignment(grid, size, x, y, module_width, module_height);
					}
					row_switch = 0;
				} else {
					row_switch = 1;
				}
				mod_x++;
				x -= module_width;
			} while (x >= 0);

			mod_y++;
			y += module_height;
		} while (y < size);
	}
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

static bool getFunctionalInfo(const BitMatrix& image, const int size, int &version, int &ecLevel, int &mask)
{
	std::vector<int> funcInfo(7, 0);

	// Version
	funcInfo[0] = ((int)image.get(0, 8) << 3) | ((int)image.get(1, 8) << 2)
					| ((int)image.get(2, 8) << 1) | (int)image.get(3, 8);
	funcInfo[1] = ((int)image.get(4, 8) << 3) | ((int)image.get(5, 8) << 2)
					| ((int)image.get(6, 8) << 1) | (int)image.get(7, 8);
	// EC level & mask
	funcInfo[2] = ((int)image.get(8, 8) << 3) | ((int)image.get(8, 7) << 2)
					| ((int)image.get(8, 6) << 1) | (int)image.get(8, 5);

	// Reed-Solomon
	const int c = size - 9;
	funcInfo[3] = ((int)image.get(8, 4) << 3) | ((int)image.get(8, 3) << 2)
					| ((int)image.get(8, 2) << 1) | (int)image.get(8, 1);
	funcInfo[4] = ((int)image.get(8, 0) << 3) | ((int)image.get(c, 0) << 2)
					| ((int)image.get(c, 1) << 1) | (int)image.get(c, 2);
	funcInfo[5] = ((int)image.get(c, 3) << 3) | ((int)image.get(c, 4) << 2)
					| ((int)image.get(c, 5) << 1) | (int)image.get(c, 6);
	funcInfo[6] = ((int)image.get(c, 7) << 3) | ((int)image.get(c, 8) << 2)
					| ((int)image.get(c + 1, 8) << 1) | (int)image.get(c + 2, 8);

	#if 0
	fprintf(stderr, "%s(%d) %s:", __FILE__, __LINE__, __func__);
	for (int i = 0; i < 7; i++) {
		fprintf(stderr, " funcInfo[%d] 0x%X", i, funcInfo[i]);
	}
	fprintf(stderr, "\n");
	#endif

	if (!ReedSolomonDecode(GenericGF::HanXinFuncInfo(), funcInfo, 4)) {
		version = ecLevel = mask = 0;
		return false;
	}

	version = ((funcInfo[0] << 4) | funcInfo[1]) - 20;
	ecLevel = (funcInfo[2] >> 2) + 1;
	mask = funcInfo[2] & 0x03;

	return true;
}

ByteArray BitMatrixParser::ReadCodewords(const BitMatrix& image, int& version, int& ecLevel, int& mask)
{
	int size = image.width();

	if (image.height() != size) {
		//fprintf(stderr, "%s(%d) %s: image.height() %d != size %d\n", __FILE__, __LINE__, __func__, image.height(), size);
		return {};
	}
	//fprintf(stderr, "%s(%d) %s: size %d\n", __FILE__, __LINE__, __func__, size);

	if (!getFunctionalInfo(image, size, version, ecLevel, mask)) {
		//fprintf(stderr, "%s(%d) %s: Fail(getFunctionalInfo)\n", __FILE__, __LINE__, __func__); fflush(stdout);
		BitMatrix inverted = image.copy();
		inverted.rotate180();
		if (!getFunctionalInfo(inverted, size, version, ecLevel, mask)) {
			//fprintf(stderr, "%s(%d) %s: Fail(getFunctionalInfo2)\n", __FILE__, __LINE__, __func__); fflush(stdout);
		}
		//Diagnostics::put("Fail(RSDecodeFuncInfo)");
		//return {};
	}
	//fprintf(stderr, "%s(%d) %s: version %d, ecLevel %d, mask %d\n", __FILE__, __LINE__, __func__, version, ecLevel, mask);

	if (size != version * 2 + 21) {
		//fprintf(stderr, "%s(%d) %s: size %d != version %d * 2 + 21\n", __FILE__, __LINE__, __func__, size, version);
		return {};
	}

	unsigned char grid[189 * 189];
	setupGrid(grid, size, version);

	ByteArray batched;
	int codeword = 0;
	int bit = 0;
	for (int y = 0; y < size; y++) {
		const int yw = y * size;
		int i = y + 1;
		for (int x = 0; x < size; x++) {
			if (grid[yw + x] == 0) {
				int v = image.get(x, y);
				int j = x + 1;
				codeword <<= 1;
				bit++;
				if (mask == 1) {
					if (((i + j) & 1) == 0) {
						v = !v;
					}
				} else if (mask == 2) {
					if ((((i + j) % 3 + (j % 3)) & 1) == 0) {
						v = !v;
					}
				} else if (mask == 3) {
					if (((i % j + j % i + i % 3 + j % 3) & 1) == 0) {
						v = !v;
					}
				}
				if (v) {
					codeword |= 1;
				}
				if (bit == 8) {
					batched.push_back(codeword);
					codeword = 0;
					bit = 0;
				}
			}
		}
	}

	//Diagnostics::fmt("  Batched:    (%d)", Size(batched)); Diagnostics::dump(batched, "\n", -1, -1, true /*hex*/);
	ByteArray result(Size(batched));
	for (int start = 0, j = 0; start < 13; start++) {
		for (int i = start; i < Size(batched); i += 13) {
			result[i] = batched[j++];
		}
	}


	return result;
}

} // namespace ZXing::HanXin
