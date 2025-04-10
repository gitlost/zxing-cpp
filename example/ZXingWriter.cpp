/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

#ifdef ZXING_EXPERIMENTAL_API
#ifdef ZXING_USE_ZINT
#include "ECI.h"
#endif
#include "JSON.h"
#include "Utf.h"
#include "WriteBarcode.h"
#else
#include "BitMatrixIO.h"
#include "CharacterSet.h"
#include "MultiFormatWriter.h"
#endif
#include "BitMatrix.h"
#include "Version.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace ZXing;

static void PrintUsage(const char* exePath)
{
	std::cout << "Usage: " << exePath
			  << " [-size <width/height>] [-eclevel <level>] [-noqz] [-hrt] <format> <text> <output>\n"
			  << "    -size       Size of generated image\n"
			  << "    -eclevel    Error correction level, [0-8]\n"
			  << "    -binary     Interpret <text> as a file name containing binary data\n"
			  << "    -noqz       Print barcode without quiet zone\n"
			  << "    -hrt        Print human readable text below the barcode (if supported)\n"
			  << "    -verbose    Print barcode information\n"
#if defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT)
			  << "    -scale      Set X-dimension of generated image\n"
			  << "    -margin     Margin around barcode\n"
			  << "    -encoding   Encoding used to encode input text\n"
			  << "    -eci        Set ECI\n"
			  << "    -height     Set height in X-dimensions of linear symbol\n"
			  << "    -readerinit Reader Initialisation/Programming\n"
			  << "    -rotate     Rotate 0, 90, 180, 270 degrees\n"
			  << "    -debug      Print debug information\n"
#endif
			  << "    -options    Comma separated list of symbology specific options and flags\n"
			  << "    -help       Print usage information\n"
			  << "    -version    Print version information\n"
			  << "\n"
			  << "Supported formats are:\n";
#ifdef ZXING_EXPERIMENTAL_API
	for (auto f : BarcodeFormats::all())
#else
	for (auto f : BarcodeFormatsFromString("Aztec Codabar Code39 Code93 Code128 DataMatrix EAN8 EAN13 ITF PDF417 QRCode UPCA UPCE"))
#endif
		std::cout << "    " << ToString(f) << "\n";

	std::cout << "Format can be lowercase letters, with or without '-'.\n"
			  << "Output format is determined by file name, supported are png, jpg and svg.\n";
}

static bool ParseSize(std::string str, int* width, int* height)
{
	std::transform(str.begin(), str.end(), str.begin(), [](char c) { return (char)std::tolower(c); });
	auto xPos = str.find('x');
	if (xPos != std::string::npos) {
		*width  = std::stoi(str.substr(0, xPos));
		*height = std::stoi(str.substr(xPos + 1));
		return true;
	}
	return false;
}

struct CLI
{
	BarcodeFormat format;
	int sizeHint = 0;
	std::string input;
	std::string outPath;
	std::string ecLevel;
	std::string options;
	bool inputIsFile = false;
	bool withHRT = false;
	bool withQZ = true;
	bool verbose = false;
#if defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT)
	int scale = 0;
	CharacterSet encoding = CharacterSet::Unknown;
	int margin = 0;
	ECI eci = ECI::Unknown;
	float height = 0.0f;
	bool readerInit = false;
	bool debug = false;
#endif
	int rotate = 0;
};

static bool ParseOptions(int argc, char* argv[], CLI& cli)
{
	int nonOptArgCount = 0;
	for (int i = 1; i < argc; ++i) {
		auto is = [&](const char* str) { return strlen(argv[i]) >= 3 && strncmp(argv[i], str, strlen(argv[i])) == 0; };
		if (is("-size")) {
			if (++i == argc)
				return false;
			cli.sizeHint = std::stoi(argv[i]);
		} else if (is("-eclevel")) {
			if (++i == argc)
				return false;
			cli.ecLevel = argv[i];
#if defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT)
		} else if (is("-scale")) {
			if (++i == argc)
				return false;
			cli.scale = std::stoi(argv[i]);
		} else if (is("-margin")) {
			if (++i == argc)
				return false;
			cli.margin = std::stoi(argv[i]);
		} else if (is("-encoding")) {
			if (++i == argc)
				return false;
			if ((cli.encoding = CharacterSetFromString(argv[i])) == CharacterSet::Unknown) {
				std::cerr << "Unrecognized encoding: " << argv[i] << std::endl;
				return false;
			}
		} else if (is("-eci")) {
			if (++i == argc)
				return false;
			if ((cli.eci = ToECI(CharacterSetFromString(argv[i]))) == ECI::Unknown) {
				std::cerr << "Unrecognized ECI: " << argv[i] << std::endl;
				return false;
			}
		} else if (is("-height")) {
			if (++i == argc)
				return false;
			cli.height = std::stof(argv[i]);
		} else if (is("-readerinit")) {
			cli.readerInit = true;
		} else if (is("-debug")) {
			cli.debug = true;
		} else if (is("-rotate")) {
			if (++i == argc)
				return false;
			cli.rotate = std::stoi(argv[i]);
#endif
		} else if (is("-binary")) {
			cli.inputIsFile = true;
		} else if (is("-hrt")) {
			cli.withHRT = true;
		} else if (is("-noqz")) {
			cli.withQZ = false;
		} else if (is("-options")) {
			if (++i == argc)
				return false;
			cli.options = argv[i];
		} else if (is("-verbose")) {
			cli.verbose = true;
		} else if (is("-help") || is("--help")) {
			PrintUsage(argv[0]);
			exit(0);
		} else if (is("-version") || is("--version")) {
			std::cout << "ZXingWriter " << ZXING_VERSION_STR << "\n";
			exit(0);
		} else if (nonOptArgCount == 0) {
			cli.format = BarcodeFormatFromString(argv[i]);
			if (cli.format == BarcodeFormat::None) {
				std::cerr << "Unrecognized format: " << argv[i] << std::endl;
				return false;
			}
			++nonOptArgCount;
		} else if (nonOptArgCount == 1) {
			cli.input = argv[i];
			++nonOptArgCount;
		} else if (nonOptArgCount == 2) {
			cli.outPath = argv[i];
			++nonOptArgCount;
		} else {
			return false;
		}
	}

	return nonOptArgCount == 3;
}

static std::string GetExtension(const std::string& path)
{
	auto fileNameStart = path.find_last_of("/\\");
	auto fileName = fileNameStart == std::string::npos ? path : path.substr(fileNameStart + 1);
	auto extStart = fileName.find_last_of('.');
	auto ext = extStart == std::string::npos ? "" : fileName.substr(extStart + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); });
	return ext;
}

template <typename T = char>
std::vector<T> ReadFile(const std::string& fn)
{
	std::basic_ifstream<T> ifs(fn, std::ios::binary);
	if (!ifs.good())
		throw std::runtime_error("failed to open/read file " + fn);
	return ifs ? std::vector(std::istreambuf_iterator<T>(ifs), std::istreambuf_iterator<T>()) : std::vector<T>();
};

#if defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT)
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
#endif

int main(int argc, char* argv[])
{
	CLI cli;

	if (!ParseOptions(argc, argv, cli)) {
		PrintUsage(argv[0]);
		return -1;
	}

	try {
#ifdef ZXING_EXPERIMENTAL_API
		auto cOpts = CreatorOptions(cli.format).ecLevel(cli.ecLevel).options(cli.options);
#ifdef ZXING_USE_ZINT
		cOpts.withQuietZones(cli.withQZ).encoding(cli.encoding).margin(cli.margin)
			 .eci(cli.eci).height(cli.height)
			 .readerInit(cli.readerInit).debug(cli.debug).rotate(cli.rotate);
#endif
		auto barcode = cli.inputIsFile ? CreateBarcodeFromBytes(ReadFile(cli.input), cOpts) : CreateBarcodeFromText(cli.input, cOpts);

		auto wOpts = WriterOptions().sizeHint(cli.sizeHint).withQuietZones(cli.withQZ).withHRT(cli.withHRT).rotate(cli.rotate);
#ifdef ZXING_USE_ZINT
		wOpts.scale(cli.scale);
#endif
		auto bitmap = WriteBarcodeToImage(barcode, wOpts);

		if (cli.verbose) {
			std::string text = EscapeNonGraphical(barcode.text(TextMode::Plain));
			std::cout << "Text:       \"" << EscapeNonGraphical(barcode.text(TextMode::Plain)) << "\"\n";
			if (text != barcode.text(TextMode::HRI))
				std::cout << "Text HRI:   \"" << barcode.text(TextMode::HRI) << "\"\n";
			std::cout << "Bytes:      " << ToHex(barcode.bytes()) << "\n";
#ifdef ZXING_USE_ZINT
			std::cout << "Text ECI:   \"" << barcode.text(TextMode::ECI) << "\"\n"
					  << "Bytes ECI:  " << ToHex(barcode.bytesECI()) << "\n";
#endif
			std::cout << "Format:     " << ToString(barcode.format()) << "\n"
					  << "Identifier: " << barcode.symbologyIdentifier() << "\n"
					  << "Content:    " << ToString(barcode.contentType()) << "\n";
			if (barcode.hasECI())
				std::cout << "HasECI:     " << (barcode.hasECI() ? "Y" : "N") << "\n";
#ifdef ZXING_USE_ZINT
			if (barcode.hasECI())
				std::cout << "ECIs:       " << barcode.ECIs() << "\n";
#endif
			std::cout << "Position:   " << ToString(barcode.position()) << "\n";
			if (barcode.orientation())
				std::cout << "Rotation:   " << barcode.orientation() << " deg\n";
			if (barcode.isMirrored())
					std::cout << "IsMirrored: " << barcode.isMirrored() << "\n";
			if (barcode.isInverted())
					std::cout << "IsInverted: " << barcode.isInverted() << "\n";
			if (!barcode.ecLevel().empty())
				std::cout << "EC Level:   " << barcode.ecLevel() << "\n";
			if (!barcode.version().empty()) {
				std::string azType;
				if (barcode.format() == BarcodeFormat::Aztec) {
					if (int version = std::stoi(barcode.version()), height = barcode.bits().height();
							(version == 1 && height % 15 == 0) || (version == 2 && height % 19 == 0)
							 || (version == 3 && height % 23 == 0) || (version == 4 && height % 27 == 0))
						azType = " (Compact)";
					else
						azType = " (Full)";
				}
				std::cout << "Version:    " << barcode.version() << azType << "\n";
			}
			if (barcode.format() == BarcodeFormat::QRCode || barcode.format() == BarcodeFormat::MicroQRCode)
				std::cout << "Data Mask:  " << JsonGetStr(barcode.extra(), "DataMask") << "\n";
			if (barcode.readerInit())
				std::cout << "Reader Initialisation/Programming\n";
			std::cout << WriteBarcodeToUtf8(barcode);
		}
#else
		auto writer = MultiFormatWriter(cli.format).setMargin(cli.withQZ ? 10 : 0);
		if (!cli.ecLevel.empty())
			writer.setEccLevel(std::stoi(cli.ecLevel));

		BitMatrix matrix;
		if (cli.inputIsFile) {
			auto file = ReadFile(cli.input);
			std::wstring bytes;
			for (uint8_t c : file)
				bytes.push_back(c);
			writer.setEncoding(CharacterSet::BINARY);
			matrix = writer.encode(bytes, cli.sizeHint, std::clamp(cli.sizeHint / 2, 50, 300));
		} else {
			writer.setEncoding(CharacterSet::UTF8);
			matrix = writer.encode(cli.input, cli.sizeHint, std::clamp(cli.sizeHint / 2, 50, 300));
		}
		auto bitmap = ToMatrix<uint8_t>(matrix);
#endif

		auto ext = GetExtension(cli.outPath);
		int success = 0;
		if (ext == "" || ext == "png") {
			success = stbi_write_png(cli.outPath.c_str(), bitmap.width(), bitmap.height(), 1, bitmap.data(), 0);
		} else if (ext == "jpg" || ext == "jpeg") {
			success = stbi_write_jpg(cli.outPath.c_str(), bitmap.width(), bitmap.height(), 1, bitmap.data(), 0);
		} else if (ext == "svg") {
#ifdef ZXING_EXPERIMENTAL_API
			success = (std::ofstream(cli.outPath) << WriteBarcodeToSVG(barcode, wOpts)).good();
#else
			success = (std::ofstream(cli.outPath) << ToSVG(matrix)).good();
#endif
		}

		if (!success) {
			std::cerr << "Failed to write image: " << cli.outPath << std::endl;
			return -1;
		}
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
