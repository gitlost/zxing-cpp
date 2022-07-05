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

#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "DecodeHints.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "DCDecoder.h"
#include "DCDetector.h"
#include "Diagnostics.h"
#include "Result.h"

namespace ZXing::DotCode {

Reader::Reader(const DecodeHints& hints)
	: ZXing::Reader(hints)
{
	_formatSpecified = hints.hasFormat(BarcodeFormat::DotCode);
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

	//printf(" DCReader: detectorResult.isValid() %d\n", (int)detectorResult.isValid());
	if (detectorResult.isValid()) {
        DecoderResult decoderResult = Decoder::Decode(detectorResult.bits(), _hints.characterSet());
        //printf("DecoderResult status %d\n", (int)decoderResult.errorCode());
        if (decoderResult.isValid()) {
		    return Result(std::move(decoderResult), {}, BarcodeFormat::DotCode);
        }
	}
	return Result(DecodeStatus::NotFound);
}

} // namespace ZXing::DotCode
