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

#include "BinaryBitmap.h"
#include "DecodeHints.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "HXDecoder.h"
#include "HXDetector.h"
#include "Diagnostics.h"
#include "Result.h"

namespace ZXing::HanXin {

Reader::Reader(const DecodeHints& hints)
	: ZXing::Reader(hints)
{
	_formatSpecified = hints.hasFormat(BarcodeFormat::HanXin);
}

Result
Reader::decode(const BinaryBitmap& image) const
{
	if (!_formatSpecified) {
		(void)image;
		return Result(DecodeStatus::NotFound);
	}
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr) {
		return Result(DecodeStatus::NotFound);
	}

	auto detectorResult = Detect(*binImg, _hints.tryHarder(), _hints.isPure());

	//printf(" HXReader: detectorResult.isValid() %d\n", (int)detectorResult.isValid());
	if (detectorResult.isValid()) {
		return Result(Decoder::Decode(detectorResult.bits(), _hints.characterSet()), {}, BarcodeFormat::HanXin);
	}
	return Result(DecodeStatus::NotFound);
}

} // namespace ZXing::HanXin
