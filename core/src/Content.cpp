/*
 * Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "Content.h"

#include "Diagnostics.h"
#include "ECI.h"
#include "HRI.h"
#include "TextDecoder.h"
#include "Utf.h"
#include "ZXAlgorithms.h"
#include "ZXCType.h"

#if !defined(ZXING_READERS) && !defined(ZXING_WRITERS)
#include "Version.h"
#endif

namespace ZXing {

std::string ToString(ContentType type)
{
	const char* t2s[] = {"Text", "Binary", "Mixed", "GS1", "ISO15434", "UnknownECI"};
	return t2s[static_cast<int>(type)];
}

template <typename FUNC>
void Content::ForEachECIBlock(FUNC func) const
{
	ECI defaultECI = hasECI ? ECI::ISO8859_1 : ECI::Unknown;
	if (encodings.empty())
		func(defaultECI, 0, Size(bytes));
	else if (encodings.front().pos != 0)
		func(defaultECI, 0, encodings.front().pos);

	for (int i = 0; i < Size(encodings); ++i) {
		auto [eci, start, ignore] = encodings[i];
		int end = i + 1 == Size(encodings) ? Size(bytes) : encodings[i + 1].pos;

		if (start != end)
			func(eci, start, end);
	}
}

void Content::switchEncoding(ECI eci, bool isECI)
{
	// remove all non-ECI entries on first ECI entry
	if (isECI && !hasECI)
		encodings.clear();
	if (isECI || !hasECI)
		encodings.push_back({eci, Size(bytes), isECI});

	hasECI |= isECI;
}

Content::Content() {}

Content::Content(ByteArray&& bytes, SymbologyIdentifier si, CharacterSet _defaultCharSet)
	: bytes(std::move(bytes)), symbology(si), defaultCharset(_defaultCharSet) {}

Content::Content(std::string&& utf8, ByteArray&& bytes, SymbologyIdentifier si, ECI eci)
	: utf8Cache(std::move(utf8)), bytes(std::move(bytes)), symbology(si)
{
	if (eci != ECI::Unknown) {
		encodings.push_back({eci, 0, true});
		hasECI = true;
	}
}

void Content::switchEncoding(CharacterSet cs)
{
	switchEncoding(ToECI(cs), false);
}

void Content::append(const Content& other)
{
	if (!hasECI && other.hasECI)
		encodings.clear();
	if (other.hasECI || !hasECI)
		for (auto& e : other.encodings)
			encodings.push_back({e.eci, Size(bytes) + e.pos, e.isECI});
	append(other.bytes);

	hasECI |= other.hasECI;
}

void Content::erase(int pos, int n)
{
	bytes.erase(bytes.begin() + pos, bytes.begin() + pos + n);
	for (auto& e : encodings)
		if (e.pos > pos)
			e.pos -= n;
}

void Content::insert(int pos, std::string_view str)
{
	bytes.insert(bytes.begin() + pos, str.begin(), str.end());
	for (auto& e : encodings)
		if (e.pos > pos)
			e.pos += Size(str);
}

bool Content::canProcess() const
{
	return std::all_of(encodings.begin(), encodings.end(), [](Encoding e) { return CanProcess(e.eci); });
}

std::string Content::render(bool withECI) const
{
	if (empty() || !canProcess())
		return {};

	CharacterSet fallbackCS = optionsCharset;
	if (!hasECI && fallbackCS == CharacterSet::Unknown) {
		if (defaultCharset != CharacterSet::Unknown) {
			fallbackCS = defaultCharset;
			if (p_diagnostics) {
				Diagnostics::fmt(p_diagnostics, "DefEnc(%d)", ToInt(ToECI(fallbackCS)));
			}
		} else {
			fallbackCS = guessEncoding();
			if (p_diagnostics) {
				Diagnostics::fmt(p_diagnostics, "GuessEnc(%d)", ToInt(ToECI(fallbackCS)));
			}
		}
	} else {
		if (p_diagnostics) {
			Diagnostics::fmt(p_diagnostics, "Fallback(%d,%d)", ToInt(ToECI(fallbackCS)), hasECI ? 1 : 0);
		}
	}

#ifdef ZXING_READERS
	std::string res;
	res.reserve(bytes.size() * 2);
	if (withECI)
		res += symbology.toString(true);
	ECI lastECI = ECI::Unknown;

	ForEachECIBlock([&](ECI eci, int begin, int end) {
		// basic idea: if IsText(eci), we transcode it to UTF8, otherwise we treat it as binary but
		// transcoded it to valid UTF8 bytes seqences representing the code points 0-255. The eci we report
		// back to the caller by inserting their "\XXXXXX" ECI designator is UTF8 for text and
		// the original ECI for everything else.
		// first determine how to decode the content (use fallback if unknown)
		auto inEci = IsText(eci) ? eci : eci == ECI::Unknown ? ToECI(fallbackCS) : ECI::Binary;
		if (withECI) {
			// then find the eci to report back in the ECI designator
			auto outEci = IsText(inEci) ? ECI::UTF8 : eci;

			if (lastECI != outEci)
				res += ToString(outEci);
			lastECI = outEci;

			for (auto c : BytesToUtf8(bytes.asView(begin, end - begin), inEci)) {
				res += c;
				if (c == '\\') // in the ECI protocol a '\' (0x5c) has to be doubled, works only because 0x5c can only mean `\`
					res += c;
			}
		} else {
			res += BytesToUtf8(bytes.asView(begin, end - begin), inEci);
		}
	});

	return res;
#elif defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT)
	assert(!utf8Cache.empty());
	if (!withECI)
		return utf8Cache;

	std::string res;
	res.reserve(3 + utf8Cache.size() * 2 + encodings.size() * 7);
	res += symbology.toString(true);

	auto eci = encodings.front().eci;
	auto inEci = IsText(eci) ? eci : eci == ECI::Unknown ? ToECI(fallbackCS) : ECI::Binary;
	auto outEci = IsText(inEci) ? ECI::UTF8 : eci;
	res += ToString(outEci);

	for (auto c : utf8Cache) {
		res += c;
		if (c == '\\') // in the ECI protocol a '\' (0x5c) has to be doubled, works only because 0x5c can only mean `\`
			res += c;
	}

	return res;
#else
	(void)withECI;
	return std::string(bytes.asString());
#endif
}

std::string Content::text(TextMode mode) const
{
	switch (mode) {
	case TextMode::Plain: return render(false);
	case TextMode::ECI: return render(true);
	case TextMode::HRI:
		switch (type()) {
#if defined(ZXING_READERS) || (defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT))
		case ContentType::GS1: {
			auto plain = render(false);
			auto hri = HRIFromGS1(plain);
			return hri.empty() ? plain : hri;
		}
		case ContentType::ISO15434: return HRIFromISO15434(render(false));
		case ContentType::Text: return render(false);
#endif
		default: return text(TextMode::Escaped);
		}
	case TextMode::Hex: return ToHex(bytes);
	case TextMode::Escaped: return EscapeNonGraphical(render(false));
	}

	return {}; // silence compiler warning
}

std::wstring Content::utfW() const
{
	return FromUtf8(render(false));
}

ByteArray Content::bytesECI() const
{
	if (empty())
		return {};

	ByteArray res;
	res.reserve(3 + bytes.size() + hasECI * encodings.size() * 7);

	// report ECI protocol only if actually found ECI data in the barode bit stream
	// see also https://github.com/zxing-cpp/zxing-cpp/issues/936
	res.append(symbology.toString(hasECI));

	if (hasECI)
		ForEachECIBlock([&](ECI eci, int begin, int end) {
			if (hasECI)
				res.append(ToString(eci));

			for (auto b : bytes.asView(begin, end - begin)) {
				res.push_back(b);
				if (b == '\\') // in the ECI protocol a '\' has to be doubled
					res.push_back(b);
			}
		});
	else
		res.append(bytes);

	return res;
}

CharacterSet Content::guessEncoding() const
{
#if defined(ZXING_READERS) || (defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT))
	// assemble all blocks with unknown encoding
	ByteArray input;
	ForEachECIBlock([&](ECI eci, int begin, int end) {
		if (eci == ECI::Unknown)
			input.append(bytes.asView(begin, end - begin));
	});

	if (input.empty())
		return CharacterSet::Unknown;

	return GuessTextEncoding(input);
#else
	return CharacterSet::ISO8859_1;
#endif
}

ContentType Content::type() const
{
#if 1 //def ZXING_READERS
	if (empty())
		return ContentType::Text;

	if (!canProcess())
		return ContentType::UnknownECI;

	if (symbology.aiFlag == AIFlag::GS1)
		return ContentType::GS1;

	// check for the absolut minimum of a ISO 15434 conforming message ("[)>" + RS + digit + digit)
	if (bytes.size() > 6 && bytes.asString(0, 4) == "[)>\x1E" && zx_isdigit(bytes[4]) && zx_isdigit(bytes[5]))
		return ContentType::ISO15434;

	ECI fallback = ToECI(guessEncoding());
	std::vector<bool> binaryECIs;
	ForEachECIBlock([&](ECI eci, int begin, int end) {
		if (eci == ECI::Unknown)
			eci = fallback;
		binaryECIs.push_back((!IsText(eci)
							  || (ToInt(eci) > 0 && ToInt(eci) < 28 && ToInt(eci) != 25
								  && std::any_of(bytes.begin() + begin, bytes.begin() + end,
												 [](auto c) { return c < 0x20 && c != 0x9 && c != 0xa && c != 0xd; }))));
	});

	if (!Contains(binaryECIs, true))
		return ContentType::Text;
	if (!Contains(binaryECIs, false))
		return ContentType::Binary;

	return ContentType::Mixed;
#else
	return ContentType::Text;
#endif
}

} // namespace ZXing
