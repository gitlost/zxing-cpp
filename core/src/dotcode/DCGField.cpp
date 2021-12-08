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

#include "DCGField.h"

namespace ZXing::DotCode {

static const short _exp3Table[(GField::GF - 1) * 2] = {
	  1,   3,   9,  27,  81,  17,  51,  40,   7,  21,
	 63,  76,   2,   6,  18,  54,  49,  34, 102,  80,
	 14,  42,  13,  39,   4,  12,  36, 108,  98,  68,
	 91,  47,  28,  84,  26,  78,   8,  24,  72, 103,
	 83,  23,  69,  94,  56,  55,  52,  43,  16,  48,
	 31,  93,  53,  46,  25,  75, 112, 110, 104,  86,
	 32,  96,  62,  73, 106,  92,  50,  37, 111, 107,
	 95,  59,  64,  79,  11,  33,  99,  71, 100,  74,
	109, 101,  77,   5,  15,  45,  22,  66,  85,  29,
	 87,  35, 105,  89,  41,  10,  30,  90,  44,  19,
	 57,  58,  61,  70,  97,  65,  82,  20,  60,  67,
	 88,  38,
	  1,   3,   9,  27,  81,  17,  51,  40,   7,  21,
	 63,  76,   2,   6,  18,  54,  49,  34, 102,  80,
	 14,  42,  13,  39,   4,  12,  36, 108,  98,  68,
	 91,  47,  28,  84,  26,  78,   8,  24,  72, 103,
	 83,  23,  69,  94,  56,  55,  52,  43,  16,  48,
	 31,  93,  53,  46,  25,  75, 112, 110, 104,  86,
	 32,  96,  62,  73, 106,  92,  50,  37, 111, 107,
	 95,  59,  64,  79,  11,  33,  99,  71, 100,  74,
	109, 101,  77,   5,  15,  45,  22,  66,  85,  29,
	 87,  35, 105,  89,  41,  10,  30,  90,  44,  19,
	 57,  58,  61,  70,  97,  65,  82,  20,  60,  67,
	 88,  38,
};

static const short _log3Table[GField::GF] = {
	  0,   0,  12,   1,  24,  83,  13,   8,  36,   2,
	 95,  74,  25,  22,  20,  84,  48,   5,  14,  99,
	107,   9,  86,  41,  37,  54,  34,   3,  32,  89,
	 96,  50,  60,  75,  17,  91,  26,  67, 111,  23,
	  7,  94,  21,  47,  98,  85,  53,  31,  49,  16,
	 66,   6,  46,  52,  15,  45,  44, 100, 101,  71,
	108, 102,  62,  10,  72, 105,  87, 109,  29,  42,
	103,  77,  38,  63,  79,  55,  11,  82,  35,  73,
	 19,   4, 106,  40,  33,  88,  59,  90, 110,  93,
	 97,  30,  65,  51,  43,  70,  61, 104,  28,  76,
	 78,  81,  18,  39,  58,  92,  64,  69,  27,  80,
	 57,  68,  56,
};

GField::GField() : GenericGF(0, GField::GF, 1)
{
	_expTable.assign(_exp3Table, _exp3Table + (GField::GF - 1) * 2);
	_logTable.assign(_log3Table, _log3Table + GField::GF);
}

} // namespace ZXing::DotCode
