/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

#include <list>
#include <string>

namespace ZXing {

class DecodeHints;

namespace Pdf417 {

/**
* This implementation can detect and decode PDF417 codes in an image.
*
* @author Guenther Grau
*/
class Reader : public ZXing::Reader
{
	bool _isPure;
	std::string _characterSet;

public:
	explicit Reader(const DecodeHints& hints);

	Result decode(const BinaryBitmap& image) const override;
	Results decode(const BinaryBitmap& image, int maxSymbols) const override;

	[[deprecated]] std::list<Result> decodeMultiple(const BinaryBitmap& image) const;
};

} // Pdf417
} // ZXing
