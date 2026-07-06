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

#include "GMDataBlock.h"

#include "ZXAlgorithms.h"

namespace ZXing::GridMatrix {

/* Table A.1 N1 */
static const char gm_n1[13] = {
	18, 50, 98, 81, 121, 113, 113, 116, 121, 126, 118, 125, 122
};

/* Table A.1 B1 */
static const char gm_b1[13] = {
	1, 1, 1, 2, 2, 2, 2, 3, 2, 7, 5, 10, 6
};

/* Table A.1 B2 */
static const char gm_b2[13] = {
	0, 0, 0, 0, 0, 1, 2, 2, 4, 0, 4, 0, 6
};

/* Table A.1 E1, B3, E2 */
static const char gm_e1b3e2[13][5][3] = {
	/*  E1 B3  E2 */
	{ {  0, 0,  0 }, {  3,  1,  0 }, {  5, 1,  0 }, {  7,  1,  0 }, {  9, 1,  0 } }, /* 1 */
	{ {  5, 1,  0 }, { 10,  1,  0 }, { 15, 1,  0 }, { 20,  1,  0 }, { 25, 1,  0 } }, /* 2 */
	{ {  9, 1,  0 }, { 19,  1,  0 }, { 29, 1,  0 }, { 39,  1,  0 }, { 49, 1,  0 } }, /* 3 */
	{ {  8, 2,  0 }, { 16,  2,  0 }, { 24, 2,  0 }, { 32,  2,  0 }, { 41, 1, 40 } }, /* 4 */
	{ { 12, 2,  0 }, { 24,  2,  0 }, { 36, 2,  0 }, { 48,  2,  0 }, { 61, 1, 60 } }, /* 5 */
	{ { 11, 3,  0 }, { 23,  1, 22 }, { 34, 2, 33 }, { 45,  3,  0 }, { 57, 1, 56 } }, /* 6 */
	{ { 12, 1, 11 }, { 23,  2, 22 }, { 34, 3, 33 }, { 45,  4,  0 }, { 57, 1, 56 } }, /* 7 */
	{ { 12, 2, 11 }, { 23,  5,  0 }, { 35, 3, 34 }, { 47,  1, 46 }, { 58, 4, 57 } }, /* 8 */
	{ { 12, 6,  0 }, { 24,  6,  0 }, { 36, 6,  0 }, { 48,  6,  0 }, { 61, 1, 60 } }, /* 9 */
	{ { 13, 4, 12 }, { 26,  1, 25 }, { 38, 5, 37 }, { 51,  2, 50 }, { 63, 7,  0 } }, /* 10 */
	{ { 12, 6, 11 }, { 24,  4, 23 }, { 36, 2, 35 }, { 47,  9,  0 }, { 59, 7, 58 } }, /* 11 */
	{ { 13, 5, 12 }, { 25, 10,  0 }, { 38, 5, 37 }, { 50, 10,  0 }, { 63, 5, 62 } }, /* 12 */
	{ { 13, 1, 12 }, { 25,  3, 24 }, { 37, 5, 36 }, { 49,  7, 48 }, { 61, 9, 60 } }  /* 13 */
};

std::vector<DataBlock> GetDataBlocks(const ByteArray& rawCodewords, const int version, const int ecLevel)
{
	const int layer_idx = version - 1;
	const int ecLevel_idx = ecLevel - 1;

	const int n1 = gm_n1[layer_idx];
	const int b1 = gm_b1[layer_idx];
	const int n2 = n1 - 1;
	const int b2 = gm_b2[layer_idx];
	const int e1 = gm_e1b3e2[layer_idx][ecLevel_idx][0];
	const int b3 = gm_e1b3e2[layer_idx][ecLevel_idx][1];
	const int e2 = gm_e1b3e2[layer_idx][ecLevel_idx][2];

	const int numBlocks = b1 + b2;
	std::vector<DataBlock> result(numBlocks);

	for (int i = 0; i < numBlocks; i++) {
		const int blockSize = i < b1 ? n1 : n2;
		const int eccLength = i < b3 ? e1 : e2;
		const int dataLength = blockSize - eccLength;
		result[i].numDataCodewords = dataLength;
		auto& blockCodewords = result[i].codewords;
		blockCodewords.resize(dataLength + eccLength, 0);
		for (int j = 0; j < blockSize; j++) {
			blockCodewords[j] = rawCodewords[(b1 + b2) * j + i];
		}
	}
	//printf("%s(%d) %s: numBlocks %d\n", __FILE__, __LINE__, __func__, numBlocks);

	//printf("%s(%d) %s: cw %d\n", __FILE__, __LINE__, __func__, cw); fflush(stdout);

	return result;
}

} // namespace ZXing::GridMatrix
