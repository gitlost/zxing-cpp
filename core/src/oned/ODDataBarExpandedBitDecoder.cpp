/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
* Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODDataBarExpandedBitDecoder.h"

#include "BitArray.h"
#include "Diagnostics.h"
#include "Error.h"
#include "GTIN.h"

#include <cmath>

namespace ZXing::OneD::DataBar {

constexpr char GS = 29; // FNC1

static std::string DecodeGeneralPurposeBits(BitArrayView& bits)
{
	Diagnostics::put(__func__);
	enum State { NUMERIC, ALPHA, ISO_IEC_646 };
	State state = NUMERIC;
	std::string res;

	auto decode5Bits = [](State& state, std::string& res, BitArrayView& bits) {
		int v = bits.readBits(5);
		if (v == 4) {
			state = state == ALPHA ? ISO_IEC_646 : ALPHA;
			Diagnostics::put(state == ALPHA ? "ALPHA" : "ISO646");
		} else if (v == 15) { // FNC1 + latch to Numeric
			res.push_back(GS);
			state = NUMERIC;
			Diagnostics::put("<GS> NUM");
			// Allow for some generators incorrectly placing a numeric latch "000" after an FNC1
			if (bits.size() >= 7 && bits.peakBits(7) < 8) {
				bits.skipBits(3);
				Diagnostics::put("Skip(3)");
			}
		} else {
			res.push_back(v + 43);
			Diagnostics::chr(res.back());
		}
	};

	auto isPadding = [](State state, BitArrayView& bits) {
		bool res = (state == NUMERIC) ? bits.size() < 4
									  : bits.size() < 5 && (0b00100 >> (5 - bits.size()) == bits.peakBits(bits.size()));
		if (res) {
			bits.skipBits(bits.size());
			Diagnostics::fmt("Skip(%d)", bits.size());
		}
		return res;
	};

	while(bits.size() >= 3) {
		switch(state) {
		case NUMERIC:
			if (isPadding(state, bits))
				break;
			if (bits.size() < 7) {
				int v = bits.readBits(4);
				if (v > 0) {
					res.push_back(ToDigit(v - 1));
					Diagnostics::chr(res.back());
				}
			} else if (bits.peakBits(4) == 0) {
				bits.skipBits(4);
				state = ALPHA;
				Diagnostics::put("Skip(4) ALPHA");
			} else {
				int v = bits.readBits(7);
				for (int digit : {(v - 8) / 11, (v - 8) % 11}) {
					res.push_back(digit == 10 ? GS : ToDigit(digit));
					Diagnostics::chr(res.back());
				}
			}
			break;
		case ALPHA:
			if (isPadding(state, bits))
				break;
			if (bits.peakBits(1) == 1) {
				constexpr char const* lut58to62 = R"(*,-./)";
				int v = bits.readBits(6);
				if (v < 58)
					res.push_back(v + 33);
				else if (v < 63)
					res.push_back(lut58to62[v - 58]);
				else
					throw FormatError();
				Diagnostics::chr(res.back());
			} else if (bits.peakBits(3) == 0) {
				bits.skipBits(3);
				state = NUMERIC;
				Diagnostics::put("Skip(3) NUM");
			} else {
				decode5Bits(state, res, bits);
			}
			break;
		case ISO_IEC_646:
			if (isPadding(state, bits))
				break;
			if (bits.peakBits(3) == 0) {
				bits.skipBits(3);
				state = NUMERIC;
				Diagnostics::put("Skip(3) NUM");
			} else {
				int v = bits.peakBits(5);
				if (v < 16) {
					decode5Bits(state, res, bits);
				} else if (v < 29) {
					v = bits.readBits(7);
					res.push_back(v < 90 ? v + 1 : v + 7);
					Diagnostics::chr(res.back());
				} else {
					constexpr char const* lut232to252 = R"(!"%&'()*+,-./:;<=>?_ )";
					v = bits.readBits(8);
					if (v < 232 || 252 < v)
						throw FormatError();
					res.push_back(lut232to252[v - 232]);
					Diagnostics::chr(res.back());
				}
			}
			break;
		}
	}

	// in NUMERIC encodation there might be a trailing FNC1 that needs to be ignored
	if (res.size() && res.back() == GS)
		res.pop_back();

	return res;
}

static std::string DecodeCompressedGTIN(std::string prefix, BitArrayView& bits)
{
	Diagnostics::put(__func__);
	for (int i = 0; i < 4; ++i)
		prefix.append(ToString(bits.readBits(10), 3));

	prefix.push_back(GTIN::ComputeCheckDigit(prefix.substr(2)));

	return prefix;
}

static std::string DecodeAI01GTIN(BitArrayView& bits)
{
	Diagnostics::put(__func__);
	return DecodeCompressedGTIN("019", bits);
}

static std::string DecodeAI01AndOtherAIs(BitArrayView& bits)
{
	Diagnostics::put(__func__);
	bits.skipBits(2); // Variable length symbol bit field

	auto header = DecodeCompressedGTIN("01" + std::to_string(bits.readBits(4)), bits);
	auto trailer = DecodeGeneralPurposeBits(bits);

	return header + trailer;
}

static std::string DecodeAnyAI(BitArrayView& bits)
{
	Diagnostics::put(__func__);
	bits.skipBits(2); // Variable length symbol bit field

	return DecodeGeneralPurposeBits(bits);
}

static std::string DecodeAI013103(BitArrayView& bits)
{
	Diagnostics::put(__func__);
	std::string buffer = DecodeAI01GTIN(bits);
	buffer.append("3103");
	buffer.append(ToString(bits.readBits(15), 6));

	return buffer;
}

static std::string DecodeAI01320x(BitArrayView& bits)
{
	Diagnostics::put(__func__);
	std::string buffer = DecodeAI01GTIN(bits);
	int weight = bits.readBits(15);
	buffer.append(weight < 10000 ? "3202" : "3203");
	buffer.append(ToString(weight < 10000 ? weight : weight - 10000, 6));

	return buffer;
}

static std::string DecodeAI0139yx(BitArrayView& bits, char y)
{
	Diagnostics::fmt("%s(%c)", __func__, y);
	bits.skipBits(2); // Variable length symbol bit field

	std::string buffer = DecodeAI01GTIN(bits);
	buffer.append("39");
	buffer.push_back(y);
	buffer.append(std::to_string(bits.readBits(2)));

	if (y == '3')
		buffer.append(ToString(bits.readBits(10), 3));

	auto trailer = DecodeGeneralPurposeBits(bits);
	if (trailer.empty())
		return {};

	return buffer + trailer;
}

static std::string DecodeAI013x0x1x(BitArrayView& bits, const char* aiPrefix, const char* dateCode)
{
	Diagnostics::fmt("%s(%s,%s)", __func__, aiPrefix, dateCode);
	std::string buffer = DecodeAI01GTIN(bits);
	buffer.append(aiPrefix);

	int weight = bits.readBits(20);
	buffer.append(std::to_string(weight / 100000));
	buffer.append(ToString(weight % 100000, 6));

	int date = bits.readBits(16);
	if (date != 38400) {
		buffer.append(dateCode);

		int day = date % 32;
		date /= 32;
		int month = date % 12 + 1;
		date /= 12;
		int year = date;

		buffer.append(ToString(year, 2));
		buffer.append(ToString(month, 2));
		buffer.append(ToString(day, 2));
	}

	return buffer;
}

std::string DecodeExpandedBits(const BitArray& _bits)
{
	Diagnostics::put("  Decode:");
	auto bits = BitArrayView(_bits);
	bits.readBits(1); // skip linkage bit

	if (bits.peakBits(1) == 1)
		return DecodeAI01AndOtherAIs(bits.skipBits(1));

	if (bits.peakBits(2) == 0)
		return DecodeAnyAI(bits.skipBits(2));

	switch (bits.peakBits(4)) {
	case 4: return DecodeAI013103(bits.skipBits(4));
	case 5: return DecodeAI01320x(bits.skipBits(4));
	}

	switch (bits.peakBits(5)) {
	case 12: return DecodeAI0139yx(bits.skipBits(5), '2');
	case 13: return DecodeAI0139yx(bits.skipBits(5), '3');
	}

	switch (bits.readBits(7)) {
	case 56: return DecodeAI013x0x1x(bits, "310", "11");
	case 57: return DecodeAI013x0x1x(bits, "320", "11");
	case 58: return DecodeAI013x0x1x(bits, "310", "13");
	case 59: return DecodeAI013x0x1x(bits, "320", "13");
	case 60: return DecodeAI013x0x1x(bits, "310", "15");
	case 61: return DecodeAI013x0x1x(bits, "320", "15");
	case 62: return DecodeAI013x0x1x(bits, "310", "17");
	case 63: return DecodeAI013x0x1x(bits, "320", "17");
	}

	return {};
}

} // namespace ZXing::OneD::DataBar
