#pragma once
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

#include "BarcodeFormat.h"
#include "ByteArray.h"
#include "CharacterSet.h"

#include <string>
#include <utility>

namespace ZXing {

enum class SegmentType
{
	ASCII, BYTE, KANJI, HANZI
};

class Content
{
public:
	Content(BarcodeFormat barcodeFormat = BarcodeFormat::None, const std::string& characterSetName = "",
			CharacterSet encodingDefault = CharacterSet::ISO8859_1);

	void SetGS1() { _isGS1 = true; }
	void SetSegmentType(const SegmentType segmentType);
	bool SetECI(const int eci);

	void Append(const uint8_t* bytes, size_t length);
	void Append(const uint8_t byte);
	void Prepend(const uint8_t* bytes, size_t length);

	bool Finalize();

	std::wstring getResultText() const;

	std::string getGS1HRT() const;

	const ByteArray& getByteStream() const { return _byteStream; }

	const std::vector<std::tuple<SegmentType, size_t, size_t>>& getSegmentTypes() const {
		return _segmentTypes;
	}

	const std::vector<std::tuple<int, size_t, size_t>>& getECIs() const { return _ecis; }

    bool haveECIs() const { return _ecis.size() != 0; }

	BarcodeFormat barcodeFormat() const { return _barcodeFormat; }
	CharacterSet defaultCharacterSet() const { return _defaultCharacterSet; }

	bool isGS1() const { return _isGS1; }

	bool haveKanjiSegment() const { return _haveKanjiSegment; }
	bool haveHanziSegment() const { return _haveHanziSegment; }

	bool isFinalized() const { return _isFinalized; }

private:
	BarcodeFormat _barcodeFormat;
	CharacterSet _defaultCharacterSet;
	bool _isGS1;
	bool _haveKanjiSegment;
	bool _haveHanziSegment;

	ByteArray _byteStream;
	std::vector<std::tuple<SegmentType, size_t, size_t>> _segmentTypes;
	std::vector<std::tuple<int, size_t, size_t>> _ecis;

	bool _isFinalized;
};

} // ZXing
