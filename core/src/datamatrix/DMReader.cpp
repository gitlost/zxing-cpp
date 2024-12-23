/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "DMReader.h"

#include "BinaryBitmap.h"
#include "DMDecoder.h"
#include "DMDetector.h"
#include "ReaderOptions.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Barcode.h"

#include <utility>

namespace ZXing::DataMatrix {

Barcode Reader::decode(const BinaryBitmap& image) const
{
	//fprintf(stderr, "Reader::decode\n");
#ifdef __cpp_impl_coroutine
	//fprintf(stderr, " calling FirstOrDefault\n");
	return FirstOrDefault(decode(image, 1));
#else
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr) {
		//fprintf(stderr, " binImg NULL\n");
		return {};
	}
	
	auto detectorResult = Detect(*binImg, _opts.tryHarder(), _opts.tryRotate(), _opts.isPure());
	if (!detectorResult.isValid()) {
		//fprintf(stderr, " !isValid\n");
		return {};
	}
	//fprintf(stderr, " isValid\n");

	return Barcode(Decode(detectorResult.bits()), std::move(detectorResult), BarcodeFormat::DataMatrix);
#endif
}

#ifdef __cpp_impl_coroutine
Barcodes Reader::decode(const BinaryBitmap& image, int maxSymbols) const
{
	//fprintf(stderr, "Reader::decode(maxSymbols %d)\n", maxSymbols);
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr) {
		//fprintf(stderr, " binImg NULL\n");
		return {};
	}

	Barcodes res;
	for (auto&& detRes : Detect(*binImg, _opts.tryHarder(), _opts.tryRotate(), _opts.isPure())) {
		auto decRes = Decode(detRes.bits());
		//fprintf(stderr, " decRes.isValid %s\n", decRes.isValid(_opts.returnErrors()) ? "true" : "false");
		if (decRes.isValid(_opts.returnErrors())) {
			res.emplace_back(std::move(decRes), std::move(detRes), BarcodeFormat::DataMatrix);
			if (maxSymbols > 0 && Size(res) >= maxSymbols)
				break;
		}
	}

	return res;
}
#endif
} // namespace ZXing::DataMatrix
