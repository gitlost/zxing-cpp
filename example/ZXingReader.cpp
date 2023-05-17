/*
* Copyright 2016 Nu-book Inc.
* Copyright 2019 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0


#ifdef ZX_DIAGNOSTICS
#include "Diagnostics.h"
#endif
#include "ReadBarcode.h"
#include "GTIN.h"
#include "Utf.h"
#include "pdf417/PDFDecoderResultExtra.h"
#include "ZXVersion.h"

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

static void PrintUsage(const char* exePath)
{
	std::cout << "Usage: " << exePath << " [options] <image file>...\n"
			  << "    -fast         Skip some lines/pixels during detection (faster)\n"
			  << "    -norotate     Don't try rotated image during detection (faster)\n"
			  << "    -noinvert     Don't search for inverted codes during detection (faster)\n"
			  << "    -noscale      Don't try downscaled images during detection (faster)\n"
			  << "    -format <FORMAT[,...]>\n"
			  << "                  Only detect given format(s) (faster)\n"
			  << "    -ispure       Assume the image contains only a 'pure'/perfect code (faster)\n"
			  << "    -errors       Include results with errors (like checksum error)\n"
			  << "    -1            Print main details on one line per file/barcode (implies '-escape')\n"
			  << "    -escape       Escape non-graphical characters in angle brackets\n"
			  << "    -bytes        Write (only) the bytes content of the symbol(s) to stdout\n"
			  << "    -pngout <file name>\n"
			  << "                  Write a copy of the input image with barcodes outlined by a green line\n"
			  << "    -binarizer <BINARIZER>\n"
			  << "                  Use specific binarizer (default LocalAverage)\n"
			  << "    -charset <CHARSET>\n"
			  << "                  Default character set\n"
#ifdef ZX_DIAGNOSTICS
			  << "    -diagnostics  Print diagnostics\n"
#endif
			  << "    -help         Print usage information and exit\n"
			  << "    -version      Print version information\n"
			  << "\n"
			  << "Supported formats are:\n" << "   ";
	for (auto f : BarcodeFormats::all()) {
		std::cout << " " << ToString(f);
	}
	std::cout << "\nFormats can be lowercase, with or without '-', separated by ',' and/or '|'\n";

	std::cout << "\n" << "Supported binarizers are:\n" << "   ";
	for (int j = 0; j < 4; j++) {
		std::cout << " " << binarizers[j];
	}
	std::cout << "\n";
}

static bool ParseOptions(int argc, char* argv[], DecodeHints& hints, bool& oneLine, bool& angleEscape, bool& bytesOnly,
						 std::vector<std::string>& filePaths, std::string& outPath)
{
	hints.setTryDenoise(true);

	for (int i = 1; i < argc; ++i) {
		auto is = [&](const char* str) { return strncmp(argv[i], str, strlen(argv[i])) == 0; };
		if (is("-fast")) {
			hints.setTryHarder(false);
			hints.setTryDenoise(false);
		} else if (is("-norotate")) {
			hints.setTryRotate(false);
		} else if (is("-noinvert")) {
			hints.setTryInvert(false);
		} else if (is("-noscale")) {
			hints.setTryDownscale(false);
		} else if (is("-ispure")) {
			hints.setIsPure(true);
			hints.setBinarizer(Binarizer::FixedThreshold);
		} else if (is("-errors")) {
			hints.setReturnErrors(true);
		} else if (is("-format")) {
			if (++i == argc)
				return false;
			try {
				hints.setFormats(BarcodeFormatsFromString(argv[i]));
			} catch (const std::exception& e) {
				std::cerr << e.what() << "\n";
				return false;
			}
		} else if (is("-binarizer")) {
			if (++i == argc)
				return false;
			std::string binarizer(argv[i]);
			int j;
			for (j = 0; j < 4; j++) {
				if (binarizer == binarizers[j]) {
					hints.setBinarizer((Binarizer)j);
					break;
				}
			}
			if (j == 4) {
				std::cerr << "Unknown binarizer '" << binarizer << "'\n";
				return false;
			}
		} else if (is("-charset")) {
			if (++i == argc) {
				std::cerr << "No argument for -charset\n";
				return false;
			}
			hints.setCharacterSet(argv[i]);
		} else if (is("-diagnostics")) {
#ifdef ZX_DIAGNOSTICS
			hints.setEnableDiagnostics(true);
			hints.setReturnErrors(true);
#else
			std::cerr << "Warning: ignoring '-diagnostics' option, BUILD_DIAGNOSTICS not enabled\n";
#endif
		} else if (is("-1")) {
			oneLine = true;
		} else if (is("-escape")) {
			angleEscape = true;
		} else if (is("-bytes")) {
			bytesOnly = true;
		} else if (is("-pngout")) {
			if (++i == argc)
				return false;
			outPath = argv[i];
		} else if (is("-help") || is("--help")) {
			PrintUsage(argv[0]);
			exit(0);
		} else if (is("-version") || is("--version")) {
			std::cout << "ZXingReader " << ZXING_VERSION_STR << "\n";
			exit(0);
		} else {
			filePaths.push_back(argv[i]);
		}
	}

	return !filePaths.empty();
}

std::ostream& operator<<(std::ostream& os, const Position& points)
{
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

int main(int argc, char* argv[])
{
	DecodeHints hints;
	std::vector<std::string> filePaths;
	Results allResults;
	std::string outPath;
	bool oneLine = false;
	bool angleEscape = false;
	bool bytesOnly = false;
	int ret = 0;

	hints.setTryDenoise(true);
	hints.setTextMode(TextMode::HRI);
	hints.setEanAddOnSymbol(EanAddOnSymbol::Read);

	if (!ParseOptions(argc, argv, hints, oneLine, angleEscape, bytesOnly, filePaths, outPath)) {
		PrintUsage(argv[0]);
		return argc == 1 ? 0 : -1;
	}

	std::cout.setf(std::ios::boolalpha);

	for (const auto& filePath : filePaths) {
		int width, height, channels;
		std::unique_ptr<stbi_uc, void(*)(void*)> buffer(stbi_load(filePath.c_str(), &width, &height, &channels, 3), stbi_image_free);
		if (buffer == nullptr) {
			std::cerr << "Failed to read image: " << filePath << "\n";
			continue;
		}
		//std::cerr << "File " << filePath << "\n";

		ImageView image{buffer.get(), width, height, ImageFormat::RGB};
		Results results;
		try {
			results = ReadBarcodes(image, hints);
		} catch (Error e) {
			std::cerr << filePath << " Exception: " << e.msg() << "\n";
#ifdef ZX_DIAGNOSTICS
			if (hints.enableDiagnostics()) {
				std::cerr << "  Diagnostics: " << Diagnostics::print(nullptr);
				Diagnostics::clear();
			}
#endif
			continue;
		}

		// if we did not find anything, insert a dummy to produce some output for each file
		if (results.empty())
			results.emplace_back();

		allResults.insert(allResults.end(), results.begin(), results.end());
		if (filePath == filePaths.back()) {
			auto merged = MergeStructuredAppendSequences(allResults);
			// report all merged sequences as part of the last file to make the logic not overly complicated here
			results.insert(results.end(), std::make_move_iterator(merged.begin()), std::make_move_iterator(merged.end()));
		}

		for (auto&& result : results) {

			if (!outPath.empty())
				drawRect(image, result.position(), bool(result.error()));

			ret |= static_cast<int>(result.error().type());

			if (bytesOnly) {
				std::cout.write(reinterpret_cast<const char*>(result.bytes().data()), result.bytes().size());
				continue;
			}

#ifdef ZX_DIAGNOSTICS
			if (hints.enableDiagnostics()) {
				result.setContentDiagnostics();
			}
#endif

			if (oneLine) {
				std::cout << filePath << " " << ToString(result.format()) << " " << result.symbologyIdentifier()
							<< " \"" << EscapeNonGraphical(result.text(TextMode::Plain)) << "\" " << ToString(result.error());
#ifdef ZX_DIAGNOSTICS
				if (hints.enableDiagnostics() && !result.diagnostics().empty()) {
					std::cout << Diagnostics::print(&result.diagnostics(), true /*skipToDecode*/);
				}
#endif
				std::cout << "\n";
				continue;
			}

			if (filePaths.size() > 1 || results.size() > 1) {
				static bool firstFile = true;
				if (!firstFile)
					std::cout << "\n";
				if (filePaths.size() > 1)
					std::cout << "File:       " << filePath << "\n";
				firstFile = false;
			}

			if (result.format() == BarcodeFormat::None) {
				std::cout << "No barcode found\n";
				continue;
			}

			std::cout << "Text:       \"" << (angleEscape ? EscapeNonGraphical(result.text(TextMode::Plain)) : result.text(TextMode::Plain)) << "\"\n"
					  //<< "Bytes:      (" << Size(result.bytes()) << ") " << ToHex(result.bytes()) << "\n"
					  << "Format:     " << ToString(result.format()) << "\n"
					  << "Identifier: " << result.symbologyIdentifier() << "\n";

			if (result.contentType() == ContentType::GS1 || result.contentType() == ContentType::ISO15434)
				std::cout << "HRI:        \"" << result.text(TextMode::HRI) << "\"\n";

			if (Size(result.ECIs()))
				std::cout << "ECIs:       (" << Size(result.ECIs()) << ") " << result.ECIs() << "\n";

			std::cout << "Position:   " << result.position() << "\n";
			if (result.orientation())
				std::cout << "Rotation:   " << result.orientation() << " deg\n";
			if (result.isMirrored())
				std::cout << "IsMirrored: " << result.isMirrored() << "\n";
			if (result.isInverted())
				std::cout << "IsInverted: " << result.isInverted() << "\n";

			auto printOptional = [](const char* key, const std::string& v) {
				if (!v.empty())
					std::cout << key << v << "\n";
			};

			printOptional("EC Level:   ", result.ecLevel());
			printOptional("Version:    ", result.version());
			printOptional("Error:      ", ToString(result.error()));

			if (result.lineCount())
				std::cout << "Lines:      " << result.lineCount() << "\n";

			if ((BarcodeFormat::EAN13 | BarcodeFormat::EAN8 | BarcodeFormat::UPCA | BarcodeFormat::UPCE)
					.testFlag(result.format())) {
				printOptional("Country:    ", GTIN::LookupCountryIdentifier(result.text(TextMode::Plain), result.format()));
				printOptional("Add-On:     ", GTIN::EanAddOn(result));
				printOptional("Price:      ", GTIN::Price(GTIN::EanAddOn(result)));
				printOptional("Issue #:    ", GTIN::IssueNr(GTIN::EanAddOn(result)));
			} else if (result.format() == BarcodeFormat::ITF && Size(result.bytes()) == 14) {
				printOptional("Country:    ", GTIN::LookupCountryIdentifier(result.text(TextMode::Plain), result.format()));
			}

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
			if (hints.enableDiagnostics())
				std::cout << "Diagnostics" << Diagnostics::print(&result.diagnostics());
#endif
		}

		if (Size(filePaths) == 1 && !outPath.empty())
			stbi_write_png(outPath.c_str(), image.width(), image.height(), 3, image.data(0, 0), image.rowStride());

#ifdef NDEBUG
		if (getenv("MEASURE_PERF")) {
			auto startTime = std::chrono::high_resolution_clock::now();
			auto duration = startTime - startTime;
			int N = 0;
			int blockSize = 1;
			do {
				for (int i = 0; i < blockSize; ++i)
					ReadBarcodes(image, hints);
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
