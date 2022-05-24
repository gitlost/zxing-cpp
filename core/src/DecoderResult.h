/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ByteArray.h"
#include "Content.h"
#include "DecodeStatus.h"
#include "StructuredAppend.h"
#include "ZXContainerAlgorithms.h"

#include <memory>
#include <string>
#include <utility>

namespace ZXing {

class CustomData;

/**
* <p>Encapsulates the result of decoding a matrix of bits. This typically
* applies to 2D barcode formats. For now it contains the raw bytes obtained,
* as well as a string interpretation of those bytes, if applicable.</p>
*
* @author Sean Owen
*/
class DecoderResult
{
	DecodeStatus _status = DecodeStatus::NoError;
	ByteArray _rawBytes;
	Content _content;
	int _numBits = 0;
	std::wstring _text;
	std::wstring _ecLevel;
	int _errorsCorrected = -1;
	int _erasures = -1;
	int _lineCount = -1;
	std::string _symbologyIdentifier;
	StructuredAppendInfo _structuredAppend;
	bool _isMirrored = false;
	bool _readerInit = false;
	std::shared_ptr<CustomData> _extra;

	DecoderResult(const DecoderResult &) = delete;
	DecoderResult& operator=(const DecoderResult &) = delete;

public:
	DecoderResult(DecodeStatus status) : _status(status) {}
	DecoderResult(ByteArray&& rawBytes, std::wstring&& text, Content&& binary = {})
		: _rawBytes(std::move(rawBytes)), _content(std::move(binary)), _text(std::move(text))
	{
		_numBits = 8 * Size(_rawBytes);
		if (_text.empty())
			_text = _content.text();
		// provide some best guess fallback for barcodes not, yet supporting the content info
		if (_content.empty() && std::all_of(_text.begin(), _text.end(), [](auto c) { return c < 256; }))
			std::for_each(_text.begin(), _text.end(), [this](wchar_t c) { _content += static_cast<uint8_t>(c); });
	}

	DecoderResult() = default;
	DecoderResult(DecoderResult&&) noexcept = default;
	DecoderResult& operator=(DecoderResult&&) = default;

	bool isValid() const { return StatusIsOK(_status); }
	DecodeStatus errorCode() const { return _status; }

	const ByteArray& rawBytes() const & { return _rawBytes; }
	ByteArray&& rawBytes() && { return std::move(_rawBytes); }
	const std::wstring& text() const & { return _text; }
	std::wstring&& text() && { return std::move(_text); }
	const ByteArray& binary() const & { return _content.binary; }
	ByteArray&& binary() && { return std::move(_content.binary); }

	// Simple macro to set up getter/setter methods that save lots of boilerplate.
	// It sets up a standard 'const & () const', 2 setters for setting lvalues via
	// copy and 2 for setting rvalues via move. They are provided each to work
	// either on lvalues (normal 'void (...)') or on rvalues (returning '*this' as
	// rvalue). The latter can be used to optionally initialize a temporary in a
	// return statement, e.g.
	//    return DecoderResult(bytes, text).setEcLevel(level);
#define ZX_PROPERTY(TYPE, GETTER, SETTER) \
	const TYPE& GETTER() const & { return _##GETTER; } \
	TYPE&& GETTER() && { return std::move(_##GETTER); } \
	void SETTER(const TYPE& v) & { _##GETTER = v; } \
	void SETTER(TYPE&& v) & { _##GETTER = std::move(v); } \
	DecoderResult&& SETTER(const TYPE& v) && { _##GETTER = v; return std::move(*this); } \
	DecoderResult&& SETTER(TYPE&& v) && { _##GETTER = std::move(v); return std::move(*this); }

	ZX_PROPERTY(int, numBits, setNumBits)
	ZX_PROPERTY(std::wstring, ecLevel, setEcLevel)
	ZX_PROPERTY(int, errorsCorrected, setErrorsCorrected)
	ZX_PROPERTY(int, erasures, setErasures)
	ZX_PROPERTY(int, lineCount, setLineCount)
	ZX_PROPERTY(std::string, symbologyIdentifier, setSymbologyIdentifier)
	ZX_PROPERTY(StructuredAppendInfo, structuredAppend, setStructuredAppend)
	ZX_PROPERTY(bool, isMirrored, setIsMirrored)
	ZX_PROPERTY(bool, readerInit, setReaderInit)
	ZX_PROPERTY(std::shared_ptr<CustomData>, extra, setExtra)

#undef ZX_PROPERTY
};

} // ZXing
