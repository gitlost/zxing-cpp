/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing {

class ReaderOptions;

namespace CodablockF {

class Reader : public ZXing::Reader
{
	bool _formatSpecified;

public:
	explicit Reader(const ReaderOptions& options);
	BarcodesData read(const BinaryBitmap& image, int maxSymbols) const override;
};

} // CodablockF
} // ZXing
