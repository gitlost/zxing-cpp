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

#include "GMBitMatrixParser.h"

#include "BitMatrix.h"
#include "ByteArray.h"
#include "Diagnostics.h"
#include "ReedSolomon.h"

namespace ZXing::GridMatrix {

/* Get the macromodule index, `x` column, `y` row - see https://stackoverflow.com/a/63909183 */
static int gm_macromodule(const int modules, const int x, const int y) {
    const int r = x < y ? x < modules - y - 1 ? x : modules - y - 1 : y < modules - x - 1 ? y : modules - x - 1;
    const int m = (((modules - 1) >> 1) - r) * 2 + 1;
    const int b = m * m - 1;
    int idx;

    /* Left */
    if (x == r) {
        idx = b - y + r;
    /* Top */
    } else if (y == r) {
        idx = b - (m - 1) * 4 + (x - r);
    /* Right */
    } else if (x == modules - r - 1) {
        idx = b - (m - 1) * 3 + (y - r);
    /* Bottom */
    } else {
        assert(y == modules - r - 1);
        idx = b - (m - 1) - (x - r);
    }
    return idx << 1; /* 2 codewords per macromodule */
}

/* Center layer id, first layer_id */
static const char centerFirstLayerIDs[5][2] = { { 3, 2 }, { 3, 0 }, { 2, 3 }, { 1, 2 }, { 0, 1 }, };

ByteArray BitMatrixParser::ReadCodewords(const BitMatrix& image, int& version, int& ecLevel)
{
	const int size = image.width();

	if (image.height() != size) {
		fprintf(stderr, "%s(%d) %s: image.height() %d != size %d\n", __FILE__, __LINE__, __func__, image.height(), size);
		return {};
	}
	//fprintf(stderr, "%s(%d) %s: size %d\n", __FILE__, __LINE__, __func__, size);

	version = (size - 6) / 12; /* Version == No. of layers */

	const int macromodules_per_dim = 1 + (version * 2);

	//fprintf(stderr, "version %d, size %d, macromodules_per_dim %d\n", version, size, macromodules_per_dim);

	int center_layer_id = -1, first_layer_id = -1;

	int layer_ids[4][2] = { {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0} };
	ByteArray result(macromodules_per_dim * macromodules_per_dim * 2);

    for (int r = 0; r < macromodules_per_dim; r++) {
        for (int c = 0; c < macromodules_per_dim; c++) {
            const int idx = gm_macromodule(macromodules_per_dim, c, r);
			const int y = (r * 6) + 1;
			const int x = (c * 6) + 1;
			int macromodule = 0;
			for (int j = 0; j < 4; j++) {
				for (int i = 0; i < 4; i++) {
					macromodule |= ((int) image.get(x + i, y + j)) << (15 - (j * 4 + i));
				}
			}
			const int layer_id = (macromodule >> 14) & 0x3;
			const int codeword1 = macromodule & 0x7F;
			const int codeword2 = (macromodule >> 7) & 0x7F;
			result[idx] = codeword1;
			result[idx + 1] = codeword2;
			if (idx == 0) {
				center_layer_id = layer_id;
			} else if (idx == 2) {
				first_layer_id = layer_id;
			}
			if (r == c) {
				if (layer_ids[layer_id][0] == -1) {
					layer_ids[layer_id][0] = layer_id;
					layer_ids[layer_id][1] = r;
				} else if (layer_id != layer_ids[layer_id][0]) {
					fprintf(stderr, "bad layers r %d, c %d, layer_id %d\n", r, c, layer_id);
					return {};
				}
			}
		}
	}
	#if 0
	fprintf(stderr, "center_layer_id %d, first %d\n", center_layer_id, first_layer_id);
	fprintf(stderr, "layer_ids { {%d, %d}, {%d, %d}, {%d, %d}, {%d, %d} }\n",
			layer_ids[0][0], layer_ids[0][1], layer_ids[1][0], layer_ids[1][1], layer_ids[2][0], layer_ids[2][1], layer_ids[3][0], layer_ids[3][1]);
	#endif

	ecLevel = 0;
	for (int i = 0; i < 5; i++) {
		if (centerFirstLayerIDs[i][0] == center_layer_id && centerFirstLayerIDs[i][1] == first_layer_id) {
			ecLevel = i + 1;
			break;
		}
	}
	if (!ecLevel) {
		fprintf(stderr, "No ecLevel center/first layer match\n");
		return {};
	}

	return result;
}

} // namespace ZXing::GridMatrix
