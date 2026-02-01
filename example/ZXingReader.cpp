/*
* Copyright 2016 Nu-book Inc.
* Copyright 2019 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ZXingCpp.h"

#include "BitMatrix.h"
#include "BitMatrixIO.h"
#ifdef ZX_DIAGNOSTICS
#include "Diagnostics.h"
#endif
#include "GTIN.h"
#ifdef ZXING_EXPERIMENTAL_API
#include "JSON.h"
#endif
#include "TextDecoder.h"
#include "Utf.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace ZXing;

static const char *binarizers[] = { "LocalAverage", "GlobalHistogram", "FixedThreshold", "BoolCast" };

struct CLI
{
	std::vector<std::string> filePaths;
	std::string outPath;
	int forceChannels = 0;
	int rotate = 0;
	bool oneLine = false;
	bool bytesOnly = false;
	bool showSymbol = false;
	bool angleEscape = false;
	bool json = false;
};

static void PrintUsage(const char* exePath)
{
	std::cout << "Usage: " << exePath << " [options] <image file>...\n"
			  << "    -fast         Skip some lines/pixels during detection (faster)\n"
			  << "    -norotate     Don't try rotated image during detection (faster)\n"
			  << "    -noinvert     Don't search for inverted codes during detection (faster)\n"
			  << "    -noscale      Don't try downscaled images during detection (faster)\n"
			  << "    -formats <FORMAT[,...]>\n"
			  << "                  Only detect given format(s) (faster)\n"
			  << "    -single       Stop after the first barcode is detected (faster)\n"
			  << "    -ispure       Assume the image contains only a 'pure'/perfect code (faster)\n"
			  << "    -errors       Include results with errors (like checksum error)\n"
			  << "    -1            Print main details on one line per file/barcode (implies '-escape')\n"
			  << "    -escape       Escape non-graphical characters in angle brackets\n"
			  << "    -bytes        Write (only) the bytes content of the symbol(s) to stdout\n"
#ifdef ZXING_EXPERIMENTAL_API
			  << "    -symbol       Print the detected symbol (if available)\n"
			  << "    -json         Print a complete JSON formatted serialization\n"
			  << "    -denoise      Use extra denoiseing (closing operation)\n"
#endif
			  << "    -pngout <file name>\n"
			  << "                  Write a copy of the input image with barcodes outlined by a green line\n"
			  << "    -binarizer <BINARIZER>\n"
			  << "                  Use specific binarizer (default LocalAverage)\n"
			  << "    -mode <plain|eci|hri|escaped>\n"
			  << "                  Text mode used to render the raw byte content into text\n"
			  << "    -charset <CHARSET>\n"
			  << "                  Default character set\n"
#ifdef ZX_DIAGNOSTICS
			  << "    -diagnostics  Print diagnostics\n"
#endif
			  << "    -help         Print usage information and exit\n"
			  << "    -version      Print version information\n"
			  << "\n"
			  << "Supported formats are:\n" << "   ";
	for (auto f : BarcodeFormats::list(BarcodeFormat::AllReadable)) {
		std::cout << " " << ToString(f);
	}
	std::cout << "BarcodeFormats can be lowercase, with or without any of ' -_/', separated by ',' or '|'\n";

	std::cout << "\n" << "Supported binarizers are:\n" << "   ";
	for (int j = 0; j < 4; j++) {
		std::cout << " " << binarizers[j];
	}
	std::cout << "\n";
}

static bool ParseOptions(int argc, char* argv[], ReaderOptions& options, CLI& cli)
{
	for (int i = 1; i < argc; ++i) {
		auto is = [&](const char* str) { return strlen(argv[i]) > 1 && strncmp(argv[i], str, strlen(argv[i])) == 0; };
		if (is("-fast")) {
			options.tryHarder(false);
		} else if (is("-norotate")) {
			options.tryRotate(false);
		} else if (is("-noinvert")) {
			options.tryInvert(false);
		} else if (is("-noscale")) {
			options.tryDownscale(false);
#ifdef ZXING_EXPERIMENTAL_API
		} else if (is("-denoise")) {
			options.tryDenoise(true);
#endif
		} else if (is("-single")) {
			options.maxNumberOfSymbols(1);
		} else if (is("-ispure")) {
			options.isPure(true);
			options.binarizer(Binarizer::FixedThreshold);
		} else if (is("-errors")) {
			options.returnErrors(true);
		} else if (is("-formats")) {
			if (++i == argc)
				return false;
			try {
				options.formats(BarcodeFormatsFromString(argv[i]));
			} catch (const std::exception& e) {
				std::cerr << "Error: " << e.what() << "\n\n";
				return false;
			}
		} else if (is("-binarizer")) {
			if (++i == argc)
				return false;
			std::string binarizer(argv[i]);
			int j;
			for (j = 0; j < 4; j++) {
				if (binarizer == binarizers[j]) {
					options.setBinarizer((Binarizer)j);
					break;
				}
			}
			if (j == 4) {
				if (is("local"))
					options.setBinarizer(Binarizer::LocalAverage);
				else if (is("global"))
					options.setBinarizer(Binarizer::GlobalHistogram);
				else if (is("fixed"))
					options.setBinarizer(Binarizer::FixedThreshold);
				else {
					std::cerr << "Unknown binarizer '" << binarizer << "'\n";
					return false;
				}
			}
		} else if (is("-charset")) {
			if (++i == argc) {
				std::cerr << "No argument for -charset\n";
				return false;
			}
			options.setCharacterSet(argv[i]);
		} else if (is("-diagnostics")) {
#ifdef ZX_DIAGNOSTICS
			options.setEnableDiagnostics(true);
			options.setReturnErrors(true);
#else
			std::cerr << "Warning: ignoring '-diagnostics' option, BUILD_DIAGNOSTICS not enabled\n";
#endif
		} else if (is("-mode")) {
			if (++i == argc)
				return false;
			else if (is("plain"))
				options.textMode(TextMode::Plain);
			else if (is("eci"))
				options.textMode(TextMode::ECI);
			else if (is("hri"))
				options.textMode(TextMode::HRI);
			else if (is("escaped"))
				options.textMode(TextMode::Escaped);
			else
				return false;
		} else if (is("-1")) {
			cli.oneLine = true;
		} else if (is("-escape")) {
			cli.angleEscape = true;
		} else if (is("-bytes")) {
			cli.bytesOnly = true;
		} else if (is("-symbol")) {
			cli.showSymbol = true;
		} else if (is("-json")) {
			cli.json = true;
		} else if (is("-pngout")) {
			if (++i == argc)
				return false;
			cli.outPath = argv[i];
		} else if (is("-channels")) {
			if (++i == argc)
				return false;
			cli.forceChannels = std::stoi(argv[i]);
		} else if (is("-rotate")) {
			if (++i == argc)
				return false;
			cli.rotate = std::stoi(argv[i]);
		} else if (is("-help") || is("--help")) {
			PrintUsage(argv[0]);
			exit(0);
		} else if (is("-version") || is("--version")) {
			std::cout << "ZXingReader " << Version() << "\n";
			exit(0);
		} else {
			cli.filePaths.push_back(argv[i]);
		}
	}

	return !cli.filePaths.empty();
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

void drawLine(const ImageView& iv, PointI a, PointI b, bool error)
{
	int steps = maxAbsComponent(b - a);
	PointF dir = bresenhamDirection(PointF(b - a));
	int R = RedIndex(iv.format()), G = GreenIndex(iv.format()), B = BlueIndex(iv.format());
	for (int i = 0; i < steps; ++i) {
		auto p = PointI(centered(a + i * dir));
		auto* dst = const_cast<uint8_t*>(iv.data(p.x, p.y));
		if (dst < iv.data(0, 0) || dst > iv.data(iv.width() - 1, iv.height() - 1))
			continue;
		dst[R] = error ? 0xff : 0;
		dst[G] = error ? 0 : 0xff;
		dst[B] = 0;
	}
}

void drawRect(const ImageView& image, const Position& pos, bool error)
{
	for (int i = 0; i < 4; ++i)
		drawLine(image, pos[i], pos[(i + 1) % 4], error);
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
	ReaderOptions options;
	CLI cli;
	Barcodes allBarcodes;
	int ret = 0;

	options.textMode(TextMode::HRI);
	options.eanAddOnSymbol(EanAddOnSymbol::Read);

	if (!ParseOptions(argc, argv, options, cli)) {
		PrintUsage(argv[0]);
		return argc == 1 ? 0 : -1;
	}

	std::string mode = "";
	if (options.textMode() == TextMode::ECI)
		mode = "ECI";
	else if (options.textMode() == TextMode::HRI)
		mode = "HRI";
	else if (options.textMode() == TextMode::Escaped)
		mode = "ESC";

	std::cout.setf(std::ios::boolalpha);

	if (!cli.outPath.empty())
		cli.forceChannels = 3; // the drawing code only works for RGB data

	for (const auto& filePath : cli.filePaths) {
		int width, height, channels;
		std::unique_ptr<stbi_uc, void (*)(void*)> buffer(
			filePath == "-" ? stbi_load_from_file(stdin, &width, &height, &channels, cli.forceChannels)
							: stbi_load(filePath.c_str(), &width, &height, &channels, cli.forceChannels),
			stbi_image_free);
		if (buffer == nullptr) {
			std::cerr << "Failed to read image: " << filePath << " (" << stbi_failure_reason() << ")" << "\n";
			continue;
		}
		channels = cli.forceChannels ? cli.forceChannels : channels;

		//std::cerr << "File " << filePath << "\n";

		auto ImageFormatFromChannels = std::array{ImageFormat::None, ImageFormat::Lum, ImageFormat::LumA, ImageFormat::RGB, ImageFormat::RGBA};
		ImageView image{buffer.get(), width, height, ImageFormatFromChannels.at(channels)};
		Barcodes barcodes;
		try {
			barcodes = ReadBarcodes(image.rotated(cli.rotate), options);
		} catch (Error e) {
			std::cerr << filePath << " Exception: " << e.msg() << "\n";
#ifdef ZX_DIAGNOSTICS
			if (options.enableDiagnostics()) {
				std::cerr << "  Diagnostics: " << Diagnostics::print(nullptr);
				Diagnostics::clear();
			}
#endif
			continue;
		}

		// if we did not find anything, insert a dummy to produce some output for each file
		if (barcodes.empty())
			barcodes.emplace_back();

		allBarcodes.insert(allBarcodes.end(), barcodes.begin(), barcodes.end());
		if (filePath == cli.filePaths.back()) {
			auto merged = MergeStructuredAppendSequences(allBarcodes);
			// report all merged sequences as part of the last file to make the logic not overly complicated here
			barcodes.insert(barcodes.end(), std::make_move_iterator(merged.begin()), std::make_move_iterator(merged.end()));
		}

		for (auto&& barcode : barcodes) {

			if (!cli.outPath.empty())
				drawRect(image, barcode.position(), bool(barcode.error()));

			ret |= static_cast<int>(barcode.error().type());

			if (cli.bytesOnly) {
				std::cout.write(reinterpret_cast<const char*>(barcode.bytes().data()), barcode.bytes().size());
				continue;
			}

#ifdef ZX_DIAGNOSTICS
			if (options.enableDiagnostics()) {
				barcode.setContentDiagnostics();
			}
#endif
#ifdef ZXING_EXPERIMENTAL_API
			if (cli.json) {
				if (barcode.format() != BarcodeFormat::None)
					std::cout << "{\"FilePath\":\"" << filePath << "\"," << barcode.extra("ALL").substr(1) << "\n";
				continue;
			}
#endif

			if (cli.oneLine) {
				std::cout << filePath << " " << ToString(barcode.format()) << " " << barcode.symbologyIdentifier()
							<< " \"" << EscapeNonGraphical(appendBinIfTextEmpty(barcode)) << "\" " << ToString(barcode.error());
#ifdef ZX_DIAGNOSTICS
				if (options.enableDiagnostics() && !barcode.diagnostics().empty()) {
					std::cout << Diagnostics::print(&barcode.diagnostics(), true /*skipToDecode*/);
				}
#endif
				std::cout << "\n";
				continue;
			}

			if (cli.filePaths.size() > 1 || barcodes.size() > 1) {
				static bool firstFile = true;
				if (!firstFile)
					std::cout << "\n";
				if (cli.filePaths.size() > 1)
					std::cout << "File:       " << filePath << "\n";
				firstFile = false;
			}

			if (barcode.format() == BarcodeFormat::None) {
				std::cout << "No barcode found\n";
				continue;
			}

			std::string text = appendBinIfTextEmpty(barcode);
			std::cout << "Text:         \"" << (cli.angleEscape ? EscapeNonGraphical(text) : text) << "\"\n";
			if (options.textMode() != TextMode::Plain)
				std::cout << "Text " << mode << ":     \"" << EscapeNonGraphical(barcode.text()) << "\"\n";
			std::cout << "Bytes:        " << ToHex(barcode.bytes()) << "\n";
			if (options.textMode() == TextMode::ECI)
				std::cout << "Bytes ECI:    " << ToHex(barcode.bytesECI()) << "\n";
			std::cout << "Length:       " << text.size() << "\n"
					  << "Format:       " << ToString(barcode.format()) << "\n"
					  << "Symbology:    " << ToString(barcode.symbology()) << "\n"
					  << "Identifier:   " << barcode.symbologyIdentifier() << "\n"
					  << "Content Type: " << ToString(barcode.contentType()) << "\n";

			if (barcode.contentType() == ContentType::GS1 || barcode.contentType() == ContentType::ISO15434)
				std::cout << "HRI:          \"" << barcode.text(TextMode::HRI) << "\"\n";

			if (Size(barcode.ECIs()))
				std::cout << "ECIs:         (" << Size(barcode.ECIs()) << ") " << barcode.ECIs() << "\n";

			std::cout << "Position:     " << ToString(barcode.position()) << "\n";
			if (barcode.orientation())
				std::cout << "Rotation:     " << barcode.orientation() << " deg\n";
			if (barcode.isMirrored())
				std::cout << "IsMirrored:   " << barcode.isMirrored() << "\n";
			if (barcode.isInverted())
				std::cout << "IsInverted:   " << barcode.isInverted() << "\n";

			auto printOptional = [](const char* key, const std::string& v) {
				if (!v.empty())
					std::cout << key << v << "\n";
			};

			printOptional("EC Level:     ", barcode.ecLevel());
			std::string azType;
#ifdef ZXING_EXPERIMENTAL_API
			if (barcode.format() == BarcodeFormat::Aztec && !barcode.version().empty()) {
				if (int version = std::stoi(barcode.version()), height = barcode.symbolMatrix().height();
						(version == 1 && height % 15 == 0) || (version == 2 && height % 19 == 0)
						 || (version == 3 && height % 23 == 0) || (version == 4 && height % 27 == 0))
					azType = " (Compact)";
				else
					azType = " (Full)";
			}
#endif
			printOptional("Version:      ", barcode.version() + azType);
			printOptional("Error:        ", ToString(barcode.error()));
#ifdef ZXING_EXPERIMENTAL_API
			printOptional("Extra:        ", barcode.extra());
#endif

			if (barcode.lineCount())
				std::cout << "Lines:        " << barcode.lineCount() << "\n";

			if (barcode.symbology() == BarcodeFormat::EANUPC) {
				printOptional("Country:      ", GTIN::LookupCountryIdentifier(barcode.text(TextMode::Plain), barcode.format()));
				printOptional("Add-On:       ", GTIN::EanAddOn(barcode));
				printOptional("Price:        ", GTIN::Price(GTIN::EanAddOn(barcode)));
				printOptional("Issue #:      ", GTIN::IssueNr(GTIN::EanAddOn(barcode)));
			} else if (barcode.format() == BarcodeFormat::ITF && Size(barcode.bytes()) == 14) {
				printOptional("Country:      ", GTIN::LookupCountryIdentifier(barcode.text(TextMode::Plain), barcode.format()));
			}

			if (barcode.isPartOfSequence()) {
				std::cout << "Structured Append\n";
				if (barcode.sequenceSize() > 0)
					std::cout << "    Sequence: " << barcode.sequenceIndex() + 1 << " of " << barcode.sequenceSize() << "\n";
				else
					std::cout << "    Sequence: " << barcode.sequenceIndex() + 1 << " of unknown number\n";
				if (!barcode.sequenceId().empty())
					std::cout << "    Id:       \"" << barcode.sequenceId() << "\"\n";
			}

			if (barcode.readerInit())
				std::cout << "Reader Initialisation/Programming\n";

#ifdef ZX_DIAGNOSTICS
			if (options.enableDiagnostics())
				std::cout << "Diagnostics" << Diagnostics::print(&barcode.diagnostics());
#endif
#ifdef ZXING_EXPERIMENTAL_API
			if (cli.showSymbol && barcode.symbol().data())
				std::cout << "Symbol:\n" << WriteBarcodeToUtf8(barcode);
#endif
		}

		if (cli.filePaths.size() == 1 && !cli.outPath.empty())
			stbi_write_png(cli.outPath.c_str(), image.width(), image.height(), 3, image.data(), image.rowStride());

#ifdef NDEBUG
		if (getenv("MEASURE_PERF")) {
			auto startTime = std::chrono::high_resolution_clock::now();
			auto duration = startTime - startTime;
			int N = 0;
			int blockSize = 1;
			do {
				for (int i = 0; i < blockSize; ++i)
					ReadBarcodes(image, options);
				N += blockSize;
				duration = std::chrono::high_resolution_clock::now() - startTime;
				if (blockSize < 1000 && duration < std::chrono::milliseconds(100))
					blockSize *= 10;
			} while (duration < std::chrono::seconds(1));
			printf("time: %5.2f ms per frame\n", double(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) / N);
		}
#endif
	}

	return ret;
}
