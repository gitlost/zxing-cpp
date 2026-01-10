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
#include "BarcodeData.h"

#include <utility>

namespace ZXing::DataMatrix {

BarcodesData Reader::read(const BinaryBitmap& image, int maxSymbols) const
{
	//fprintf(stderr, "Reader::decode\n");
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr) {
		//fprintf(stderr, " binImg NULL\n");
		return {};
	}

	BarcodesData res;
	for (auto&& detRes : Detect(*binImg, _opts.tryHarder(), _opts.tryRotate(), _opts.isPure())) {
		auto decRes = Decode(detRes.bits());
		//fprintf(stderr, " decRes.isValid %s\n", decRes.isValid(_opts.returnErrors()) ? "true" : "false");
		if (decRes.isValid(_opts.returnErrors())) {
			res.emplace_back(MatrixBarcode(std::move(decRes), std::move(detRes), BarcodeFormat::DataMatrix));
			if (maxSymbols > 0 && Size(res) >= maxSymbols)
				break;
		}
	}

	return res;
}

} // namespace ZXing::DataMatrix
