/*
* Copyright 2021-2022 gitlost
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

#define ZX_USE_UTF8 1 // see Result.h

#include "BitMatrixIO.h"
#include "CharacterSet.h"
#ifdef ZX_DIAGNOSTICS
#include "Diagnostics.h"
#endif
#include "ReadBarcode.h"
#include "TextDecoder.h"
#include "TextUtfEncoding.h"
#include "ThresholdBinarizer.h"
#include "aztec/AZReader.h"
#include "datamatrix/DMReader.h"
#include "dotcode/DCReader.h"
#include "hanxin/HXReader.h"
#include "maxicode/MCReader.h"
#include "oned/ODReader.h"
#include "pdf417/PDFReader.h"
#include "pdf417/PDFDecoderResultExtra.h"
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
using namespace TextUtfEncoding;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

static const char *hints[] = {
	"tryCode39ExtendedMode", "validateCode39CheckSum", "validateITFCheckSum", "returnCodabarStartEnd"
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
#ifdef ZX_DIAGNOSTICS
			  << "    -diagnostics         Print diagnostics\n"
#endif
			  << "    -hint <HINT[,HINT]>  Hints\n"
			  << "    -charset <CHARSET>   Default character set\n"
			  << "Supported formats (case insensitive, with or without '-'):\n  ";
	for (auto f : BarcodeFormats::all()) {
		std::cout << "  " << ToString(f);
	}
	std::cout << "\nSupported hints (case insensitive, comma-separated):\n  ";
	for (int i = 0; i < ARRAY_SIZE(hints); i++) {
		std::cout << "  " << hints[i];
	}
	std::cout << "\nSupported character sets (case insensitive):\n  ";
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

static bool ParseOptions(int argc, char* argv[], DecodeHints &hints, std::string &bitstream, int &width,
			bool &textOnly, bool& angleEscape)
{
	bool haveFormat = false, haveBits = false, haveWidth = false, haveCharSet = false;
	int nlWidth = -1;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-format") == 0) {
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
				hints.setFormats(BarcodeFormatsFromString(argv[i]));
			} catch (const std::exception& e) {
				std::cerr << e.what() << "\n";
				return false;
			}
			if (ToString(hints.formats()).find('|') != std::string::npos) { /* Single format only */
				std::cerr << "Invalid argument for -format: " << ToString(hints.formats()) << " (single format only)\n";
				return false;
			}

		} else if (strcmp(argv[i], "-bits") == 0) {
			if (haveBits) {
				std::cerr << "Single -bits only\n";
				return false;
			}
			haveBits = true;
			if (++i == argc) {
				std::cerr << "No argument for -bits\n";
				return false;
			}
			for (int j = 0, len = static_cast<int>(strlen(argv[i])); j < len; j++) {
				if (argv[i][j] == '\n') {
					if (nlWidth == -1) {
						nlWidth = static_cast<int>(bitstream.size());
					}
				} else if (argv[i][j] == '1' || argv[i][j] == '0') {
					bitstream.push_back(argv[i][j]);
				}
			}

		} else if (strcmp(argv[i], "-width") == 0) {
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

		} else if (strcmp(argv[i], "-hint") == 0) {
			if (++i == argc) {
				std::cerr << "No argument for -hint\n";
				return false;
			}
			std::string hint(argv[i]);
			std::transform(hint.begin(), hint.end(), hint.begin(), [](char c) { return (char)std::tolower(c); });
			std::istringstream input(hint);
			for (std::string token; std::getline(input, token, ',');) {
				if (!token.empty()) {
					if (token == "trycode39extendedmode" || token == "code39extendedmode") {
						hints.setTryCode39ExtendedMode(true);
					} else if (token == "validatecode39checksum" || token == "code39checksum") {
						hints.setValidateCode39CheckSum(true);
					} else if (token == "validateitfchecksum" || token == "itfchecksum") {
						hints.setValidateITFCheckSum(true);
					} else if (token == "returncodabarstartend" || token == "codabarstartend") {
						hints.setReturnCodabarStartEnd(true);
					} else {
						std::cerr << "Unknown hint '" << token << "'\n";
						return false;
					}
				}
			}

		} else if (strcmp(argv[i], "-charset") == 0) {
			if (haveCharSet) {
				std::cerr << "Single -charset only\n";
				return false;
			}
			haveCharSet = true;
			if (++i == argc) {
				std::cerr << "No argument for -charset\n";
				return false;
			}
			if (CharacterSetFromString(argv[i]) == CharacterSet::Unknown) {
				std::cerr << "Unknown character set '" << argv[i] << "'\n";
				return false;
			}
			hints.setCharacterSet(argv[i]);
		} else if (strcmp(argv[i], "-textonly") == 0) {
			textOnly = true;
		} else if (strcmp(argv[i], "-diagnostics") == 0) {
#ifdef ZX_DIAGNOSTICS
			hints.setEnableDiagnostics(true);
#else
			std::cerr << "Warning: ignoring '-diagnostics' option, BUILD_DIAGNOSTICS not enabled\n";
#endif
		} else if (strcmp(argv[i], "-escape") == 0) {
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

int main(int argc, char* argv[])
{
	DecodeHints hints;
	std::string bitstream;
	int width = 0, height = 0;
	bool textOnly = false;
	BitMatrix bits;
	std::vector<uint8_t> buf;
	bool angleEscape = false;
	int ret = 0;
	Result result;
#ifdef ZX_DIAGNOSTICS
	std::list<std::string> diagnostics;
#endif

	if (!ParseOptions(argc, argv, hints, bitstream, width, textOnly, angleEscape)) {
		PrintUsage(argv[0]);
		return argc == 1 ? 0 : -1;
	}

#ifdef ZX_DIAGNOSTICS
	Diagnostics::setEnabled(hints.enableDiagnostics());
#endif

	if (width == 0) {
		width = static_cast<int>(bitstream.length());
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

	hints.setIsPure(true);

	if (hints.formats() == BarcodeFormat::Aztec) {
		Aztec::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));

	} else if (hints.formats() == BarcodeFormat::DataMatrix) {
		DataMatrix::Reader reader(hints);
		auto ivbits = InflateXY(bits.copy(), bits.width() * 2, bits.height() * 2);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, ivbits), 127));

	} else if (hints.formats() == BarcodeFormat::HanXin) {
		HanXin::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));

	} else if (hints.formats() == BarcodeFormat::DotCode) {
		DotCode::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));

	} else if (hints.formats() == BarcodeFormat::MaxiCode) {
		MaxiCode::Reader reader(hints);
		BitMatrix ivbits(bits.width() * 2, bits.height()); // Shift odd rows 1 right
		for (int r = 0, offset = 0; r < bits.height(); r++, offset = 1 - offset) {
			for (int c = 0; c < bits.width(); c++) {
				ivbits.set(c * 2 + offset, r, bits.get(c, r));
			}
		}
		result = reader.decode(ThresholdBinarizer(getImageView(buf, ivbits), 127));

	} else if (hints.formats() == BarcodeFormat::PDF417) {
		Pdf417::Reader reader(hints);
		int height = bits.height() * 3;
		if (height >= 1) {
			while (height < 10) { // BARCODE_MIN_HEIGHT in "pdf417/PDFDetector.cpp"
				height *= 2;
			}
		}
		auto ivbits = InflateXY(bits.copy(), bits.width(), height);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, ivbits), 127));

	} else if (hints.formats().testFlags(BarcodeFormat::LinearCodes)) {
		hints.setEanAddOnSymbol(EanAddOnSymbol::Read);
		OneD::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));

	} else if (hints.formats() == BarcodeFormat::QRCode) {
		QRCode::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));

	} else if (hints.formats() == BarcodeFormat::MicroQRCode) {
		QRCode::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));
	}

	if (hints.characterSet() == CharacterSet::Unknown) {
		result.setCharacterSet(hints.characterSet());
	}

	if (!result.isValid()) {
		ret = 1;
	}

	if (textOnly) {
		if (ret == 0) {
			std::string text = result.text();
			if (text.empty() && !result.bytes().empty()) {
				TextDecoder::Append(text, result.bytes().data(), result.bytes().size(), CharacterSet::BINARY);
			}
			std::cout << (angleEscape ? AngleEscape(text) : text);
		}
		return ret;
	}

#ifdef ZX_DIAGNOSTICS
	if (hints.enableDiagnostics()) {
		result.setContentDiagnostics();
	}
#endif

	std::cout << "Text:       \"" << (angleEscape ? AngleEscape(result.text()) : result.text()) << "\"\n"
			  << "Bytes:      (" << Size(result.bytes()) << ") " << ToHex(result.bytes()) << "\n";

	if (Size(result.ECIs()))
		std::cout << "ECIs:       (" << Size(result.ECIs()) << ") " << result.ECIs() << "\n";
	else
		std::cout << "ECIs:       None\n";

	std::cout << "Format:     " << ToString(result.format()) << "\n"
			  << "Identifier: " << result.symbologyIdentifier() << "\n"
			  << "Position:   " << result.position() << "\n"
			  << "Error:      " << ToString(result.error()) << "\n";

	auto printOptional = [](const char* key, const std::string& v) {
		if (!v.empty())
			std::cout << key << v << "\n";
	};

	printOptional("EC Level:   ", result.ecLevel());

	if (result.isPartOfSequence()) {
		std::cout << "Structured Append\n";
		if (result.sequenceSize() > 0)
			std::cout << "    Sequence: " << result.sequenceIndex() + 1 << " of " << result.sequenceSize() << "\n";
		else
			std::cout << "    Sequence: " << result.sequenceIndex() + 1 << " of unknown number\n";
		if (!result.sequenceId().empty())
			std::cout << "    Id:       \"" << result.sequenceId() << "\"\n";
	}

	const auto& meta = result.metadata();
	if (meta.getCustomData(ResultMetadata::PDF417_EXTRA_METADATA)) {
		const auto& extra = std::dynamic_pointer_cast<Pdf417::DecoderResultExtra>(meta.getCustomData(ResultMetadata::PDF417_EXTRA_METADATA));
		if (!extra->empty()) {
			std::cout << "PDF417 Macro";
			if (!extra->fileName().empty())
				std::cout << "\n  File Name:  " << extra->fileName();
			if (extra->timestamp() != -1)
				std::cout << "\n  Time Stamp: " << extra->timestamp();
			if (!extra->sender().empty())
				std::cout << "\n  Sender:     " << extra->sender();
			if (!extra->addressee().empty())
				std::cout << "\n  Addressee:  " << extra->addressee();
			if (extra->fileSize() != -1)
				std::cout << "\n  File Size:  " << extra->fileSize();
			if (extra->checksum() != -1)
				std::cout << "\n  Checksum:   " << extra->checksum();
			std::cout << "\n";
		}
	}

	if (result.readerInit())
		std::cout << "Reader Initialisation/Programming\n";

#ifdef ZX_DIAGNOSTICS
	if (hints.enableDiagnostics()) {
		std::cout << "Diagnostics" << Diagnostics::print(&result.diagnostics());
	}
#endif

	return ret;
}
