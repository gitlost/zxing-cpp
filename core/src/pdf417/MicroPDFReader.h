/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing {

class DecodeHints;

namespace MicroPdf417 {

class Reader : public ZXing::Reader
{
	bool _formatSpecified;

public:
	explicit Reader(const DecodeHints& hints);
	Result decode(const BinaryBitmap& image) const override;
};

} // MicroPdf417
} // ZXing
