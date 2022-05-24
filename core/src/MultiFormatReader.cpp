/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "MultiFormatReader.h"

#include "BarcodeFormat.h"
#include "DecodeHints.h"
#include "Diagnostics.h"
#include "Result.h"
#include "aztec/AZReader.h"
#include "datamatrix/DMReader.h"
#include "dotcode/DCReader.h"
#include "hanxin/HXReader.h"
#include "maxicode/MCReader.h"
#include "oned/ODReader.h"
#include "pdf417/PDFReader.h"
#include "qrcode/QRReader.h"

#include <memory>

namespace ZXing {

MultiFormatReader::MultiFormatReader(const DecodeHints& hints)
{
	bool tryHarder = hints.tryHarder();
	auto formats = hints.formats().empty() ? BarcodeFormat::Any : hints.formats();

	// Put 1D readers upfront in "normal" mode
	if (formats.testFlags(BarcodeFormat::OneDCodes) && !tryHarder)
		_readers.emplace_back(new OneD::Reader(hints));

	if (formats.testFlags(BarcodeFormat::QRCode | BarcodeFormat::MicroQRCode))
		_readers.emplace_back(new QRCode::Reader(hints));
	if (formats.testFlag(BarcodeFormat::DataMatrix))
		_readers.emplace_back(new DataMatrix::Reader(hints));
	if (formats.testFlag(BarcodeFormat::Aztec))
		_readers.emplace_back(new Aztec::Reader(hints));
	if (formats.testFlag(BarcodeFormat::PDF417))
		_readers.emplace_back(new Pdf417::Reader(hints));
	if (formats.testFlag(BarcodeFormat::MaxiCode))
		_readers.emplace_back(new MaxiCode::Reader(hints));
	if (formats.testFlag(BarcodeFormat::DotCode))
		_readers.emplace_back(new DotCode::Reader(hints));
	if (formats.testFlag(BarcodeFormat::HanXin))
		_readers.emplace_back(new HanXin::Reader(hints));

	// At end in "try harder" mode
	if (formats.testFlags(BarcodeFormat::OneDCodes) && tryHarder) {
		_readers.emplace_back(new OneD::Reader(hints));
	}

	Diagnostics::setEnabled(hints.enableDiagnostics());
}

MultiFormatReader::~MultiFormatReader() = default;

Result
MultiFormatReader::read(const BinaryBitmap& image) const
{
	// If we have only one reader in our list, just return whatever that decoded.
	// This preserves information (e.g. ChecksumError) instead of just returning 'NotFound'.
	if (_readers.size() == 1)
		return _readers.front()->decode(image);

	for (const auto& reader : _readers) {
		Result r = reader->decode(image);
		if (r.isValid())
			return r;
	}
	return Result(DecodeStatus::NotFound);
}

Results MultiFormatReader::readMultiple(const BinaryBitmap& image, int maxSymbols) const
{
	std::vector<Result> res;

	for (const auto& reader : _readers) {
		auto r = reader->decode(image, maxSymbols);
		maxSymbols -= r.size();
		res.insert(res.end(), std::move_iterator(r.begin()), std::move_iterator(r.end()));
		if (maxSymbols <= 0)
			break;
	}

	// sort results based on their position on the image
	std::sort(res.begin(), res.end(), [](const Result& l, const Result& r) {
		auto lp = l.position().topLeft();
		auto rp = r.position().topLeft();
		return lp.y < rp.y || (lp.y == rp.y && lp.x < rp.x);
	});

	return res;
}

} // ZXing
