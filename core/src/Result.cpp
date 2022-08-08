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
#include "ZXAlgorithms.h"

#include <cmath>
#include <list>
#include <map>

namespace ZXing {

Result::Result(const std::string& text, int y, int xStart, int xStop, BarcodeFormat format, SymbologyIdentifier si, Error error, bool readerInit)
	: _content({ByteArray(text)}, si),
	  _error(error),
	  _position(Line(y, xStart, xStop)),
	  _format(format),
	  _lineCount(0),
	  _readerInit(readerInit)
{
	if (Diagnostics::enabled()) {
		Diagnostics::moveTo(_diagnostics);
	}
}

Result::Result(DecoderResult&& decodeResult, Position&& position, BarcodeFormat format)
	: _content(std::move(decodeResult).content()),
	  _error(std::move(decodeResult).error()),
	  _position(std::move(position)),
	  _ecLevel(decodeResult.ecLevel()),
	  _sai(decodeResult.structuredAppend()),
	  _format(format),
	  _lineCount(decodeResult.lineCount()),
	  _isMirrored(decodeResult.isMirrored()),
	  _readerInit(decodeResult.readerInit())
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

bool Result::isValid() const
{
	return format() != BarcodeFormat::None && _content.symbology.code != 0 && !error();
}

const ByteArray& Result::bytes() const
{
	return _content.bytes;
}

ByteArray Result::bytesECI() const
{
	return _content.bytesECI();
}

std::string Result::text(TextMode mode) const
{
	return _content.text(mode);
}

std::string Result::utf8() const
{
	return _content.utf8();
}

std::wstring Result::utfW() const
{
	return _content.utfW();
}

ContentType Result::contentType() const
{
	return _content.type();
}

bool Result::hasECI() const
{
	return _content.hasECI;
}

int Result::orientation() const
{
	constexpr auto std_numbers_pi_v = 3.14159265358979323846; // TODO: c++20 <numbers>
	return narrow_cast<int>(std::lround(_position.orientation() * 180 / std_numbers_pi_v));
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

std::string Result::symbologyIdentifier() const
{
	return _content.symbology.toString();
}

int Result::sequenceSize() const
{
	return _sai.count;
}

int Result::sequenceIndex() const
{
	return _sai.index;
}

std::string Result::sequenceId() const
{
	return _sai.id;
}

Result& Result::setDecodeHints(DecodeHints hints)
{
	if (hints.characterSet() != CharacterSet::Unknown)
		_content.hintedCharset = hints.characterSet();
	_decodeHints = hints;
	return *this;
}

const std::list<std::string>& Result::diagnostics() const
{
	return _diagnostics;
}

void Result::setContentDiagnostics()
{
	_content.p_diagnostics = &_diagnostics;
}

bool Result::operator==(const Result& o) const
{
	// two symbols may be considered the same if at least one of them has an error
	if (!(format() == o.format() && (bytes() == o.bytes() || error() || o.error())))
		return false;

	if (BarcodeFormats(BarcodeFormat::MatrixCodes).testFlag(format()))
		return IsInside(Center(o.position()), position());

	// linear symbology comparisons only implemented for this->lineCount == 1
	//assert(lineCount() == 1 || o.lineCount() == 1);

	// if one line is less than half the length of the other away from the
	// latter, we consider it to belong to the same symbol
	auto dTop = maxAbsComponent(o.position().topLeft() - position().topLeft());
	int dBot, length;
	if (lineCount() <= o.lineCount()) {
		dBot = maxAbsComponent(o.position().bottomLeft() - position().topLeft());
		length = maxAbsComponent(position().topLeft() - position().bottomRight());
	} else {
		dBot = maxAbsComponent(position().bottomLeft() - o.position().topLeft());
		length = maxAbsComponent(o.position().topLeft() - o.position().bottomRight());
	}

	return std::min(dTop, dBot) < length / 2;
}

Result MergeStructuredAppendSequence(const Results& results)
{
	if (results.empty())
		return {};

	std::list<Result> allResults(results.begin(), results.end());
	allResults.sort([](const Result& r1, const Result& r2) { return r1.sequenceIndex() < r2.sequenceIndex(); });

	Result res = allResults.front();
	for (auto i = std::next(allResults.begin()); i != allResults.end(); ++i) {
		res._content.append(i->_content);
		if (!res._diagnostics.empty()) {
			res._diagnostics.push_back("\n");
		}
		res._diagnostics.insert(res._diagnostics.end(), i->_diagnostics.begin(), i->_diagnostics.end());
	}

	res._position = {};
	res._sai.index = -1;

	if (allResults.back().sequenceSize() != Size(allResults) ||
		!std::all_of(allResults.begin(), allResults.end(),
					 [&](Result& it) { return it.sequenceId() == allResults.front().sequenceId(); }))
		res._error = FormatError("sequenceIDs not matching during structured append sequence merging");

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
