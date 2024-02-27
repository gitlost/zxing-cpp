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

#include "HXReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Diagnostics.h"
#include "ReaderOptions.h"
#include "HXDecoder.h"
#include "HXDetector.h"

namespace ZXing::HanXin {

Reader::Reader(const ReaderOptions& options)
	: ZXing::Reader(options)
{
	_formatSpecified = options.hasFormat(BarcodeFormat::HanXin);
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

	//printf(" HXReader: detectorResult.isValid() %d\n", (int)detectorResult.isValid());
	if (detectorResult.isValid()) {
		return Barcode(Decoder::Decode(detectorResult.bits(), _opts.characterSet()), DetectorResult{}, BarcodeFormat::HanXin);
	}
	return {};
}

} // namespace ZXing::HanXin
