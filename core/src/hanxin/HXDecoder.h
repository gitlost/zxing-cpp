#pragma once
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

#include "CharacterSet.h"
#include <string>

namespace ZXing {

class DecoderResult;
class BitMatrix;

namespace HanXin {

/**
* <p>The main class which implements HanXin decoding -- as opposed to locating and extracting
* the HanXin from an image.</p>
*/
class Decoder
{
public:
	static DecoderResult Decode(const BitMatrix& bits, const CharacterSet characterSet);
};

} // HanXin
} // ZXing
