/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
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

#include "Result.h"

#include "DecoderResult.h"
#include "Diagnostics.h"
#include "TextDecoder.h"

#include <cmath>
#include <utility>

namespace ZXing {

Result::Result(DecodeStatus status) : _status(status)
{
	if (Diagnostics::enabled()) {
		Diagnostics::moveTo(_diagnostics);
	}
}

Result::Result(std::wstring&& text, Position&& position, BarcodeFormat format, ByteArray&& rawBytes,
			   std::string symbologyIdentifier, StructuredAppendInfo sai, const bool readerInit)
	: _format(format), _text(std::move(text)), _position(std::move(position)), _rawBytes(std::move(rawBytes)),
	  _symbologyIdentifier(symbologyIdentifier), _sai(sai), _readerInit(readerInit)
{
	_numBits = Size(_rawBytes) * 8;

	if (Diagnostics::enabled()) {
		Diagnostics::moveTo(_diagnostics);
	}
}

Result::Result(const std::string& text, int y, int xStart, int xStop, BarcodeFormat format, ByteArray&& rawBytes,
			   std::string symbologyIdentifier, const bool readerInit)
	: Result(TextDecoder::FromLatin1(text), Line(y, xStart, xStop), format, std::move(rawBytes), symbologyIdentifier,
			 {}, readerInit)
{}

Result::Result(DecoderResult&& decodeResult, Position&& position, BarcodeFormat format)
	: _status(decodeResult.errorCode()), _format(format), _text(std::move(decodeResult).text()),
	  _position(std::move(position)), _rawBytes(std::move(decodeResult).rawBytes()), _numBits(decodeResult.numBits()),
	  _ecLevel(decodeResult.ecLevel()), _symbologyIdentifier(decodeResult.symbologyIdentifier()),
	  _sai(decodeResult.structuredAppend()), _readerInit(decodeResult.readerInit())
{
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
	if (sequenceSize() != -1) {
		_metadata.put(ResultMetadata::STRUCTURED_APPEND_CODE_COUNT, sequenceSize());
	}
	if (sequenceIndex() != -1) {
		_metadata.put(ResultMetadata::STRUCTURED_APPEND_SEQUENCE, sequenceIndex());
	}
	if (_format == BarcodeFormat::QRCode && !sequenceId().empty()) {
		_metadata.put(ResultMetadata::STRUCTURED_APPEND_PARITY, std::stoi(sequenceId()));
	}
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

	// TODO: add type opaque and code specific 'extra data'? (see DecoderResult::extra())

	if (Diagnostics::enabled()) {
		Diagnostics::moveTo(_diagnostics);
	}
}

int Result::orientation() const
{
	constexpr auto std_numbers_pi_v = 3.14159265358979323846; // TODO: c++20 <numbers>
	return std::lround(_position.orientation() * 180 / std_numbers_pi_v);
}

} // ZXing
