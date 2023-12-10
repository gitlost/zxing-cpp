/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CharacterSet.h"

namespace ZXing {

class DecoderResult;
class BitMatrix;

namespace QRCode {

DecoderResult Decode(const BitMatrix& bits, const CharacterSet optionsCharset = CharacterSet::Unknown);

} // QRCode
} // ZXing
