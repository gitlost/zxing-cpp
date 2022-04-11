/*
* Copyright 2021 gitlost
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

#include "BitMatrixIO.h"
#include "Diagnostics.h"
#include "ReadBarcode.h"
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

#include <cctype>
#include <chrono>
#include <clocale>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace ZXing;
using namespace TextUtfEncoding;

static void PrintUsage(const char* exePath)
{
	std::cout << "Usage: " << exePath << " [-width <NUMBER>] [-textonly] [-diagnostics] -format <FORMAT> -bits <BITSTREAM>\n"
			  << "    -width        Width of bit dump (if omitted 1st LF in bits)\n"
			  << "    -textonly     Return bare text only\n"
			  << "    -diagnostics  Print diagnostics\n"
			  << "    -format       Format\n"
			  << "    -charset      Default character set\n"
			  << "    -zint         Zint hints\n"
			  << "    -bits         Bit dump\n"
			  << "\n"
			  << "Supported formats are:\n";
	for (auto f : BarcodeFormats::all()) {
		std::cout << "    " << ToString(f) << "\n";
	}
	std::cout << "Format can be lowercase, with or without '-'\n";
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

static int validateZint(const char* optarg, int &zintBarcode, int &zintOption2)
{
	int length = static_cast<int>(strlen(optarg));
	const char *comma, *arg;

	zintBarcode = 0;
	zintOption2 = 0;

	arg = optarg;
	comma = strchr(arg, ',');
	if (!comma || comma == arg || !validateInt(arg, static_cast<int>(comma - arg), false, zintBarcode)) {
		return 0;
	}

	arg = comma + 1;
	if (arg == optarg + length || !validateInt(arg, static_cast<int>(optarg - arg + length), false, zintOption2)) {
		return 0;
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
	buf.reserve(bits.width() * bits.height());
	for (int r = 0; r < bits.height(); r++) {
		const int k = r * bits.width();
		for (int c = 0; c < bits.width(); c++) {
			buf[k + c] = bits.get(c, r) ? 0x00 : 0xFF;
		}
	}
	return ImageView(buf.data(), bits.width(), bits.height(), ImageFormat::Lum);
}

static bool ParseOptions(int argc, char* argv[], DecodeHints &hints, std::string &bitstream, int &width,
			bool &textOnly, int &zintBarcode, int &zintOption2)
{
	bool haveFormat = false, haveBits = false, haveWidth = false;
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

		} else if (strcmp(argv[i], "-zint") == 0) {
			if (++i == argc) {
				std::cerr << "No argument for -format\n";
				return false;
			}
			if (!validateZint(argv[i], zintBarcode, zintOption2)) {
				std::cerr << "Invalid argument for -zint (\"<symbology>,<option_1>,<option_2>,<option_3>\")\n";
				return false;
			}

		} else if (strcmp(argv[i], "-charset") == 0) {
			if (++i == argc) {
				std::cerr << "No argument for -charset\n";
				return false;
			}
			hints.setCharacterSet(argv[i]);
		} else if (strcmp(argv[i], "-textonly") == 0) {
			textOnly = true;
		} else if (strcmp(argv[i], "-diagnostics") == 0) {
			hints.setEnableDiagnostics(true);
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

int main(int argc, char* argv[])
{
	DecodeHints hints;
	std::string bitstream;
	int width = 0, height = 0;
	bool textOnly = false;
	BitMatrix bits;
	std::vector<uint8_t> buf;
	bool angleEscape = false;
	int zintBarcode = 0;
	int zintOption2 = 0;
	int ret = 0;
	Result result(DecodeStatus::NotFound);
	std::list<std::string> diagnostics;

	if (!ParseOptions(argc, argv, hints, bitstream, width, textOnly, zintBarcode, zintOption2)) {
		PrintUsage(argv[0]);
		return argc == 1 ? 0 : -1;
	}

	Diagnostics::setEnabled(hints.enableDiagnostics());

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

	} else if (hints.formats().testFlags(BarcodeFormat::OneDCodes)) {
		hints.setEanAddOnSymbol(EanAddOnSymbol::Read);
		// BARCODE_EXCODE39
		if (zintBarcode == 9) {
			hints.setTryCode39ExtendedMode(true);
		}
		// BARCODE_CODE39, BARCODE_EXCODE39, BARCODE_LOGMARS
		if ((zintBarcode == 8 || zintBarcode == 9 || zintBarcode == 50) && zintOption2 == 1) {
			hints.setAssumeCode39CheckDigit(true);
		}
		OneD::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));

	} else if (hints.formats() == BarcodeFormat::QRCode) {
		QRCode::Reader reader(hints);
		result = reader.decode(ThresholdBinarizer(getImageView(buf, bits), 127));
	}

	if (!result.isValid()) {
		ret = 1;
	}

	if (textOnly) {
		if (ret == 0) {
			std::cout << ToUtf8(result.text(), angleEscape);
		}
		return ret;
	}

	std::cout << "Text:       \"" << ToUtf8(result.text(), angleEscape) << "\"\n"
			  << "Format:     " << ToString(result.format()) << "\n"
			  << "Identifier: " << result.symbologyIdentifier() << "\n"
			  << "Position:   " << result.position() << "\n"
			  << "Error:      " << ToString(result.status()) << "\n";

	auto printOptional = [](const char* key, const std::string& v) {
		if (!v.empty())
			std::cout << key << v << "\n";
	};

	printOptional("EC Level:   ", ToUtf8(result.ecLevel()));

	if (result.isPartOfSequence()) {
		std::cout << "Structured Append\n";
		if (result.sequenceSize() > 0)
			std::cout << "    Sequence: " << result.sequenceIndex() + 1 << " of " << result.sequenceSize() << "\n";
		else
			std::cout << "    Sequence: " << result.sequenceIndex() + 1 << " of unknown number\n";
		if (!result.sequenceId().empty())
			std::cout << "    Id:       \"" << result.sequenceId() << "\"\n";
		if (result.sequenceLastECI() > -1)
			std::cout << "    Last ECI: " << result.sequenceLastECI() << "\n";
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

	if (hints.enableDiagnostics()) {
		std::cout << "Diagnostics";
		const auto& diagnostics = result.diagnostics();
		if (diagnostics.empty()) {
			std::cout << " (empty)\n";
		} else {
			std::cout << "\n";
			for (std::string value : diagnostics) {
				std::cout << value;
				if (!std::isspace(value.back())) {
					std::cout << " ";
				}
			}
			std::cout << "\n";
		}
	}

	return ret;
}
