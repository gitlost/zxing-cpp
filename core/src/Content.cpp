/*
 * Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "Content.h"

#include "CharacterSetECI.h"
#include "Diagnostics.h"
#include "TextDecoder.h"
#include "TextUtfEncoding.h"
#include "ZXContainerAlgorithms.h"

namespace ZXing {

template <typename FUNC>
void Content::ForEachECIBlock(FUNC func) const
{
	for (int i = 0; i < Size(encodings); ++i) {
		auto [eci, start] = encodings[i];
		int end = i + 1 == Size(encodings) ? Size(binary) : encodings[i + 1].pos;

		func(eci, start, end);
	}
}

void Content::switchEncoding(ECI eci, bool isECI)
{
	// TODO: replace non-ECI entries on first ECI entry with default ECI
	if (isECI || !hasECI) {
		if (encodings.back().pos == Size(binary))
			encodings.back().eci = eci; // no point in recording 0 length segments
		else
			encodings.push_back({eci, Size(binary)});
		Diagnostics::fmt("%s(%d)", isECI ? "ECI" : "NonECI", ToInt(eci));
	}
	hasECI |= isECI;
}

void Content::switchEncoding(CharacterSet cs)
{
	 switchEncoding(ToECI(cs), false);
}

std::wstring Content::text() const
{
	auto fallbackCS = CharacterSetECI::CharsetFromName(hintedCharset.c_str());
	if (!hasECI && fallbackCS == CharacterSet::Unknown) {
		if (defaultCharset != CharacterSet::Unknown) {
			fallbackCS = defaultCharset;
			Diagnostics::fmt("DefEnc(%d,%d)", (int)fallbackCS, ToInt(ToECI(fallbackCS)));
		} else {
			fallbackCS = guessEncoding();
			Diagnostics::fmt("GuessEnc(%d,%d)", (int)fallbackCS, ToInt(ToECI(fallbackCS)));
		}
	}

	std::wstring wstr;
	ForEachECIBlock([&](ECI eci, int begin, int end) {
		CharacterSet cs = ToCharacterSet(eci);
		if (cs == CharacterSet::Unknown)
			cs = ToInt(eci) > 899 ? CharacterSet::BINARY : fallbackCS;

		TextDecoder::Append(wstr, binary.data() + begin, end - begin, cs);
	});
	return wstr;
}

std::string Content::utf8Protocol() const
{
	std::wstring res;
	ECI lastECI = ECI::Unknown;

	ForEachECIBlock([&](ECI eci, int begin, int end) {
		if (!hasECI)
			eci = ToECI(guessEncoding());
		CharacterSet cs = ToCharacterSet(eci);
		if (cs == CharacterSet::Unknown)
			cs = CharacterSet::BINARY;
		if (eci == ECI::Unknown)
			eci = ECI::Binary;
		else if (IsText(eci))
			eci = ECI::UTF8;

		if (lastECI != eci)
			TextDecoder::AppendLatin1(res, ToString(eci));
		lastECI = eci;

		std::wstring tmp;
		TextDecoder::Append(tmp, binary.data() + begin, end - begin, cs);
		for (auto c : tmp) {
			res += c;
			if (c == L'\\') // in the ECI protocol a '\' has to be doubled
				res += c;
		}
	});

	return TextUtfEncoding::ToUtf8(res);
}

CharacterSet Content::guessEncoding() const
{
	// assemble all blocks with unknown encoding
	ByteArray input;
	ForEachECIBlock([&](ECI eci, int begin, int end) {
		if (eci == ECI::Unknown)
			input.insert(input.end(), binary.begin() + begin, binary.begin() + end);
	});

	if (input.empty())
		return CharacterSet::Unknown;

	return TextDecoder::GuessEncoding(input.data(), input.size(), CharacterSet::BINARY);
}

ContentType Content::type() const
{
	auto isBinary = [](Encoding e) { return !IsText(e.eci); };

	if (hasECI) {
		if (std::none_of(encodings.begin(), encodings.end(), isBinary))
			return ContentType::Text;
		if (std::all_of(encodings.begin(), encodings.end(), isBinary))
			return ContentType::Binary;
	} else {
		if (std::none_of(encodings.begin(), encodings.end(), isBinary))
			return ContentType::Text;
		auto cs = guessEncoding();
		if (cs == CharacterSet::BINARY)
			return ContentType::Binary;
	}

	return ContentType::Mixed;
}

} // namespace ZXing
