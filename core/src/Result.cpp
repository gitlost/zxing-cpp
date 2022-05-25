/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

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

Result::Result(const std::string& text, int y, int xStart, int xStop, BarcodeFormat format,
			   std::string&& symbologyIdentifier, ByteArray&& rawBytes, const bool readerInit)
	:
	  _format(format),
	  _content({ByteArray(text), ECI::ISO8859_1}),
	  _text(TextDecoder::FromLatin1(text)),
	  _position(Line(y, xStart, xStop)),
	  _rawBytes(std::move(rawBytes)),
	  _numBits(Size(_rawBytes) * 8),
	  _symbologyIdentifier(std::move(symbologyIdentifier)),
	  _readerInit(readerInit),
	  _lineCount(0)
{
	_numBits = Size(_rawBytes) * 8;

	if (Diagnostics::enabled()) {
		Diagnostics::moveTo(_diagnostics);
	}
}

Result::Result(DecoderResult&& decodeResult, Position&& position, BarcodeFormat format)
	: _status(decodeResult.errorCode()),
	  _format(format),
	  _content(std::move(decodeResult).content()),
	  _text(std::move(decodeResult).text()),
	  _position(std::move(position)),
	  _rawBytes(std::move(decodeResult).rawBytes()),
	  _numBits(decodeResult.numBits()),
	  _ecLevel(decodeResult.ecLevel()),
	  _symbologyIdentifier(decodeResult.symbologyIdentifier()),
	  _sai(decodeResult.structuredAppend()),
	  _isMirrored(decodeResult.isMirrored()),
	  _readerInit(decodeResult.readerInit()),
	  _lineCount(decodeResult.lineCount())
{
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(disable: 4996) /* was declared deprecated */
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
#if defined(__clang__) || defined(__GNUC__)
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

bool Result::operator==(const Result& o) const
{
	if (format() != o.format() || text() != o.text())
		return false;

	if (BarcodeFormats(BarcodeFormat::TwoDCodes).testFlag(format()))
		return IsInside(Center(o.position()), position());

	// if one line is less than half the length of the other away from the
	// latter, we consider it to belong to the same symbol
	auto dTop = maxAbsComponent(o.position().topLeft() - position().topLeft());
	auto dBot = maxAbsComponent(o.position().bottomLeft() - position().topLeft());
	auto length = maxAbsComponent(position().topLeft() - position().bottomRight());

	return std::min(dTop, dBot) < length / 2;
}

} // ZXing
