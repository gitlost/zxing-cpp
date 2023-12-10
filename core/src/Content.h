/*
* Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ByteArray.h"
#include "CharacterSet.h"
#include "Diagnostics.h"
#include "ReaderOptions.h"

#include <string>
#include <vector>

namespace ZXing {

enum class ECI : int;

enum class ContentType { Text, Binary, Mixed, GS1, ISO15434, UnknownECI };
enum class AIFlag : char { None, GS1, AIM };

std::string ToString(ContentType type);

struct SymbologyIdentifier
{
	char code = 0, modifier = 0, eciModifierOffset = 0;
	AIFlag aiFlag = AIFlag::None;

	std::string toString(bool hasECI = false) const
	{
		return code > '0' ? ']' + std::string(1, code) + static_cast<char>(modifier + eciModifierOffset * hasECI) : std::string();
	}
};

class Content
{
	template <typename FUNC>
	void ForEachECIBlock(FUNC f) const;

	void switchEncoding(ECI eci, bool isECI);
	std::string render(bool withECI) const;

public:
	struct Encoding
	{
		ECI eci;
		int pos;
		bool isECI;
	};

	ByteArray bytes;
	std::vector<Encoding> encodings;
	SymbologyIdentifier symbology;
	CharacterSet defaultCharset = CharacterSet::Unknown;
	CharacterSet optionsCharset = CharacterSet::Unknown;
	bool hasECI = false;
	std::list<std::string>* p_diagnostics = nullptr;

	Content();
	Content(ByteArray&& bytes, SymbologyIdentifier si, CharacterSet _defaultCharSet = CharacterSet::Unknown);

	void switchEncoding(ECI eci) { switchEncoding(eci, true); }
	void switchEncoding(CharacterSet cs);

	void reserve(int count) { bytes.reserve(bytes.size() + count); }

	void push_back(uint8_t val) { bytes.push_back(val); Diagnostics::chr(val); }
	void append(const std::string& str) { bytes.insert(bytes.end(), str.begin(), str.end()); Diagnostics::put(str); }
	void append(const ByteArray& ba) { bytes.insert(bytes.end(), ba.begin(), ba.end()); Diagnostics::put(ba); }
	void append(const Content& other);

	void operator+=(char val) { push_back(val); }
	void operator+=(const std::string& str) { append(str); }

	void erase(int pos, int n);
	void insert(int pos, const std::string& str);

	bool empty() const { return bytes.empty(); }
	bool canProcess() const;

	std::string text(TextMode mode) const;
	std::wstring utfW() const; // utf16 or utf32 depending on the platform, i.e. on size_of(wchar_t)
	std::string utf8() const { return render(false); }

	ByteArray bytesECI() const;
	CharacterSet guessEncoding() const;
	ContentType type() const;
};

} // ZXing
