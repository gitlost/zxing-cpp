/*
 * Copyright 2025 gitlost
 * Copyright 2025 Axel Waggershauser
 */
// SPDX-License-Identifier: Apache-2.0

#include "GTIN.h"
#include "Version.h"

#include <iomanip>

#if defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_WRITERS) && defined(ZXING_USE_ZINT)

#include "BitMatrix.h"
#include "BitMatrixIO.h"
#include "JSON.h"
#ifdef ZXING_READERS
#include "ReadBarcode.h"
#endif
#include "WriteBarcode.h"

#include "gtest/gtest.h"

using namespace ZXing;
using namespace testing;

namespace {
	static void check(int line, const Barcode& barcode, std::string_view symbologyIdentifier, std::string_view text, std::string_view bytes,
					  bool hasECI, std::string_view textECI, std::string_view bytesECI, std::string_view HRI, std::string_view contentType,
					  std::string_view position = {}, std::string_view ecLevel = {}, std::string_view version = {}, int dataMask = -1)
	{
		EXPECT_TRUE(barcode.isValid()) << "line:" << line;
		EXPECT_EQ(barcode.symbologyIdentifier(), symbologyIdentifier) << "line:" << line;
		EXPECT_EQ(barcode.text(TextMode::Plain), text) << "line:" << line;
		EXPECT_EQ(ToHex(barcode.bytes()), bytes) << "line:" << line;
		EXPECT_EQ(barcode.hasECI(), hasECI) << "line:" << line;
		EXPECT_EQ(barcode.text(TextMode::ECI), textECI) << "line:" << line;
		EXPECT_EQ(ToHex(barcode.bytesECI()), bytesECI) << "line:" << line;
		EXPECT_EQ(barcode.text(TextMode::HRI), HRI) << "line:" << line;
		EXPECT_EQ(ToString(barcode.contentType()), contentType) << "line:" << line;
		if (!position.empty())
			EXPECT_EQ(ToString(barcode.position()), position) << "line:" << line;
		EXPECT_EQ(barcode.ecLevel(), ecLevel) << "line:" << line;
		EXPECT_EQ(barcode.version(), version) << "line:" << line;
		if (dataMask != -1)
			EXPECT_EQ(JsonGetStr(barcode.extra(), "DataMask"), std::to_string(dataMask)) << "line:" << line;
	}

	static void check_same(int line, const Barcode& b1, const Barcode& b2,
						   bool cmpPosition = true, bool cmpBits = true, bool cmpEcLevel = true)
	{
		EXPECT_TRUE(b1.isValid()) << "line:" << line;
		EXPECT_TRUE(b2.isValid()) << "line:" << line;
		EXPECT_EQ(b1.symbologyIdentifier(), b2.symbologyIdentifier()) << "line:" << line;
		EXPECT_EQ(b1.text(TextMode::Plain), b2.text(TextMode::Plain)) << "line:" << line;
		EXPECT_EQ(ToHex(b1.bytes()), ToHex(b2.bytes())) << "line:" << line;
		EXPECT_EQ(b1.hasECI(), b2.hasECI()) << "line:" << line;
		EXPECT_EQ(b1.text(TextMode::ECI), b2.text(TextMode::ECI)) << "line:" << line;
		EXPECT_EQ(ToHex(b1.bytesECI()), ToHex(b2.bytesECI())) << "line:" << line;
		EXPECT_EQ(b1.text(TextMode::HRI), b2.text(TextMode::HRI)) << "line:" << line;
		EXPECT_EQ(ToString(b1.contentType()), ToString(b2.contentType())) << "line:" << line;
		if (cmpPosition)
			EXPECT_EQ(ToString(b1.position()), ToString(b2.position())) << "line:" << line;
		if (cmpEcLevel)
			EXPECT_EQ(b1.ecLevel(), b2.ecLevel()) << "line:" << line;
		EXPECT_EQ(b1.version(), b2.version()) << "line:" << line;
		EXPECT_EQ(b1.extra(), b2.extra()) << "line:" << line;

		EXPECT_EQ(b1.ECIs(), b2.ECIs()) << "line:" << line;
		EXPECT_EQ(b1.orientation(), b2.orientation()) << "line:" << line;
		EXPECT_EQ(b1.isMirrored(), b2.isMirrored()) << "line:" << line;
		EXPECT_EQ(b1.isInverted(), b2.isInverted()) << "line:" << line;
		EXPECT_EQ(b1.readerInit(), b2.readerInit()) << "line:" << line;
		if (cmpBits) {
			std::string b1BitsStr = ToString(b1.bits(), ' ', 'X', false /*addSpace*/); // Bits inverted
			std::string b2BitsStr = ToString(b2.bits(), ' ', 'X', false /*addSpace*/);
			EXPECT_EQ(b1BitsStr, b2BitsStr) << "line:" << line;
		}
	}

	static void check_x_position(int line, const Barcode& b1, const Barcode& b2)
	{
		Position p1 = b1.position();
		Position p2 = b2.position();
		EXPECT_EQ(p1.topLeft().x, p2.topLeft().x) << "line:" << line;
		EXPECT_EQ(p1.topRight().x, p2.topRight().x) << "line:" << line;
		EXPECT_EQ(p1.bottomRight().x, p2.bottomRight().x) << "line:" << line;
		EXPECT_EQ(p1.bottomLeft().x, p2.bottomLeft().x) << "line:" << line;
	}

	static std::string getImageStr(const Image& image)
	{
		std::ostringstream res;
		for (int y = 0; y < image.height(); y++) {
			for (int x = 0, w = image.width(); x < w; x++)
				res << (bool(*image.data(x, y)) ? ' ' : 'X');
			res << '\n';
		}
		return res.str();
	}
}

TEST(WriteBarcodeTest, ZintASCII)
{
	{
		BarcodeFormat format = BarcodeFormat::Aztec;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]z0", "1234", "31 32 33 34", false, "]z3\\0000261234", "5D 7A 30 31 32 33 34",
			  "1234", "Text", "0x0 15x0 15x15 0x15", "58%", "1" /*version*/);

		std::string bitsStr = ToString(barcode.bits(), ' ', 'X', false /*addSpace*/); // Bits inverted
		std::string expected_bitsStr =
"   XXX XXX XX  \n"
"XX  X XX   XXX \n"
"XXXX      X XXX\n"
"  XXXXXXXXXXX  \n"
" XXX       XXXX\n"
"X XX XXXXX XX  \n"
"X  X X   X XXXX\n"
"   X X X X XXXX\n"
"X  X X   X X X \n"
"X XX XXXXX XX  \n"
"XXXX       XX X\n"
" X XXXXXXXXXXX \n"
"XX    X X    X \n"
"     XX  XX  XX\n"
"X XX   XXXXXX  \n";
		EXPECT_EQ(bitsStr, expected_bitsStr);

		auto wOpts = WriterOptions().withQuietZones(false);
		Image image = WriteBarcodeToImage(barcode, wOpts);
		std::string imageStr = getImageStr(image);
		EXPECT_EQ(imageStr, bitsStr);

#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		Barcode readBarcode = ReadBarcode(image, rOpts);
		check_same(__LINE__, barcode, readBarcode); // TODO: CreateBarcode position hacked to agree
#endif

		std::string utf8 = WriteBarcodeToUtf8(barcode, wOpts);
		std::string expected_utf8 =
"▄▄ ▀█▀▄█▀▀ ██▄ \n"
"▀▀██▄▄▄▄▄▄█▄█▀▀\n"
"▄▀██ ▄▄▄▄▄ ██▀▀\n"
"▀  █ █ ▄ █ ████\n"
"█ ▄█ █▄▄▄█ █▄▀ \n"
"▀█▀█▄▄▄▄▄▄▄██▄▀\n"
"▀▀   ▄█ ▀▄▄  █▄\n"
"▀ ▀▀   ▀▀▀▀▀▀  \n";
		EXPECT_EQ(utf8, expected_utf8);

		std::string svg = WriteBarcodeToSVG(barcode, wOpts);
		std::string expected_svg = "<?xml version=\"1.0\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n<svg width=\"15\" height=\"15\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n <desc>Zint Generated Symbol</desc>\n <g id=\"barcode\" fill=\"#000000\">\n  <rect x=\"0\" y=\"0\" width=\"15\" height=\"15\" fill=\"#FFFFFF\"/>\n  <path d=\"M3 0h3v1h-3ZM7 0h3v1h-3ZM11 0h2v1h-2ZM0 1h2v1h-2ZM4 1h1v1h-1ZM6 1h2v1h-2ZM11 1h3v1h-3ZM0 2h4v1h-4ZM10 2h1v1h-1ZM12 2h3v1h-3ZM2 3h11v1h-11ZM1 4h3v1h-3ZM11 4h4v1h-4ZM0 5h1v2h-1ZM2 5h2v1h-2ZM5 5h5v1h-5ZM11 5h2v1h-2ZM3 6h1v3h-1ZM5 6h1v3h-1ZM9 6h1v3h-1ZM11 6h4v2h-4ZM7 7h1v1h-1ZM0 8h1v2h-1ZM11 8h1v1h-1ZM13 8h1v1h-1ZM2 9h2v1h-2ZM5 9h5v1h-5ZM11 9h2v2h-2ZM0 10h4v1h-4ZM14 10h1v1h-1ZM1 11h1v1h-1ZM3 11h11v1h-11ZM0 12h2v1h-2ZM6 12h1v1h-1ZM8 12h1v1h-1ZM13 12h1v1h-1ZM5 13h2v1h-2ZM9 13h2v1h-2ZM13 13h2v1h-2ZM0 14h1v1h-1ZM2 14h2v1h-2ZM7 14h6v1h-6Z\"/>\n </g>\n</svg>\n";
		EXPECT_EQ(svg, expected_svg);
	}
	{
		BarcodeFormat format = BarcodeFormat::Codabar;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("A12B", cOpts);
		check(__LINE__, barcode, "]F0", "A12B", "41 31 32 42", false, "]F0\\000026A12B", "5D 46 30 41 31 32 42",
			  "A12B", "Text", "0x0 40x0 40x49 0x49");

		std::string bitsStr = ToString(barcode.bits(), ' ', 'X', false /*addSpace*/); // Bits inverted
		std::string expected_bitsStr =
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n"
"X XX  X  X X X XX  X X X  X XX X  X  X XX \n";
		EXPECT_EQ(bitsStr, expected_bitsStr);

		auto wOpts = WriterOptions().withQuietZones(false);
		Image image = WriteBarcodeToImage(barcode, wOpts);
		std::string imageStr = getImageStr(image);
		EXPECT_EQ(imageStr, bitsStr);

#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		Barcode readBarcode = ReadBarcode(image, rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 40x25 40x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif

		std::string utf8 = WriteBarcodeToUtf8(barcode, wOpts);
		std::string expected_utf8 = "█ ██  █  █ █ █ ██  █ █ █  █ ██ █  █  █ ██ \n";
		EXPECT_EQ(utf8, expected_utf8);

		std::string svg = WriteBarcodeToSVG(barcode, wOpts);
		std::string expected_svg = "<?xml version=\"1.0\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n<svg width=\"42\" height=\"50\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n <desc>Zint Generated Symbol</desc>\n <g id=\"barcode\" fill=\"#000000\">\n  <rect x=\"0\" y=\"0\" width=\"42\" height=\"50\" fill=\"#FFFFFF\"/>\n  <path d=\"M0 0h1v50h-1ZM2 0h2v50h-2ZM6 0h1v50h-1ZM9 0h1v50h-1ZM11 0h1v50h-1ZM13 0h1v50h-1ZM15 0h2v50h-2ZM19 0h1v50h-1ZM21 0h1v50h-1ZM23 0h1v50h-1ZM26 0h1v50h-1ZM28 0h2v50h-2ZM31 0h1v50h-1ZM34 0h1v50h-1ZM37 0h1v50h-1ZM39 0h2v50h-2Z\"/>\n </g>\n</svg>\n";
		EXPECT_EQ(svg, expected_svg);
	}
	{
		BarcodeFormat format = BarcodeFormat::CodablockF;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]O4", "1234", "31 32 33 34", false, "]O4\\0000261234", "5D 4F 34 31 32 33 34",
			  "1234", "Text", "0x0 100x0 100x21 0x21");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]C0", "1234", "31 32 33 34", false, "]C0\\0000261234", "5D 43 30 31 32 33 34",
			  "1234", "Text", "0x0 56x0 56x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 56x25 56x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::Code16K;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]K0", "1234", "31 32 33 34", false, "]K0\\0000261234", "5D 4B 30 31 32 33 34",
			  "1234", "Text", "0x0 69x0 69x23 0x23");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// Plain (non-extended) Code 39
		BarcodeFormat format = BarcodeFormat::Code39;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]A0", "1234", "31 32 33 34", false, "]A0\\0000261234", "5D 41 30 31 32 33 34",
			  "1234", "Text", "0x0 76x0 76x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 76x25 76x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// Extended Code 39 with DEL
		BarcodeFormat format = BarcodeFormat::Code39;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("12\17734", cOpts);
		// HRI not escaped as content type considered "Text" (DEL not recognized)
		check(__LINE__, barcode, "]A4", "12\17734", "31 32 7F 33 34", false, "]A4\\00002612\17734", "5D 41 34 31 32 7F 33 34",
			  "12\17734", "Text", "0x0 102x0 102x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 102x25 102x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// Extended Code 39 with SOH & DEL
		BarcodeFormat format = BarcodeFormat::Code39;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("12\001\17734", cOpts);
		// HRI escaped as content type considered "Binary" (SOH)
		check(__LINE__, barcode, "]A4", "12\001\17734", "31 32 01 7F 33 34", false, "]A4\\00002612\001\17734",
			  "5D 41 34 31 32 01 7F 33 34", "12<SOH><DEL>34", "Binary", "0x0 128x0 128x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 128x25 128x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// Extended Code 39 with NUL
		BarcodeFormat format = BarcodeFormat::Code39;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		// HRI escaped as content type considered "Binary" (NUL)
		Barcode barcode = CreateBarcodeFromText(std::string("12\00034", 5), cOpts);
		check(__LINE__, barcode, "]A4", std::string("12\00034", 5), "31 32 00 33 34", false,
			  std::string("]A4\\00002612\00034", 15), "5D 41 34 31 32 00 33 34", "12<NUL>34", "Binary", "0x0 102x0 102x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 102x25 102x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::Code93;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]G0", "1234", "31 32 33 34", false, "]G0\\0000261234", "5D 47 30 31 32 33 34", "1234", "Text",
			  "0x0 72x0 72x39 0x39"); // Check digits removed
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x20 72x20 72x20 0x20");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DataBar;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]e0", "0100000000012348", "30 31 30 30 30 30 30 30 30 30 30 31 32 33 34 38", false,
			  "]e0\\0000260100000000012348", "5D 65 30 30 31 30 30 30 30 30 30 30 30 30 31 32 33 34 38", "(01)00000000012348",
			  "GS1", "1x0 96x0 96x32 1x32");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "1x16 96x16 96x16 1x16"); // TODO: CreateBarcode position hacked to agree
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DataBar; // Stacked

		auto cOpts = CreatorOptions(format, "stacked").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]e0", "0100000000012348", "30 31 30 30 30 30 30 30 30 30 30 31 32 33 34 38", false,
			  "]e0\\0000260100000000012348", "5D 65 30 30 31 30 30 30 30 30 30 30 30 30 31 32 33 34 38", "(01)00000000012348",
			  "GS1", "1x0 50x0 50x68 1x68");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "1x32 50x32 50x36 1x36"); // TODO: CreateBarcode position hacked to agree
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DataBarExpanded;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("(01)12345678901231(20)12(90)123(91)1234", cOpts);
		check(__LINE__, barcode, "]e0", "0112345678901231201290123\x1D" "911234",
			  "30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32 39 30 31 32 33 1D 39 31 31 32 33 34", false,
			  "]e0\\0000260112345678901231201290123\x1D" "911234",
			  "5D 65 30 30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32 39 30 31 32 33 1D 39 31 31 32 33 34",
			  "(01)12345678901231(20)12(90)123(91)1234", "GS1", "2x0 246x0 246x33 2x33");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "2x17 246x17 246x17 2x17"); // TODO: CreateBarcode position hacked to agree
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DataBarExpanded; // Stacked

		auto cOpts = CreatorOptions(format, "stacked").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("(01)12345678901231(20)12(90)123(91)1234", cOpts);
		check(__LINE__, barcode, "]e0", "0112345678901231201290123\x1D" "911234",
			  "30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32 39 30 31 32 33 1D 39 31 31 32 33 34", false,
			  "]e0\\0000260112345678901231201290123\x1D" "911234",
			  "5D 65 30 30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32 39 30 31 32 33 1D 39 31 31 32 33 34",
			  "(01)12345678901231(20)12(90)123(91)1234", "GS1", "0x0 102x0 102x108 0x108");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "2x33 50x33 50x75 2x75"); // TODO: hack position in CreateBarcode
		//check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DataBarLimited;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]e0", "0100000000012348", "30 31 30 30 30 30 30 30 30 30 30 31 32 33 34 38", false,
			  "]e0\\0000260100000000012348", "5D 65 30 30 31 30 30 30 30 30 30 30 30 30 31 32 33 34 38", "(01)00000000012348",
			  "GS1", "1x0 73x0 73x9 1x9");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "1x5 73x5 73x5 1x5");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DataMatrix;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]d1", "1234", "31 32 33 34", false, "]d4\\0000261234", "5D 64 31 31 32 33 34", "1234", "Text",
			  "0x0 9x0 9x9 0x9", "" /*ecLevel*/, "1" /*version*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DotCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]J0", "1234", "31 32 33 34", false, "]J3\\0000261234", "5D 4A 30 31 32 33 34", "1234", "Text",
			  "1x1 25x1 25x19 1x19", "" /*ecLevel*/, "" /*version*/, 3 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/); // TODO: position for DotCode
#endif
	}
	{
		// DX number only
		BarcodeFormat format = BarcodeFormat::DXFilmEdge;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "" /*si*/, "77-2", "37 37 2D 32", false, "\\00002677-2", "37 37 2D 32", "77-2", "Text",
			  "0x0 22x0 22x5 0x5");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x4 22x4 22x4 0x4");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// DX number + frame number
		BarcodeFormat format = BarcodeFormat::DXFilmEdge;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("77-2/XA", cOpts);
		check(__LINE__, barcode, "" /*si*/, "77-2/62A", "37 37 2D 32 2F 36 32 41", false, "\\00002677-2/62A",
			  "37 37 2D 32 2F 36 32 41", "77-2/62A", "Text", "0x0 30x0 30x5 0x5");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x4 30x4 30x4 0x4");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::EAN8;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("123456", cOpts);
		check(__LINE__, barcode, "]E4", "01234565", "30 31 32 33 34 35 36 35", false, "]E4\\00002601234565",
			  "5D 45 34 30 31 32 33 34 35 36 35", "01234565", "Text", "0x0 66x0 66x59 0x59");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x30 66x30 66x30 0x30");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::EAN13;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("123456789", cOpts);
		check(__LINE__, barcode, "]E0", "0001234567895", "30 30 30 31 32 33 34 35 36 37 38 39 35", false,
			  "]E0\\0000260001234567895", "5D 45 30 30 30 30 31 32 33 34 35 36 37 38 39 35", "0001234567895", "Text",
			  "0x0 94x0 94x73 0x73");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x37 94x37 94x37 0x37");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::HanXin;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]h0", "1234", "31 32 33 34", false, "]h1\\0000261234", "5D 68 30 31 32 33 34", "1234", "Text",
			  "0x0 22x0 22x22 0x22", "L4", "1" /*version*/, 2 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/); // TODO: position for HanXin
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::ITF;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]I0", "1234", "31 32 33 34", false, "]I0\\0000261234", "5D 49 30 31 32 33 34", "1234", "Text",
			  "0x0 44x0 44x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 44x25 44x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::MaxiCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]U0", "1234", "31 32 33 34", false, "]U2\\0000261234", "5D 55 30 31 32 33 34", "1234", "Text",
			  "0x0 148x0 148x132 0x132", "4" /*ecLevel*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::MicroPDF417;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]L2", "1234", "31 32 33 34", false, "]L1\\0000261234", "5D 4C 32 31 32 33 34", "1234", "Text",
			  "0x0 37x0 37x21 0x21", "64%" /*ecLevel*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, true /*cmpPosition*/, true /*cmpBits*/, false /*cmpEcLevel*/);
		EXPECT_EQ(readBarcode.ecLevel(), "58%"); // TODO: different EC level calc
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::MicroQRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]Q1", "1234", "31 32 33 34", false, "]Q2\\0000261234", "5D 51 31 31 32 33 34", "1234", "Text",
			  "0x0 10x0 10x10 0x10", "L", "1" /*version*/, 2 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::PDF417;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]L2", "1234", "31 32 33 34", false, "]L1\\0000261234", "5D 4C 32 31 32 33 34", "1234", "Text",
			  "0x0 102x0 102x17 0x17", "66%" /*ecLevel*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::QRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]Q1", "1234", "31 32 33 34", false, "]Q2\\0000261234", "5D 51 31 31 32 33 34", "1234", "Text",
			  "0x0 20x0 20x20 0x20", "H", "1" /*version*/, 6 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::RMQRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]Q1", "1234", "31 32 33 34", false, "]Q2\\0000261234", "5D 51 31 31 32 33 34", "1234", "Text",
			  "0x0 26x0 26x10 0x10", "H", "11" /*version*/, 4 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::UPCA;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]E0", "0000000012348", "30 30 30 30 30 30 30 30 31 32 33 34 38", false,
			  "]E0\\0000260000000012348", "5D 45 30 30 30 30 30 30 30 30 30 31 32 33 34 38", "0000000012348", "Text",
			  "0x0 94x0 94x73 0x73");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x37 94x37 94x37 0x37");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::UPCE;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234", cOpts);
		check(__LINE__, barcode, "]E0", "0000120000034", "30 30 30 30 31 32 30 30 30 30 30 33 34", false,
			  "]E0\\0000260000120000034", "5D 45 30 30 30 30 30 31 32 30 30 30 30 30 33 34", "0000120000034", "Text",
			  "0x0 50x0 50x73 0x73");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x37 50x37 50x37 0x37");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
}

TEST(WriteBarcodeTest, ZintEANUPCAddOn)
{
	{
		BarcodeFormat format = BarcodeFormat::EAN8;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("123456+01", cOpts);
		check(__LINE__, barcode, "]E3", "000000123456501", "30 30 30 30 30 30 31 32 33 34 35 36 35 30 31", false,
			  "]E3\\000026000000123456501", "5D 45 33 30 30 30 30 30 30 31 32 33 34 35 36 35 30 31", "000000123456501", "Text",
			  "0x0 93x0 93x59 0x59");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true).setEanAddOnSymbol(EanAddOnSymbol::Read);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x30 93x30 93x30 0x30");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::EAN13;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("123456789+12345", cOpts);
		check(__LINE__, barcode, "]E3", "000123456789512345", "30 30 30 31 32 33 34 35 36 37 38 39 35 31 32 33 34 35", false,
			  "]E3\\000026000123456789512345", "5D 45 33 30 30 30 31 32 33 34 35 36 37 38 39 35 31 32 33 34 35",
			  "000123456789512345", "Text", "0x0 148x0 148x73 0x73");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true).setEanAddOnSymbol(EanAddOnSymbol::Read);;
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x37 148x37 148x37 0x37");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::UPCA;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("123456789+12345", cOpts);
		check(__LINE__, barcode, "]E3", "000123456789512345", "30 30 30 31 32 33 34 35 36 37 38 39 35 31 32 33 34 35", false,
			  "]E3\\000026000123456789512345", "5D 45 33 30 30 30 31 32 33 34 35 36 37 38 39 35 31 32 33 34 35",
			  "000123456789512345", "Text", "0x0 150x0 150x73 0x73");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true).setEanAddOnSymbol(EanAddOnSymbol::Read);;
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x37 150x37 150x37 0x37");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::UPCE;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234+01", cOpts);
		check(__LINE__, barcode, "]E3", "000012000003401", "30 30 30 30 31 32 30 30 30 30 30 33 34 30 31", false,
			  "]E3\\000026000012000003401", "5D 45 33 30 30 30 30 31 32 30 30 30 30 30 33 34 30 31", "000012000003401", "Text",
			  "0x0 77x0 77x73 0x73");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true).setEanAddOnSymbol(EanAddOnSymbol::Read);;
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x37 77x37 77x37 0x37");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::UPCE;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234 01", cOpts);
		check(__LINE__, barcode, "]E3", "000012000003401", "30 30 30 30 31 32 30 30 30 30 30 33 34 30 31", false,
			  "]E3\\000026000012000003401", "5D 45 33 30 30 30 30 31 32 30 30 30 30 30 33 34 30 31", "000012000003401", "Text",
			  "0x0 77x0 77x73 0x73");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true).setEanAddOnSymbol(EanAddOnSymbol::Read);;
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x37 77x37 77x37 0x37");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
}

TEST(WriteBarcodeTest, ZintISO8859_1)
{
	{
		// Control chars (SOH & DEL)
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("12\001\17734", cOpts);
		check(__LINE__, barcode, "]C0", "12\001\17734", "31 32 01 7F 33 34", false, "]C0\\00002612\001\17734",
			  "5D 43 30 31 32 01 7F 33 34", "12<SOH><DEL>34", "Binary");
#ifdef ZXING_READERS
		auto wOpts = WriterOptions().withQuietZones(false);
		Image image = WriteBarcodeToImage(barcode, wOpts);
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		Barcode readBarcode = ReadBarcode(image, rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// NUL
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText(std::string("12\00034", 5), cOpts);
		check(__LINE__, barcode, "]C0", std::string("12\00034", 5), "31 32 00 33 34", false,
			  std::string("]C0\\00002612\00034", 15), "5D 43 30 31 32 00 33 34", "12<NUL>34", "Binary");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// Latin-1 (Extended ASCII)
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("12é34", cOpts);
		check(__LINE__, barcode, "]C0", "12é34", "31 32 E9 33 34", false, "]C0\\00002612é34", "5D 43 30 31 32 E9 33 34",
			  "12é34", "Text");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// Control char & Latin-1
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("\007Ç", cOpts);
		check(__LINE__, barcode, "]C0", "\007Ç", "07 C7", false, "]C0\\000026\007Ç", "5D 43 30 07 C7", "<BEL>Ç", "Binary");

		auto wOpts = WriterOptions().withQuietZones(false);
		wOpts.scale(2).withHRT(true); // Scale 2 to get HRT to appear
		Image image = WriteBarcodeToImage(barcode, wOpts);
		std::string imageStr = getImageStr(image);
		std::string expected_imageStr =
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"XXXX  XX        XX    XX    XXXX  XX        XXXXXX  XX  XXXXXXXX  XXXX  XX      XX      XX    XX  XXXXXXXX    XXXX      XXXXXX  XX  XXXX\n"
"                                                                                                                                        \n"
"                                                                                                                                        \n"
"                                                                                                                                        \n"
"                                                                                                                                        \n"
"                                                                     XXXX                                                               \n"
"                                                                    X    X                                                              \n"
"                                                                    X    X                                                              \n"
"                                                                    X                                                                   \n"
"                                                                    X                                                                   \n"
"                                                                    X                                                                   \n"
"                                                                    X                                                                   \n"
"                                                                    X    X                                                              \n"
"                                                                    X    X                                                              \n"
"                                                                     XXXX                                                               \n"
"                                                                       X                                                                \n"
"                                                                      X                                                                 \n";
		check(__LINE__, barcode, "]C0", "\007Ç", "07 C7", false, "]C0\\000026\007Ç", "5D 43 30 07 C7", "<BEL>Ç", "Binary");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		Barcode readBarcode = ReadBarcode(image, rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/, false /*cmpBits*/); // Bits won't compare (scaling/HRT)
		//check_x_position(__LINE__, barcode, readBarcode); // TODO: check, allowing for scaling & HRT
#endif
	}
	{
		// No ECI
		BarcodeFormat format = BarcodeFormat::Aztec;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]z0", "1234é", "31 32 33 34 E9", false, "]z3\\0000261234é", "5D 7A 30 31 32 33 34 E9",
			  "1234é", "Text", "" /*position*/, "35%" /*ecLevel*/, "1" /*version*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI
		BarcodeFormat format = BarcodeFormat::Aztec;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::ISO8859_1);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]z3", "1234é", "31 32 33 34 E9", true, "]z3\\0000261234é",
			  "5D 7A 33 5C 30 30 30 30 30 33 31 32 33 34 E9", "1234é", "Text", "" /*position*/, "17%", "1");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// No ECI
		BarcodeFormat format = BarcodeFormat::DataMatrix;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]d1", "1234é", "31 32 33 34 E9", false, "]d4\\0000261234é", "5D 64 31 31 32 33 34 E9",
			  "1234é", "Text", "" /*position*/, "" /*ecLevel*/, "2" /*version*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI
		BarcodeFormat format = BarcodeFormat::DataMatrix;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::ISO8859_1);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]d4", "1234é", "31 32 33 34 E9", true, "]d4\\0000261234é",
			  "5D 64 34 5C 30 30 30 30 30 33 31 32 33 34 E9", "1234é", "Text", "" /*position*/, "" /*ecLevel*/, "3");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// No ECI
		BarcodeFormat format = BarcodeFormat::MaxiCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]U0", "1234é", "31 32 33 34 E9", false, "]U2\\0000261234é", "5D 55 30 31 32 33 34 E9",
			  "1234é", "Text", "" /*position*/, "4" /*ecLevel*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI
		BarcodeFormat format = BarcodeFormat::MaxiCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::ISO8859_1);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]U2", "1234é", "31 32 33 34 E9", true, "]U2\\0000261234é",
			  "5D 55 32 5C 30 30 30 30 30 33 31 32 33 34 E9", "1234é", "Text", "" /*position*/, "4" /*ecLevel*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// No ECI
		BarcodeFormat format = BarcodeFormat::PDF417;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]L2", "1234é", "31 32 33 34 E9", false, "]L1\\0000261234é", "5D 4C 32 31 32 33 34 E9",
			  "1234é", "Text", "" /*position*/, "57%");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI
		BarcodeFormat format = BarcodeFormat::PDF417;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::ISO8859_1);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]L1", "1234é", "31 32 33 34 E9", true, "]L1\\0000261234é",
			  "5D 4C 31 5C 30 30 30 30 30 33 31 32 33 34 E9", "1234é", "Text", "" /*position*/, "50%");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// No ECI
		BarcodeFormat format = BarcodeFormat::QRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]Q1", "1234é", "31 32 33 34 E9", false, "]Q2\\0000261234é", "5D 51 31 31 32 33 34 E9",
			  "1234é", "Text", "0x0 20x0 20x20 0x20", "H", "1", 0 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI
		BarcodeFormat format = BarcodeFormat::QRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::ISO8859_1);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]Q2", "1234é", "31 32 33 34 E9", true, "]Q2\\0000261234é",
			  "5D 51 32 5C 30 30 30 30 30 33 31 32 33 34 E9", "1234é", "Text", "0x0 20x0 20x20 0x20", "H", "1", 1 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// No ECI
		BarcodeFormat format = BarcodeFormat::RMQRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]Q1", "1234é", "31 32 33 34 E9", false, "]Q2\\0000261234é", "5D 51 31 31 32 33 34 E9",
			  "1234é", "Text", "0x0 26x0 26x10 0x10", "H", "11", 4 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI
		BarcodeFormat format = BarcodeFormat::RMQRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::ISO8859_1);
		Barcode barcode = CreateBarcodeFromText("1234é", cOpts);
		check(__LINE__, barcode, "]Q2", "1234é", "31 32 33 34 E9", true, "]Q2\\0000261234é",
			  "5D 51 32 5C 30 30 30 30 30 33 31 32 33 34 E9", "1234é", "Text", "0x0 26x0 26x10 0x10", "M", "11", 4 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
}

TEST(WriteBarcodeTest, ZintBinary)
{
	{
		// No ECI (not supported anyway for Code128)
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		std::string data("\x00\x5C\x7E\x7F\x80\x81\xC0\xFF", 8);
		Barcode barcode = CreateBarcodeFromBytes(data, cOpts);
		check(__LINE__, barcode, "]C0",
			  std::string("\x00\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 12) /*TextMode::Plain - UTF-8*/,
			  "00 5C 7E 7F 80 81 C0 FF" /*bytes() - binary*/, false,
			  std::string("]C0\\000026\x00\x5C\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 23) /*TextMode::ECI - UTF-8*/,
			  "5D 43 30 00 5C 7E 7F 80 81 C0 FF" /*bytesECI() - binary*/,
			  "<NUL>\\~<DEL><U+80><U+81>Àÿ" /*TextMode::HRI - UTF-8*/,
			  "Binary", "0x0 177x0 177x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 177x25 177x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI (ignored as Code128 doesn't support ECI)
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::Binary);
		std::string data("\x00\x5C\x7E\x7F\x80\x81\xC0\xFF", 8);
		Barcode barcode = CreateBarcodeFromBytes(data, cOpts);
		check(__LINE__, barcode, "]C0",
			  std::string("\x00\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 12) /*TextMode::Plain - UTF-8*/,
			  "00 5C 7E 7F 80 81 C0 FF" /*bytes() - binary*/, false,
			  std::string("]C0\\000026\x00\x5C\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 23) /*TextMode::ECI - UTF-8*/,
			  "5D 43 30 00 5C 7E 7F 80 81 C0 FF" /*bytesECI() - binary*/,
			  "<NUL>\\~<DEL><U+80><U+81>Àÿ" /*TextMode::HRI - UTF-8*/,
			  "Binary", "0x0 177x0 177x49 0x49");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x25 177x25 177x25 0x25");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// No ECI
		BarcodeFormat format = BarcodeFormat::QRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		std::string data("\x00\x5C\x7E\x7F\x80\x81\xC0\xFF", 8);
		Barcode barcode = CreateBarcodeFromBytes(data, cOpts);
		check(__LINE__, barcode, "]Q1",
			  std::string("\x00\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 12) /*TextMode::Plain - UTF-8*/,
			  "00 5C 7E 7F 80 81 C0 FF" /*bytes() - binary*/, false,
			  std::string("]Q2\\000026\x00\x5C\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 23) /*TextMode::ECI - UTF-8*/,
			  "5D 51 31 00 5C 7E 7F 80 81 C0 FF" /*bytesECI() - binary*/,
			  "<NUL>\\~<DEL><U+80><U+81>Àÿ" /*TextMode::HRI - UTF-8*/,
			  "Binary", "0x0 20x0 20x20 0x20", "Q", "1", 0 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		// With ECI::Binary
		BarcodeFormat format = BarcodeFormat::QRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		cOpts.eci(ECI::Binary);
		std::string data("\x00\x5C\x7E\x7F\x80\x81\xC0\xFF", 8);
		Barcode barcode = CreateBarcodeFromBytes(data, cOpts);
		check(__LINE__, barcode, "]Q2",
			  std::string("\x00\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 12) /*TextMode::Plain - UTF-8*/,
			  "00 5C 7E 7F 80 81 C0 FF" /*bytes() - binary*/, true,
			  std::string("]Q2\\000899\x00\x5C\x5C\x7E\x7F\xC2\x80\xC2\x81\xC3\x80\xC3\xBF", 23) /*TextMode::ECI - UTF-8*/,
			  "5D 51 32 5C 30 30 30 38 39 39 00 5C 5C 7E 7F 80 81 C0 FF" /*bytesECI() - binary*/,
			  "<NUL>\\~<DEL><U+80><U+81>Àÿ" /*TextMode::HRI - UTF-8*/,
			  "Binary", "0x0 20x0 20x20 0x20", "Q", "1", 3 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
}

TEST(WriteBarcodeTest, ZintECI)
{
	{
		// ECI automatically added by zint
		BarcodeFormat format = BarcodeFormat::QRCode;

		auto cOpts = CreatorOptions(format).withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("AกC", cOpts); // Will add ECI 13 (ISO/IEC 8859-11 Thai)
		check(__LINE__, barcode, "]Q2", "AกC", "41 A1 43", true,
			  "]Q2\\000026AกC", "5D 51 32 5C 30 30 30 30 31 33 41 A1 43" /*ECI 13*/,
			  "AกC", "Text", "0x0 20x0 20x20 0x20", "H", "1", 4 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
}

TEST(WriteBarcodeTest, ZintGS1)
{
	{
		BarcodeFormat format = BarcodeFormat::Aztec;

		auto cOpts = CreatorOptions(format, "GS1").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("(01)12345678901231(20)12", cOpts);
		check(__LINE__, barcode, "]z1", "01123456789012312012", "30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  false, "]z4\\00002601123456789012312012", "5D 7A 31 30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  "(01)12345678901231(20)12", "GS1", "0x0 19x0 19x19 0x19", "50%", "2" /*version*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode); // TODO: CreateBarcode position hacked to agree
		EXPECT_EQ(readBarcode.ecLevel(), "50%"); // Happens to agree with CreateBarcode
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::Code128;

		auto cOpts = CreatorOptions(format, "GS1").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("[01]12345678901231[20]12", cOpts);
		check(__LINE__, barcode, "]C1", "01123456789012312012", "30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  false, "]C1\\00002601123456789012312012", "5D 43 31 30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  "(01)12345678901231(20)12", "GS1", "0x0 155x0 155x63 0x63");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/);
		EXPECT_EQ(ToString(readBarcode.position()), "0x32 155x32 155x32 0x32");
		check_x_position(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::Code16K;

		auto cOpts = CreatorOptions(format, "GS1").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("[01]04912345123459[15]970331[30]128[10]ABC123", cOpts);
		check(__LINE__, barcode, "]K1", "01049123451234591597033130128\x1D" "10ABC123",
			  "30 31 30 34 39 31 32 33 34 35 31 32 33 34 35 39 31 35 39 37 30 33 33 31 33 30 31 32 38 1D 31 30 41 42 43 31 32 33",
			  false, "]K1\\00002601049123451234591597033130128\x1D" "10ABC123",
			  "5D 4B 31 30 31 30 34 39 31 32 33 34 35 31 32 33 34 35 39 31 35 39 37 30 33 33 31 33 30 31 32 38 1D 31 30 41 42 43 31 32 33",
			  "(01)04912345123459(15)970331(30)128(10)ABC123", "GS1", "0x0 69x0 69x67 0x67");
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DataMatrix;

		auto cOpts = CreatorOptions(format, "GS1").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("[01]12345678901231[20]12", cOpts);
		check(__LINE__, barcode, "]d2", "01123456789012312012", "30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  false, "]d5\\00002601123456789012312012", "5D 64 32 30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  "(01)12345678901231(20)12", "GS1", "0x0 15x0 15x15 0x15", "" /*ecLevel*/, "4" /*version*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::DotCode;

		auto cOpts = CreatorOptions(format, "GS1").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("[01]00012345678905[17]201231[10]ABC123456", cOpts);
		check(__LINE__, barcode, "]J1", "01000123456789051720123110ABC123456",
			  "30 31 30 30 30 31 32 33 34 35 36 37 38 39 30 35 31 37 32 30 31 32 33 31 31 30 41 42 43 31 32 33 34 35 36",
			  false, "]J4\\00002601000123456789051720123110ABC123456",
			  "5D 4A 31 30 31 30 30 30 31 32 33 34 35 36 37 38 39 30 35 31 37 32 30 31 32 33 31 31 30 41 42 43 31 32 33 34 35 36",
			  "(01)00012345678905(17)201231(10)ABC123456", "GS1", "1x1 57x1 57x39 1x39", "" /*ecLevel*/, "" /*version*/,
			  1 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode, false /*cmpPosition*/); // TODO: position for DotCode
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::QRCode;

		auto cOpts = CreatorOptions(format, "GS1").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("[01]12345678901231[20]12", cOpts);
		check(__LINE__, barcode, "]Q3", "01123456789012312012", "30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  false, "]Q4\\00002601123456789012312012", "5D 51 33 30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  "(01)12345678901231(20)12", "GS1", "0x0 20x0 20x20 0x20", "Q", "1", 7 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
	{
		BarcodeFormat format = BarcodeFormat::RMQRCode;

		auto cOpts = CreatorOptions(format, "GS1").withQuietZones(false);
		Barcode barcode = CreateBarcodeFromText("[01]12345678901231[20]12", cOpts);
		check(__LINE__, barcode, "]Q3", "01123456789012312012", "30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  false, "]Q4\\00002601123456789012312012", "5D 51 33 30 31 31 32 33 34 35 36 37 38 39 30 31 32 33 31 32 30 31 32",
			  "(01)12345678901231(20)12", "GS1", "0x0 26x0 26x12 0x12", "M", "17", 4 /*dataMask*/);
#ifdef ZXING_READERS
		auto rOpts = ReaderOptions().setFormats(format).setIsPure(true);
		auto wOpts = WriterOptions().withQuietZones(false);
		Barcode readBarcode = ReadBarcode(WriteBarcodeToImage(barcode, wOpts), rOpts);
		check_same(__LINE__, barcode, readBarcode);
#endif
	}
}

#ifdef ZXING_READERS
TEST(WriteBarcodeTest, RandomDataBar)
{
	auto randomTest = [](BarcodeFormat format) {
		auto read_opts = ReaderOptions().setFormats(format).setIsPure(true).setBinarizer(Binarizer::BoolCast);

		int n = 1000;
		int nErrors = 0;
		for (int i = 0; i < n; i += 1) {
			auto input = ToString(rand(), 13);
			input = "(01)" + input + GTIN::ComputeCheckDigit(input);
			auto bc = CreateBarcodeFromText(input, format);
			auto br = ReadBarcode(bc.symbol(), read_opts);

			nErrors += !br.isValid() || bc.text(TextMode::HRI) != input;
		}
		EXPECT_EQ(nErrors, 0) << std::fixed << std::setw(4) << std::setprecision(2) << "(Error rate of " << ToString(format) << " is "
							  << nErrors * 100 / (double)n << "%)";
	};

	randomTest(BarcodeFormat::DataBar);
	randomTest(BarcodeFormat::DataBarLimited);
	randomTest(BarcodeFormat::DataBarExpanded);
}
#endif

#endif // #if defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_WRITERS) && defined(ZXING_USE_ZINT)
