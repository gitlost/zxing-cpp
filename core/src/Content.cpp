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

#include "Content.h"

#include "CharacterSetECI.h"
#include "Diagnostics.h"
#include "TextDecoder.h"
#include "TextUtfEncoding.h"

#include <tuple>
#include <utility>

namespace ZXing {

static const char *segmentTypeNames[] = { "ASC", "BYT", "KAN", "HAN" };

Content::Content(BarcodeFormat barcodeFormat, const std::string& characterSetName, CharacterSet encodingDefault)
	: _barcodeFormat(barcodeFormat), _isGS1(false), _haveKanjiSegment(false), _haveHanziSegment(false),
	  _isFinalized(false)
{
	_defaultCharacterSet = CharacterSetECI::InitEncoding(characterSetName, encodingDefault);
}

void Content::SetSegmentType(const SegmentType segmentType)
{
	_segmentTypes.push_back(std::make_tuple(segmentType, _byteStream.size(), 0));
	if (segmentType == SegmentType::KANJI) {
		_haveKanjiSegment = true;
	} else if (segmentType == SegmentType::HANZI) {
		_haveHanziSegment = true;
	}
	Diagnostics::fmt("SEG(%s)", segmentTypeNames[(int)segmentType]);
}

bool Content::SetECI(const int eci)
{
	_ecis.push_back(std::make_tuple(eci, _byteStream.size(), 0));
	if (eci >= 0 && eci <= 899 && CharacterSetECI::CharsetFromValue(eci) == CharacterSet::Unknown) {
		return false;
	}
	return true;
}

void Content::Append(const uint8_t* bytes, size_t length)
{
	Diagnostics::fmt("%.*s", length, bytes);
	while (length--) {
		_byteStream.push_back(*bytes++);
	}
}

void Content::Append(const uint8_t byte)
{
	Diagnostics::chr(byte);
	_byteStream.push_back(byte);
}

void Content::Prepend(const uint8_t* bytes, size_t length)
{
	if (!length) {
		return;
	}
	// Yes this is horrendous
	for (size_t i = 0; i < _segmentTypes.size(); i++) {
		std::get<1>(_segmentTypes[i]) += length;
	}
	for (size_t i = 0; i < _ecis.size(); i++) {
		std::get<1>(_ecis[i]) += length;
	}
	// This even more so
	for (size_t i = length - 1; i != 0; i--) {
		_byteStream.insert(_byteStream.begin(), bytes[i]);
	}
	_byteStream.insert(_byteStream.begin(), bytes[0]);
}

bool Content::Finalize()
{
	if (_isFinalized) {
		return false;
	}

	if (_segmentTypes.size()) {
		// Calculate lengths
		size_t idx = 0;
		for (size_t i = 1; i < _segmentTypes.size(); i++) {
			std::get<2>(_segmentTypes[idx]) = i - idx;
			idx = i;
		}
		std::get<2>(_segmentTypes[idx]) = _segmentTypes.size() - idx;
	}

	if (_ecis.size()) {
		// Calculate lengths
		size_t idx = 0;
		for (size_t i = 1; i < _ecis.size(); i++) {
			std::get<2>(_ecis[idx]) = i - idx;
			idx = i;
		}
		std::get<2>(_ecis[idx]) = _ecis.size() - idx;
	}

	_isFinalized = true;

	return true;
}

std::wstring Content::getResultText() const
{
	assert(_isFinalized);

	std::wstring result;

	if (_isGS1) {
		TextDecoder::Append(result, _byteStream.data(), _byteStream.size(), CharacterSet::ISO8859_1);
    	Diagnostics::put("Content(ISO8859_1,GS1)");
		return result;
	}

	if (_ecis.size()) {
		size_t i = 0;
		for (; i < _ecis.size() && CharacterSetECI::CharsetFromValue(std::get<0>(_ecis[i])) == CharacterSet::Unknown; i++);
		if (i != _ecis.size()) {
			size_t idx = 0;
			CharacterSet encoding;
			if (std::get<1>(_ecis[i]) != 0) { // If ECI not at start of stream
				encoding = _defaultCharacterSet == CharacterSet::Unknown ? CharacterSet::ISO8859_1 : _defaultCharacterSet;
				i = 0;
			} else {
				encoding = CharacterSetECI::CharsetFromValue(std::get<0>(_ecis[i]));
				i = 1;
			}
			for (; i < _ecis.size(); i++) {
				CharacterSet encodingNew = CharacterSetECI::CharsetFromValue(std::get<0>(_ecis[i]));
				if (encodingNew == CharacterSet::Unknown) {
					continue;
				}
				if (encodingNew != encoding) {
					const size_t idxNew = std::get<1>(_ecis[i]);
					TextDecoder::Append(result, _byteStream.data() + idx, idxNew - idx, encoding);
					encoding = encodingNew;
					idx = idxNew;
				}
			}
			TextDecoder::Append(result, _byteStream.data() + idx, _byteStream.size() - idx, encoding);
			Diagnostics::put("Content(ECIs)");
			return result;
		}
	}

	// TODO: total hack for now
	if (_barcodeFormat == BarcodeFormat::QRCode) {
		if (_haveHanziSegment && _haveKanjiSegment) {
            // TODO: decode each segment separately
        }
		if (_haveHanziSegment && !_haveKanjiSegment) {
			TextDecoder::Append(result, _byteStream.data(), _byteStream.size(), CharacterSet::GB2312);
			Diagnostics::put("Content(HANZI)");
			return result;
		}
		if (!_haveKanjiSegment && !_haveHanziSegment && _defaultCharacterSet == CharacterSet::ISO8859_1) {
			TextDecoder::Append(result, _byteStream.data(), _byteStream.size(), _defaultCharacterSet);
			Diagnostics::put("Content(ISO8859_1)");
			return result;
		}
	}

	CharacterSet fallback = _defaultCharacterSet;
	if (_barcodeFormat == BarcodeFormat::QRCode) {
		if (_haveHanziSegment) {
			fallback = CharacterSet::GB2312;
		} else {
			fallback = CharacterSet::Shift_JIS;
		}
	}
	CharacterSet guessCharacterSet = TextDecoder::GuessEncoding(_byteStream.data(), _byteStream.size(), fallback);

	Diagnostics::fmt("Content(Guess:%d)", (int)guessCharacterSet);
	TextDecoder::Append(result, _byteStream.data(), _byteStream.size(), guessCharacterSet);
	return result;
}

std::string Content::getGS1HRT() const
{
	assert(_isFinalized);

	std::string result;

    // TODO
    return result;
}

} // ZXing
