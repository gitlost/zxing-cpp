/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CharacterSet.h"

namespace ZXing {

class DecoderResult;

namespace Aztec {

class DetectorResult;

DecoderResult Decode(const DetectorResult& detectorResult, const CharacterSet optionsCharset = CharacterSet::Unknown);

} // Aztec
} // ZXing
