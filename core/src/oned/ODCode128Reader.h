/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

class Code128Reader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

class Code128Decoder
{
	int codeSet = 0;
	SymbologyIdentifier _symbologyIdentifier = {'C', '0'}; // ISO/IEC 15417:2007 Annex C Table C.1
	bool _readerInit = false;
	bool _prevReaderInit = false;
	std::string txt;
	size_t lastTxtSize = 0;

	bool fnc4All = false;
	bool fnc4Next = false;
	bool shift = false;

	void fnc1(const bool isCodeSetC);

public:
	Code128Decoder(int startCode);

	bool decode(int code);

	std::string text() const;

	SymbologyIdentifier symbologyIdentifier() const { return _symbologyIdentifier; }
	bool readerInit() const { return _readerInit; }
	bool prevReaderInit() const { return _prevReaderInit; }
};

} // namespace ZXing::OneD
