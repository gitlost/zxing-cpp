/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

// #define USE_OLD_WRITER_API

#include "ZXingCpp.h"

#ifdef ZXING_USE_ZINT
#include "ECI.h"
#include "JSON.h"
#include "Utf.h"
#else
#include "BitMatrixIO.h"
#include "CharacterSet.h"
#include "MultiFormatWriter.h"
#endif
#include "BitMatrix.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace ZXing;

static void PrintUsage(const char* exePath)
{
	std::cout << "Usage: " << exePath
#ifdef ZXING_USE_ZINT
			  << " [-options <creator-options>] [-scale <factor>] [-rotate <angle>] [-binary] [-noqz] [-hrt] [-invert] [-verbose] [-bytes] [-escape] <format> <text> [<output>]\n"
#else
			  << " [-options <creator-options>] [-scale <factor>] [-encoding <charset>] [-rotate <angle>] [-binary] [-noqz] [-hrt] [-invert] [-escape] <format> <text> [<output>]\n"
#endif
			  << "    -options   Comma separated list of format specific options and flags\n"
			  << "    -scale     Module size of generated image / negative numbers mean 'target size in pixels'\n"
#ifndef ZXING_USE_ZINT
			  << "    -encoding  Encoding used to encode input text\n"
#endif
			  << "    -rotate    Rotate image by given angle (90, 180 or 270)\n"
//			  << "    -encoding  Encoding used to encode input text\n"
			  << "    -binary    Interpret <text> as a file name containing binary data\n"
			  << "    -noqz      Print barcode without quiet zone\n"
			  << "    -hrt       Print human readable text below the barcode (if supported)\n"
			  << "    -invert    Invert colors (switch black and white)\n"
#ifdef ZXING_USE_ZINT
			  << "    -verbose   Print barcode information\n"
			  << "    -bytes     Encode input text as-is\n"
#endif
			  << "    -escape    Process escape sequences (2-digit \"\\xXX\", 3-digit \"\\oNNN\" and \"\\\\\")\n"
			  << "    -help      Print usage information\n"
			  << "    -version   Print version information\n"
			  << "\n"
			  << "Supported formats are:";
	for (auto f : BarcodeFormats::list(BarcodeFormat::AllCreatable)) {
		std::cout << " " << ToString(f) << " |";
	}
	std::cout << "\n";

	std::cout << "Format can be lowercase letters, with or without any of ' -_/'.\n"
			  << "Output format is determined by file name, supported are png, jpg and svg.\n";
#ifndef ZXING_USE_ZINT
	std::cout << "Supported encodings are:";
	for (int i = 1; i < int(CharacterSet::CharsetCount); i++) {
		std::cout << " " << ToString(static_cast<CharacterSet>(i)) << " |";
	}
	std::cout << "\n";
#endif
}

struct CLI
{
	BarcodeFormat format = BarcodeFormat::None;
	int scale = 0;
	int rotate = 0;
	std::string input;
	std::string outPath = "zxingwriter_out.png";
	std::string options;
	bool inputIsFile = false;
	bool invert = false;
	bool addHRT = false;
	bool addQZs = true;
	bool escape = false;
	bool verbose = false;
#ifndef ZXING_USE_ZINT
	CharacterSet encoding = CharacterSet::Unknown;
#endif
#ifdef ZXING_USE_ZINT
	bool bytes = false;
#endif
};

static bool ParseOptions(int argc, char* argv[], CLI& cli)
{
	int nonOptArgCount = 0;
	for (int i = 1; i < argc; ++i) {
		auto is = [&](const char* str) { return strlen(argv[i]) >= 3 && strncmp(argv[i], str, strlen(argv[i])) == 0; };
		if (is("-scale")) {
			if (++i == argc)
				return false;
			cli.scale = std::stoi(argv[i]);
#ifndef ZXING_USE_ZINT
		} else if (is("-encoding")) {
			if (++i == argc)
				return false;
			if ((cli.encoding = CharacterSetFromString(argv[i])) == CharacterSet::Unknown)
				return false;
#endif
#ifdef ZXING_USE_ZINT
		} else if (is("-bytes")) {
			cli.bytes = true;
		} else if (is("-rotate")) {
			if (++i == argc)
				return false;
			cli.rotate = std::stoi(argv[i]);
#endif
		} else if (is("-binary")) {
			cli.inputIsFile = true;
		} else if (is("-hrt")) {
			cli.addHRT = true;
		} else if (is("-noqz")) {
			cli.addQZs = false;
		} else if (is("-invert")) {
			cli.invert = true;
		} else if (is("-options")) {
			if (++i == argc)
				return false;
			cli.options = argv[i];
		} else if (is("-escape")) {
			cli.escape = true;
		} else if (is("-verbose")) {
			cli.verbose = true;
		} else if (is("-help") || is("--help")) {
			PrintUsage(argv[0]);
			exit(0);
		} else if (is("-version") || is("--version")) {
			std::cout << "ZXingWriter " << ZXING_VERSION_STR << "\n";
			exit(0);
		} else if (nonOptArgCount == 0) {
			try {
				cli.format = BarcodeFormatFromString(argv[i]);
			} catch (const std::exception& e) {
				std::cerr << "Error: " << e.what() << "\n\n";
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

	return nonOptArgCount >= 2;
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

#ifdef ZXING_USE_ZINT
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

static std::string escape(std::string_view input)
{
	std::string output;
	output.reserve(input.size());
	auto xtoi = [&](const char ch) { return ch <= '9' && ch >= '0' ? ch - '0' : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : ch - 'a' + 10; };
	auto isodigit = [&](const char ch) { return ch <= '7' && ch >= '0'; };
	auto otoi = [&](const char ch) { return ch - '0'; };
                               /* NUL   EOT   BEL   BS    HT    LF    VT    FF    CR    ESC   GS    RS   \ */
    static const char escs[] = {  '0',  'E',  'a',  'b',  't',  'n',  'v',  'f',  'r',  'e',  'G',  'R', '\\', '\0' };
    static const char vals[] = { 0x00, 0x04, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x1B, 0x1D, 0x1E, 0x5C };

	for (int i = 0, len = input.size(); i < len; i++) {
		if (input[i] == '\\') {
			const char *esc_ch;
			if (++i == len)
				throw std::runtime_error("single backslash at end");
			if ((esc_ch = strchr(escs, input[i])) != NULL)
				output.push_back(vals[esc_ch - escs]);
			else if (i + 2 < len && input[i] == 'x' && std::isxdigit(input[i + 1]) && std::isxdigit(input[i + 2]))
				output.push_back((xtoi(input[i + 1]) << 4) | xtoi(input[i + 2])), i += 2;
			else if (i + 3 < len && input[i] == 'o' && isodigit(input[i + 1]) && isodigit(input[i + 2]) && isodigit(input[i + 3]))
				output.push_back((otoi(input[i + 1]) << 6) | (otoi(input[i + 2]) << 3) | otoi(input[i + 3])), i += 3;
			else
				throw std::runtime_error("invalid escape sequence " + std::string(input.data() + i - 1));
		} else
			output.push_back(input[i]);
	}
	return output;
}

int main(int argc, char* argv[])
{
	CLI cli;

	if (!ParseOptions(argc, argv, cli)) {
		PrintUsage(argv[0]);
		return -1;
	}

	try {
#ifdef ZXING_USE_ZINT
		auto cOpts = CreatorOptions(cli.format, cli.options);
		auto barcode = cli.inputIsFile ? CreateBarcodeFromBytes(ReadFile(cli.input), cOpts)
					   : cli.bytes ? CreateBarcodeFromBytes(cli.input, cOpts) : CreateBarcodeFromText(cli.input, cOpts);

		auto wOpts = WriterOptions().scale(cli.scale).addQuietZones(cli.addQZs).addHRT(cli.addHRT).invert(cli.invert).rotate(cli.rotate);
		auto bitmap = WriteBarcodeToImage(barcode, wOpts);

		if (cli.verbose) {
			std::cout.setf(std::ios::boolalpha);
			std::string text = EscapeNonGraphical(barcode.text(TextMode::Plain));
			std::cout << "Text:       \"" << EscapeNonGraphical(barcode.text(TextMode::Plain)) << "\"\n";
			if (text != barcode.text(TextMode::HRI))
				std::cout << "Text HRI:   \"" << barcode.text(TextMode::HRI) << "\"\n";
			std::cout << "Bytes:      " << ToHex(barcode.bytes()) << "\n";
			std::cout << "Text ECI:   \"" << barcode.text(TextMode::ECI) << "\"\n"
					  << "Bytes ECI:  " << ToHex(barcode.bytesECI()) << "\n";
			std::cout << "Format:     " << ToString(barcode.format()) << "\n"
					  << "Symbology:  " << ToString(barcode.symbology()) << "\n"
					  << "Identifier: " << barcode.symbologyIdentifier() << "\n"
					  << "Content:    " << ToString(barcode.contentType()) << "\n";
			if (barcode.hasECI())
				std::cout << "HasECI:     " << (barcode.hasECI() ? "Y" : "N") << "\n";
			if (barcode.hasECI())
				std::cout << "ECIs:       " << barcode.ECIs() << "\n";
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
					if (int version = std::stoi(barcode.version()), height = barcode.symbolMatrix().height();
							(version == 1 && height % 15 == 0) || (version == 2 && height % 19 == 0)
							 || (version == 3 && height % 23 == 0) || (version == 4 && height % 27 == 0))
						azType = " (Compact)";
					else
						azType = " (Full)";
				}
				std::cout << "Version:    " << barcode.version() << azType << "\n";
			}
			if (barcode.format() == BarcodeFormat::QRCode || barcode.format() == BarcodeFormat::MicroQRCode)
				std::cout << "Data Mask:  " << barcode.extra("DataMask") << "\n";
			if (barcode.readerInit())
				std::cout << "Reader Initialisation/Programming\n";
			std::cout << WriteBarcodeToUtf8(barcode);
		}
#else // 'old' writer API (non zint based)
#ifdef _MSC_VER
#pragma warning(suppress : 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
		auto writer = MultiFormatWriter(cli.format).setMargin(cli.addQZs ? 10 : 0);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

		BitMatrix matrix;
		if (cli.inputIsFile) {
			auto file = ReadFile(cli.input);
			std::wstring bytes;
			for (uint8_t c : file)
				bytes.push_back(c);
			writer.setEncoding(CharacterSet::BINARY);
			matrix = writer.encode(bytes, cli.scale, std::clamp(cli.scale / 2, 50, 300));
		} else {
			if (cli.escape)
				cli.input = escape(cli.input);
			writer.setEncoding(cli.encoding != CharacterSet::Unknown ? cli.encoding : CharacterSet::UTF8);
			matrix = writer.encode(cli.input, cli.scale, std::clamp(cli.scale / 2, 50, 300));
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
#ifdef ZXING_USE_ZINT
			success = (std::ofstream(cli.outPath) << WriteBarcodeToSVG(barcode, wOpts)).good();
#else
			success = (std::ofstream(cli.outPath) << ToSVG(matrix)).good();
#endif
		}

		if (!success) {
			std::cerr << "Failed to write image: " << cli.outPath << "\n";
			return -1;
		}
	} catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
		return -1;
	}

	return 0;
}
