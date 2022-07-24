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

#include "DCDataBlock.h"

#include "DCGField.h"
#include "ZXAlgorithms.h"

namespace ZXing::DotCode {

std::vector<DataBlock> GetDataBlocks(const ByteArray& rawCodewords)
{
	int nw = Size(rawCodewords);
	int nc = nw / 3 + 2;
	int nd = nw - nc;
    int step = (nw + GField::GF - 2) / (GField::GF - 1);
	//printf("nw %d, nc %d, nd %d, step %d\n", nw, nc, nd, step);

	std::vector<DataBlock> result(step);

    for (int start = 0; start < step; start++) {
		int ND = (nd - start + step - 1) / step;
		int NW = (nw - start + step - 1) / step;

		result[start].numDataCodewords = ND;
		ByteArray& blockCodewords = result[start].codewords;
		blockCodewords.resize(NW, 0);

		for (int i = 0; i < NW; i++) {
			blockCodewords[i] = rawCodewords[start + i * step];
		}
	}

	return result;
}

} // namespace ZXing::DotCode
