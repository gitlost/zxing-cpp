/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "MultiFormatReader.h"

#include "Barcode.h"
#include "BarcodeFormat.h"
#include "BinaryBitmap.h"
#include "Diagnostics.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"
#include "aztec/AZReader.h"
#include "codablockf/CBFReader.h"
#include "code16k/C16KReader.h"
#include "datamatrix/DMReader.h"
#include "dotcode/DCReader.h"
#include "hanxin/HXReader.h"
#include "maxicode/MCReader.h"
#include "oned/ODReader.h"
#include "pdf417/MicroPDFReader.h"
#include "pdf417/PDFReader.h"
#include "qrcode/QRReader.h"

#include <memory>

namespace ZXing {

MultiFormatReader::MultiFormatReader(const ReaderOptions& opts) : _opts(opts)
{
	auto formats = opts.formats().empty() ? BarcodeFormat::Any : opts.formats();

	// Put linear readers upfront in "normal" mode
	if (formats.testFlags(BarcodeFormat::LinearCodes) && !opts.tryHarder())
		_readers.emplace_back(new OneD::Reader(opts));

	if (formats.testFlags(BarcodeFormat::QRCode | BarcodeFormat::MicroQRCode | BarcodeFormat::RMQRCode))
		_readers.emplace_back(new QRCode::Reader(opts, true));
	if (formats.testFlag(BarcodeFormat::DataMatrix))
		_readers.emplace_back(new DataMatrix::Reader(opts, true));
	if (formats.testFlag(BarcodeFormat::Aztec))
		_readers.emplace_back(new Aztec::Reader(opts, true));
	if (formats.testFlag(BarcodeFormat::PDF417))
		_readers.emplace_back(new Pdf417::Reader(opts));
	if (formats.testFlag(BarcodeFormat::MaxiCode))
		_readers.emplace_back(new MaxiCode::Reader(opts));
	#if 1
	if (formats.testFlag(BarcodeFormat::CodablockF))
		_readers.emplace_back(new CodablockF::Reader(opts));
	if (formats.testFlag(BarcodeFormat::Code16K))
		_readers.emplace_back(new Code16K::Reader(opts));
	if (formats.testFlag(BarcodeFormat::DotCode))
		_readers.emplace_back(new DotCode::Reader(opts));
	if (formats.testFlag(BarcodeFormat::HanXin))
		_readers.emplace_back(new HanXin::Reader(opts));
	if (formats.testFlag(BarcodeFormat::MicroPDF417))
		_readers.emplace_back(new MicroPdf417::Reader(opts));
	#endif

	// At end in "try harder" mode
	if (formats.testFlags(BarcodeFormat::LinearCodes) && opts.tryHarder())
		_readers.emplace_back(new OneD::Reader(opts));

	Diagnostics::setEnabled(opts.enableDiagnostics());
}

MultiFormatReader::~MultiFormatReader() = default;

Barcode MultiFormatReader::read(const BinaryBitmap& image) const
{
	Barcode r;
	for (const auto& reader : _readers) {
		r = reader->decode(image);
		if (r.isValid()) {
#ifdef ZXING_EXPERIMENTAL_API
			r.symbol(std::move(image.getBitMatrix()->copy()));
#endif
			return r;
		}
	}
	return _opts.returnErrors() ? r : Barcode();
}

Barcodes MultiFormatReader::readMultiple(const BinaryBitmap& image, int maxSymbols) const
{
	Barcodes res;

	for (const auto& reader : _readers) {
		Diagnostics::begin();
		if (image.inverted() && !reader->supportsInversion)
			continue;
		auto r = reader->decode(image, maxSymbols);
		if (!_opts.returnErrors()) {
#ifdef __cpp_lib_erase_if
			std::erase_if(r, [](auto&& s) { return !s.isValid(); });
#else
			auto it = std::remove_if(r.begin(), r.end(), [](auto&& s) { return !s.isValid(); });
			r.erase(it, r.end());
#endif
		}
		maxSymbols -= Size(r);
		res.insert(res.end(), std::move_iterator(r.begin()), std::move_iterator(r.end()));
		if (maxSymbols <= 0)
			break;
	}

	// sort barcodes based on their position on the image
	std::sort(res.begin(), res.end(), [](const Barcode& l, const Barcode& r) {
		auto lp = l.position().topLeft();
		auto rp = r.position().topLeft();
		return lp.y < rp.y || (lp.y == rp.y && lp.x < rp.x);
	});

	return res;
}

} // ZXing
