/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "Result.h"

#include "ECI.h"
#include "DecoderResult.h"
#include "Diagnostics.h"
#include "TextDecoder.h"

#include <cmath>
#include <list>
#include <map>
#include <utility>

namespace ZXing {

Result::Result(DecodeStatus status) : _status(status)
{
	if (Diagnostics::enabled()) {
		Diagnostics::moveTo(_diagnostics);
	}
}

Result::Result(const std::string& text, int y, int xStart, int xStop, BarcodeFormat format,
			   SymbologyIdentifier si, ByteArray&& rawBytes, const bool readerInit)
	:
	  _format(format),
	  _content({ByteArray(text)}, si),
	  _text(TextDecoder::FromLatin1(text)),
	  _position(Line(y, xStart, xStop)),
	  _rawBytes(std::move(rawBytes)),
	  _numBits(Size(_rawBytes) * 8),
	  _symbologyIdentifier(si.toString()),
	  _readerInit(readerInit),
	  _lineCount(0)
{
	if (Diagnostics::enabled()) {
		Diagnostics::moveTo(_diagnostics);
	}
}

Result::Result(DecoderResult&& decodeResult, Position&& position, BarcodeFormat format)
	: _status(decodeResult.errorCode()),
	  _format(format),
	  _content(std::move(decodeResult).content()),
	  _text(_content.text()),
	  _position(std::move(position)),
	  _rawBytes(std::move(decodeResult.rawBytes())),
	  _numBits(decodeResult.numBits()),
	  _ecLevel(TextDecoder::FromLatin1(decodeResult.ecLevel())),
	  _symbologyIdentifier(_content.symbology.toString(false)),
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

std::vector<std::pair<int,int>> Result::ECIs() const
{
    std::vector<std::pair<int,int>> ret;
    for (int i = 0, cnt = Size(_content.encodings); i < cnt; i++) {
        if (_content.encodings[i].isECI)
            ret.push_back(std::pair(ToInt(_content.encodings[i].eci), _content.encodings[i].pos));
    }
    return ret;
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

Result MergeStructuredAppendSequence(const Results& results)
{
	if (results.empty())
		return Result(DecodeStatus::NotFound);

	std::list<Result> allResults(results.begin(), results.end());
	allResults.sort([](const Result& r1, const Result& r2) { return r1.sequenceIndex() < r2.sequenceIndex(); });

	if (allResults.back().sequenceSize() != Size(allResults) ||
		!std::all_of(allResults.begin(), allResults.end(),
					 [&](Result& it) { return it.sequenceId() == allResults.front().sequenceId(); }))
		return Result(DecodeStatus::FormatError);

	Result res = allResults.front();
	for (auto i = std::next(allResults.begin()); i != allResults.end(); ++i)
		res._content.append(i->_content);

	res._text = res._content.text();
	res._position = {};
	res._sai.index = -1;

	return res;
}

Results MergeStructuredAppendSequences(const Results& results)
{
	std::map<std::string, Results> sas;
	for (auto& res : results) {
		if (res.isPartOfSequence())
			sas[res.sequenceId()].push_back(res);
	}

	Results saiResults;
	for (auto& [id, seq] : sas) {
		auto res = MergeStructuredAppendSequence(seq);
		if (res.isValid())
			saiResults.push_back(std::move(res));
	}

	return saiResults;
}

} // ZXing
