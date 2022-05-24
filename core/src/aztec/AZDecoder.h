/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

namespace ZXing {

class DecoderResult;

namespace Aztec {

class DetectorResult;

/**
 * @brief Decode Aztec Code after locating and extracting from an image.
 */
DecoderResult Decode(const DetectorResult& detectorResult, const std::string& characterSet = "");

} // Aztec
} // ZXing
