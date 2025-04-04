/*
* Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ByteArray.h"
#include "CharacterSet.h"
#include "Diagnostics.h"
#include "ECI.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#if __has_include(<span>) // c++20
#include <span>
#endif
#include <string>
#include <string_view>
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
		int modVal = (modifier >= 'A' ? modifier - 'A' + 10 : modifier - '0') + eciModifierOffset * hasECI;
		return code > '0' ? ']' + std::string(1, code) + static_cast<char>((modVal >= 10 ? 'A' - 10 : '0') + modVal) : std::string();
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
	Content(ByteArray&& bytes, SymbologyIdentifier si, CharacterSet _defaultCharSet = CharacterSet::Unknown,
			ECI eci = ECI::Unknown);

	void switchEncoding(ECI eci) { switchEncoding(eci, true); }
	void switchEncoding(CharacterSet cs);

	void reserve(int count) { bytes.reserve(bytes.size() + count); }

	void push_back(uint8_t val) { bytes.push_back(val); Diagnostics::chr(val); }
	void push_back(int val) { bytes.push_back(narrow_cast<uint8_t>(val)); Diagnostics::chr(narrow_cast<uint8_t>(val)); }
	void append(std::string_view str) { bytes.insert(bytes.end(), str.begin(), str.end()); Diagnostics::put(std::string(str)); }
#ifdef __cpp_lib_span
	void append(std::span<const uint8_t> ba) { bytes.insert(bytes.end(), ba.begin(), ba.end()); Diagnostics::put(ba); }
#else
	void append(const ByteArray& ba) { bytes.insert(bytes.end(), ba.begin(), ba.end()); Diagnostics::put(ba); }
#endif
	void append(const Content& other);

	void erase(int pos, int n);
	void insert(int pos, std::string_view str);

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
