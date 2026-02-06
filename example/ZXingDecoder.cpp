/*
* Copyright 2021-2024 gitlost
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
// SPDX-License-Identifier: Apache-2.0

#include "BitMatrixIO.h"
#include "CharacterSet.h"
#ifdef ZX_DIAGNOSTICS
#include "Diagnostics.h"
#endif
#include "TextDecoder.h"
#include "ThresholdBinarizer.h"
#include "Utf.h"
#include "ZXCType.h"

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

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace ZXing;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

static const char *readerOpts[] = {
};

static const char *charsets[] = {
	"ISO-8859-1",  "ISO-8859-2",  "ISO-8859-3",  "ISO-8859-4",  "ISO-8859-5",
	"ISO-8859-6",  "ISO-8859-7",  "ISO-8859-8",  "ISO-8859-9",  "ISO-8859-10",
	"ISO-8859-11", "ISO-8859-13", "ISO-8859-14", "ISO-8859-15", "ISO-8859-16",
	"Shift_JIS",   "Cp1250",      "Cp1251",      "Cp1252",      "Cp1256",
	"UTF-16BE",    "UTF-8",       "ASCII",       "Big5",        "GB2312",
	"GB18030",     "EUC-CN",      "GBK",         "EUC-KR",      "UTF-16LE",
	"UTF-32BE",    "UTF-32LE",    "BINARY",
};

static void PrintUsage(const char* exePath)
{
	std::cout << "Usage: " << exePath << " [options] -format <FORMAT> -bits <BITSTREAM>\n"
			  << "    -format <FORMAT>     Format\n"
			  << "    -bits <BITSTREAM>    Bit dump\n"
			  << "    -width <NUMBER>      Width of bit dump (if omitted 1st LF in bitstream)\n"
			  << "    -textonly            Return bare text only\n"
			  << "    -escape              Escape non-graphical characters in angle brackets\n"
#ifdef ZX_DIAGNOSTICS
			  << "    -diagnostics         Print diagnostics\n"
#endif
			  << "    -opts <OPT[,OPT]>    Reader options\n"
			  << "    -charset <CHARSET>   Default character set\n"
			  << "Supported formats (case insensitive, with or without '-'):\n  ";
	for (auto f : BarcodeFormats::list(BarcodeFormat::AllCreatable)) {
		std::cout << "  " << ToString(f);
	}
	std::cout << "\nSupported reader options (-opts) (case insensitive, comma-separated):\n  ";
	for (int i = 0; i < ARRAY_SIZE(readerOpts); i++) {
		std::cout << "  " << readerOpts[i];
	}
	std::cout << "\nSupported character sets (-charset) (case insensitive):\n  ";
	for (int i = 0; i < ARRAY_SIZE(charsets); i++) {
		if (i && i % 12 == 0) std::cout << "\n  ";
		std::cout << "  " << std::setw(11) << charsets[i];
	}
	std::cout << "\n";
}

static int validateInt(const char* optarg, int length, const bool neg, int &val)
{
	int start = 0;
	bool haveNeg = false;
	int i;

	if (length == -1) {
		length = static_cast<int>(strlen(optarg));
	}

	val = 0;

	if (neg && optarg[0] == '-') {
		haveNeg = true;
		length--;
		start = 1;
	}
	if (length > 9) { /* Prevent overflow */
		return 0;
	}
	for (i = start; i < length; i++) {
		if (optarg[i] < '0' || optarg[i] > '9') {
			return 0;
		}
		val *= 10;
		val += optarg[i] - '0';
	}
	if (haveNeg) {
		val = -val;
	}

	return 1;
}

static BitMatrix ParseBitMatrix(const std::string& str, const int width, int &height)
{
	height = static_cast<int>(str.length() / width);

	BitMatrix mat(width, height);
	for (int y = 0; y < height; ++y) {
		size_t offset = y * width;
		for (int x = 0; x < width; ++x, offset++) {
			if (str[offset] == '1') {
				mat.set(x, y);
			}
		}
	}
	return mat;
}

static ImageView getImageView(std::vector<uint8_t> &buf, const BitMatrix &bits)
{
	buf.resize(bits.width() * bits.height());
	for (int r = 0; r < bits.height(); r++) {
		const int k = r * bits.width();
		for (int c = 0; c < bits.width(); c++) {
			buf[k + c] = bits.get(c, r) ? 0x00 : 0xFF;
		}
	}
	return ImageView(buf.data(), bits.width(), bits.height(), ImageFormat::Lum);
}

static bool ParseOptions(int argc, char* argv[], ReaderOptions &opts, std::string &bitstream, int &width,
			bool &textOnly, bool& angleEscape)
{
	bool haveFormat = false, haveBits = false, haveWidth = false, haveCharSet = false;
	int nlWidth = -1;

	for (int i = 1; i < argc; ++i) {
		auto is = [&](const char* str) { return strlen(argv[i]) > 1 && strncmp(argv[i], str, strlen(argv[i])) == 0; };
		if (is("-format")) {
			if (haveFormat) {
				std::cerr << "Single -format only\n";
				return false;
			}
			haveFormat = true;
			if (++i == argc) {
				std::cerr << "No argument for -format\n";
				return false;
			}
			try {
				opts.setFormats(BarcodeFormatsFromString(argv[i]));
			} catch (const std::exception& e) {
				std::cerr << e.what() << "\n";
				return false;
			}
			if (ToString(opts.formats()).find('|') != std::string::npos) { /* Single format only */
				std::cerr << "Invalid argument for -format: " << ToString(opts.formats()) << " (single format only)\n";
				return false;
			}

		} else if (is("-bits")) {
			if (haveBits) {
				std::cerr << "Single -bits only\n";
				return false;
			}
			haveBits = true;
			// Use stdin if no arg
			std::string in;
			if (i + 1 == argc) {
				getline(std::cin, in);
			} else {
				in = argv[++i];
			}
			for (int j = 0, len = static_cast<int>(in.size()); j < len; j++) {
				if (in[j] == '\n') {
					if (nlWidth == -1) {
						nlWidth = static_cast<int>(bitstream.size());
					}
				} else if (in[j] == '1' || in[j] == '0') {
					bitstream.push_back(in[j]);
				}
			}

		} else if (is("-width")) {
			if (haveWidth) {
				std::cerr << "Single -width only\n";
				return false;
			}
			haveWidth = true;
			if (++i == argc) {
				std::cerr << "No argument for -width\n";
				return false;
			}
			if (!validateInt(argv[i], -1, false, width)) {
				std::cerr << "Invalid argument for -width (digits only)\n";
				return false;
			}
			if (width == 0) {
				std::cerr << "Invalid argument for -width (zero)\n";
				return false;
			}

		} else if (is("-opts")) {
			if (++i == argc) {
				std::cerr << "No argument for -opts\n";
				return false;
			}
			std::string opt(argv[i]);
			std::transform(opt.begin(), opt.end(), opt.begin(), [](char c) { return (char)std::tolower(c); });
			std::istringstream input(opt);
			for (std::string token; std::getline(input, token, ',');) {
				if (!token.empty()) {
					std::cerr << "Unknown opts '" << token << "'\n";
					return false;
				}
			}

		} else if (is("-charset")) {
			if (haveCharSet) {
				std::cerr << "Single -charset only\n";
				return false;
			}
			haveCharSet = true;
			if (++i == argc) {
				std::cerr << "No argument for -charset\n";
				return false;
			}
			CharacterSet cs;
			std::string argvi(argv[i]);
			if (std::find_if_not(argvi.begin(), argvi.end(), zx_isdigit) == argvi.end()) { // Allow numeric ECI
				cs = ToCharacterSet(ECI(std::stoi((argvi))));
			} else {
				cs = CharacterSetFromString(argvi);
			}
			if (cs == CharacterSet::Unknown) {
				std::cerr << "Unknown character set '" << argv[i] << "'\n";
				return false;
			}
			opts.setCharacterSet(cs);
		} else if (is("-textonly")) {
			textOnly = true;
		} else if (is("-diagnostics")) {
#ifdef ZX_DIAGNOSTICS
			opts.setEnableDiagnostics(true);
#else
			std::cerr << "Warning: ignoring '-diagnostics' option, BUILD_DIAGNOSTICS not enabled\n";
#endif
		} else if (is("-escape")) {
			angleEscape = true;
		} else {
			std::cerr << "Unknown option '" << argv[i] << "'\n";
			return false;
		}
	}

	if (!haveFormat && !haveBits) {
		std::cerr << "Missing required options -format and -bits\n";
	} else if (!haveFormat) {
		std::cerr << "Missing required option -format\n";
	} else if (!haveBits) {
		std::cerr << "Missing required option -bits\n";
	}
	if (!haveWidth && nlWidth != -1) {
		width = nlWidth;
	}
	return haveFormat && haveBits;
}

std::ostream& operator<<(std::ostream& os, const Position& points) {
	for (const auto& p : points)
		os << p.x << "," << p.y << " ";
	return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<std::pair<int,int>>& ecis) {
	bool not_first = false;
	for (const auto& e : ecis) {
		if (not_first)
			os << " ";
		os << std::get<0>(e) << "," << std::get<1>(e);
		not_first = true;
	}
	return os;
}

std::string appendBinIfTextEmpty(const Barcode& barcode)
{
	std::string text = barcode.text(TextMode::Plain);
	if (text.empty() && !barcode.bytes().empty()) {
		text = BytesToUtf8(barcode.bytes(), CharacterSet::BINARY);
	}
	return text;
}

int main(int argc, char* argv[])
{
	ReaderOptions opts;
	std::string bitstream;
	int width = 0, height = 0;
	bool textOnly = false;
	BitMatrix bits;
	std::vector<uint8_t> buf;
	bool angleEscape = false;
	int ret = 0;
	BarcodesData data;
#ifdef ZX_DIAGNOSTICS
	std::list<std::string> diagnostics;
#endif

	if (!ParseOptions(argc, argv, opts, bitstream, width, textOnly, angleEscape)) {
		PrintUsage(argv[0]);
		return argc == 1 ? 0 : -1;
	}

#ifdef ZX_DIAGNOSTICS
	Diagnostics::setEnabled(opts.enableDiagnostics());
#endif

	assert(opts.formats().size() == 1);
	auto format = *(opts.formats().data());

	if (width == 0) {
		width = static_cast<int>(bitstream.length());
		if (format & BarcodeFormat::AllMatrix) { 
			const int w = static_cast<int>(std::sqrt(width));
			if (w * w == width) {
				width = w;
			}
		}
	}
	if (width > 1 && bitstream.length() % width) {
		std::cerr << "Invalid bitstream - width " << width << " not multiple of length " << bitstream.length() << "\n";
		std::cerr << bitstream << "\n";
		PrintUsage(argv[0]);
		return -1;
	}

	bits = ParseBitMatrix(bitstream, width, height);
	if (bits.width() == 0 || bits.height() == 0) {
		std::cerr << "Failed to parse bitstream\n";
		PrintUsage(argv[0]);
		return -1;
	}

#ifdef ZX_DIAGNOSTICS
	Diagnostics::begin();
#endif

	opts.setIsPure(true);

	if (format == BarcodeFormat::Aztec) {
		Aztec::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);

	} else if (format == BarcodeFormat::CodablockF) {
		CodablockF::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);

	} else if (format == BarcodeFormat::Code16K) {
		Code16K::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);

	} else if (format == BarcodeFormat::DataMatrix) {
		DataMatrix::Reader reader(opts);
		auto ivbits = InflateXY(bits.copy(), bits.width() * 2, bits.height() * 2);
		//fprintf(stderr, "ivbits width %d, height %d\n", ivbits.width(), ivbits.height());
		data = reader.read(ThresholdBinarizer(getImageView(buf, ivbits), 127), 1);

	} else if (format == BarcodeFormat::HanXin) {
		HanXin::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);

	} else if (format == BarcodeFormat::DotCode) {
		DotCode::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);

	} else if (format == BarcodeFormat::MaxiCode) {
		MaxiCode::Reader reader(opts);
		BitMatrix ivbits(bits.width() * 2, bits.height()); // Shift odd rows 1 right
		for (int r = 0, offset = 0; r < bits.height(); r++, offset = 1 - offset) {
			for (int c = 0; c < bits.width(); c++) {
				ivbits.set(c * 2 + offset, r, bits.get(c, r));
			}
		}
		data = reader.read(ThresholdBinarizer(getImageView(buf, ivbits), 127), 1);

	} else if (format == BarcodeFormat::PDF417) {
		Pdf417::Reader reader(opts);
		int height = bits.height() * 3;
		if (height >= 1) {
			while (height < 10) { // BARCODE_MIN_HEIGHT in "pdf417/PDFDetector.cpp"
				height *= 2;
			}
		}
		auto ivbits = InflateXY(bits.copy(), bits.width(), height);
		data = reader.read(ThresholdBinarizer(getImageView(buf, ivbits), 127), 1);

	} else if (format == BarcodeFormat::MicroPDF417) {
		MicroPdf417::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);

	} else if (format == BarcodeFormat::DXFilmEdge) {
		OneD::Reader reader(opts);
		auto ivbits = InflateXY(bits.copy(), bits.width() * 6, bits.height() * 6, 7, 0);
		data = reader.read(ThresholdBinarizer(getImageView(buf, ivbits), 127), 1);

	} else if (format & BarcodeFormat::AllLinear) {
		opts.setEanAddOnSymbol(EanAddOnSymbol::Read);
		OneD::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);

	} else if (format & (BarcodeFormat::QRCode | BarcodeFormat::MicroQRCode | BarcodeFormat::RMQRCode)) {
		QRCode::Reader reader(opts);
		data = reader.read(ThresholdBinarizer(getImageView(buf, bits), 127), 1);
	}

	if (data.empty()) {
		std::cerr << "Failed to decode\n";
		return -1;
	}

	Barcode result(std::move(data.front()));

	if (!result.isValid()) {
		ret = 1;
	}

	if (textOnly) {
		if (ret == 0) {
			std::string text = appendBinIfTextEmpty(result);
			std::cout << (angleEscape ? EscapeNonGraphical(text) : text);
		}
		return ret;
	}

#ifdef ZX_DIAGNOSTICS
	if (opts.enableDiagnostics()) {
		result.setContentDiagnostics();
	}
#endif

	std::string text = appendBinIfTextEmpty(result);
	std::cout << "Text:       \"" << (angleEscape ? EscapeNonGraphical(text) : text) << "\"\n";
	std::cout << "Bytes:      " << ToHex(result.bytes()) << "\n";
	std::cout << "Length:     " << text.size() << "\n";

	if (Size(result.ECIs()))
		std::cout << "ECIs:       (" << Size(result.ECIs()) << ") " << result.ECIs() << "\n";

	std::cout << "Format:     " << ToString(result.format()) << "\n"
			  << "Identifier: " << result.symbologyIdentifier() << "\n"
			  << "Position:   " << result.position() << "\n";

	auto printOptional = [](const char* key, const std::string& v) {
		if (!v.empty())
			std::cout << key << v << "\n";
	};

	printOptional("Error:      ", ToString(result.error()));
	printOptional("EC Level:   ", result.ecLevel());
#ifdef ZXING_EXPERIMENTAL_API
	printOptional("Extra:      ", result.extra());
#endif

	if (result.isPartOfSequence()) {
		std::cout << "Structured Append\n";
		if (result.sequenceSize() > 0)
			std::cout << "    Sequence: " << result.sequenceIndex() + 1 << " of " << result.sequenceSize() << "\n";
		else
			std::cout << "    Sequence: " << result.sequenceIndex() + 1 << " of unknown number\n";
		if (!result.sequenceId().empty())
			std::cout << "    Id:       \"" << result.sequenceId() << "\"\n";
	}

	if (result.readerInit())
		std::cout << "Reader Initialisation/Programming\n";

#ifdef ZX_DIAGNOSTICS
	if (opts.enableDiagnostics()) {
		std::cout << "Diagnostics" << Diagnostics::print(&result.diagnostics());
	}
#endif

	return ret;
}
