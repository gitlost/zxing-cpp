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

#include "DCReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "DCDecoder.h"
#include "DCDetector.h"
#include "Diagnostics.h"
#include "ReaderOptions.h"

namespace ZXing::DotCode {

Reader::Reader(const ReaderOptions& options)
	: ZXing::Reader(options)
{
	_formatSpecified = options.hasFormat(BarcodeFormat::DotCode);
}

Barcode
Reader::decode(const BinaryBitmap& image) const
{
	if (!_formatSpecified) {
		(void)image;
		return {};
	}
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr) {
		return {};
	}

	auto detectorResult = Detect(*binImg, _opts.tryHarder(), _opts.isPure());

	//printf(" DCReader: detectorResult.isValid() %d\n", (int)detectorResult.isValid());
	if (detectorResult.isValid()) {
		DecoderResult decoderResult = Decoder::Decode(detectorResult.bits(), _opts.characterSet());
		//printf("DecoderResult status %d\n", (int)decoderResult.errorCode());
		if (decoderResult.isValid()) {
			return Barcode(std::move(decoderResult), DetectorResult{}, BarcodeFormat::DotCode);
		}
	}
	return {};
}

} // namespace ZXing::DotCode
