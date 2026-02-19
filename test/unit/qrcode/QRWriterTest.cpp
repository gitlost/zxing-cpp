/*
* Copyright 2017 Huy Cuong Nguyen
* Copyright 2008 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ZXingCpp.h"

#include "BitMatrixIO.h"
#ifdef ZXING_USE_ZINT
#include "JSON.h"
#else
#include "MultiFormatWriter.h"
#endif
#include "qrcode/QRWriter.h"
#include "qrcode/QRErrorCorrectionLevel.h"

#include "gtest/gtest.h"
#include <stdexcept>

using namespace ZXing;
using namespace ZXing::QRCode;

namespace {

	void DoTest(const std::wstring& contents, ErrorCorrectionLevel ecLevel, int resolution, const char* expected) {
		Writer writer;
		writer.setErrorCorrectionLevel(ecLevel);
		auto matrix = writer.encode(contents, resolution, resolution);
		auto actual = ToString(matrix, 'X', ' ', true);
		EXPECT_EQ(matrix.width(), resolution);
		EXPECT_EQ(matrix.height(), resolution);
		EXPECT_EQ(actual, expected);
	}

}

TEST(QRWriterTest, OverSize)
{
	// The QR should be multiplied up to fit, with extra padding if necessary
	int bigEnough = 256;
	Writer writer;
	BitMatrix matrix = writer.encode(L"http://www.google.com/", bigEnough, bigEnough);
	EXPECT_EQ(matrix.width(), bigEnough);
	EXPECT_EQ(matrix.height(), bigEnough);

	// The QR will not fit in this size, so the matrix should come back bigger
	int tooSmall = 20;
	matrix = writer.encode(L"http://www.google.com/", tooSmall, tooSmall);
	EXPECT_GT(matrix.width(), tooSmall);
	EXPECT_GT(matrix.height(), tooSmall);

	// We should also be able to handle non-square requests by padding them
	int strangeWidth = 500;
	int strangeHeight = 100;
	matrix = writer.encode(L"http://www.google.com/", strangeWidth, strangeHeight);
	EXPECT_EQ(matrix.width(), strangeWidth);
	EXPECT_EQ(matrix.height(), strangeHeight);

	int overflowSize = 0x1000000;
	EXPECT_THROW(writer.encode(L"http://www.google.com/", overflowSize, overflowSize), std::invalid_argument);
}

TEST(QRWriterTest, RegressionTest)
{
	DoTest(L"http://www.google.com/", ErrorCorrectionLevel::Medium, 99,
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X                   X X X       X X X X X X             X X X X X X X X X X X X X X X X X X X X X                         \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X                   X X X       X X X X X X             X X X X X X X X X X X X X X X X X X X X X                         \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X                   X X X       X X X X X X             X X X X X X X X X X X X X X X X X X X X X                         \n"
		"                        X X X                               X X X             X X X X X X       X X X                   X X X       X X X                               X X X                         \n"
		"                        X X X                               X X X             X X X X X X       X X X                   X X X       X X X                               X X X                         \n"
		"                        X X X                               X X X             X X X X X X       X X X                   X X X       X X X                               X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X                   X X X X X X X X X             X X X             X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X                   X X X X X X X X X             X X X             X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X                   X X X X X X X X X             X X X             X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X             X X X       X X X X X X X X X X X X       X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X             X X X       X X X X X X X X X X X X       X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X             X X X       X X X X X X X X X X X X       X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X                   X X X X X X X X X X X X X X X       X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X                   X X X X X X X X X X X X X X X       X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X                   X X X X X X X X X X X X X X X       X X X       X X X X X X X X X       X X X                         \n"
		"                        X X X                               X X X       X X X       X X X X X X X X X       X X X                   X X X                               X X X                         \n"
		"                        X X X                               X X X       X X X       X X X X X X X X X       X X X                   X X X                               X X X                         \n"
		"                        X X X                               X X X       X X X       X X X X X X X X X       X X X                   X X X                               X X X                         \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X       X X X       X X X       X X X       X X X       X X X X X X X X X X X X X X X X X X X X X                         \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X       X X X       X X X       X X X       X X X       X X X X X X X X X X X X X X X X X X X X X                         \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X       X X X       X X X       X X X       X X X       X X X X X X X X X X X X X X X X X X X X X                         \n"
		"                                                                        X X X       X X X                   X X X X X X                                                                               \n"
		"                                                                        X X X       X X X                   X X X X X X                                                                               \n"
		"                                                                        X X X       X X X                   X X X X X X                                                                               \n"
		"                        X X X                   X X X       X X X X X X X X X X X X       X X X                   X X X       X X X X X X X X X X X X X X X             X X X                         \n"
		"                        X X X                   X X X       X X X X X X X X X X X X       X X X                   X X X       X X X X X X X X X X X X X X X             X X X                         \n"
		"                        X X X                   X X X       X X X X X X X X X X X X       X X X                   X X X       X X X X X X X X X X X X X X X             X X X                         \n"
		"                              X X X X X X X X X       X X X             X X X             X X X X X X       X X X X X X X X X X X X             X X X X X X       X X X                               \n"
		"                              X X X X X X X X X       X X X             X X X             X X X X X X       X X X X X X X X X X X X             X X X X X X       X X X                               \n"
		"                              X X X X X X X X X       X X X             X X X             X X X X X X       X X X X X X X X X X X X             X X X X X X       X X X                               \n"
		"                                    X X X                   X X X       X X X       X X X       X X X X X X X X X X X X                   X X X X X X X X X X X X                                     \n"
		"                                    X X X                   X X X       X X X       X X X       X X X X X X X X X X X X                   X X X X X X X X X X X X                                     \n"
		"                                    X X X                   X X X       X X X       X X X       X X X X X X X X X X X X                   X X X X X X X X X X X X                                     \n"
		"                              X X X X X X       X X X X X X                         X X X       X X X       X X X                   X X X X X X             X X X X X X                               \n"
		"                              X X X X X X       X X X X X X                         X X X       X X X       X X X                   X X X X X X             X X X X X X                               \n"
		"                              X X X X X X       X X X X X X                         X X X       X X X       X X X                   X X X X X X             X X X X X X                               \n"
		"                        X X X       X X X       X X X X X X X X X                   X X X                                           X X X             X X X X X X X X X X X X                         \n"
		"                        X X X       X X X       X X X X X X X X X                   X X X                                           X X X             X X X X X X X X X X X X                         \n"
		"                        X X X       X X X       X X X X X X X X X                   X X X                                           X X X             X X X X X X X X X X X X                         \n"
		"                        X X X       X X X                         X X X X X X X X X X X X       X X X       X X X X X X X X X                   X X X             X X X                               \n"
		"                        X X X       X X X                         X X X X X X X X X X X X       X X X       X X X X X X X X X                   X X X             X X X                               \n"
		"                        X X X       X X X                         X X X X X X X X X X X X       X X X       X X X X X X X X X                   X X X             X X X                               \n"
		"                                    X X X                   X X X X X X X X X       X X X X X X X X X       X X X X X X             X X X X X X X X X X X X X X X                                     \n"
		"                                    X X X                   X X X X X X X X X       X X X X X X X X X       X X X X X X             X X X X X X X X X X X X X X X                                     \n"
		"                                    X X X                   X X X X X X X X X       X X X X X X X X X       X X X X X X             X X X X X X X X X X X X X X X                                     \n"
		"                                                X X X             X X X X X X             X X X             X X X             X X X X X X X X X       X X X X X X X X X                               \n"
		"                                                X X X             X X X X X X             X X X             X X X             X X X X X X X X X       X X X X X X X X X                               \n"
		"                                                X X X             X X X X X X             X X X             X X X             X X X X X X X X X       X X X X X X X X X                               \n"
		"                        X X X X X X       X X X             X X X                               X X X       X X X X X X X X X X X X X X X X X X X X X X X X X X X                                     \n"
		"                        X X X X X X       X X X             X X X                               X X X       X X X X X X X X X X X X X X X X X X X X X X X X X X X                                     \n"
		"                        X X X X X X       X X X             X X X                               X X X       X X X X X X X X X X X X X X X X X X X X X X X X X X X                                     \n"
		"                                                                        X X X X X X X X X                   X X X       X X X                   X X X       X X X                                     \n"
		"                                                                        X X X X X X X X X                   X X X       X X X                   X X X       X X X                                     \n"
		"                                                                        X X X X X X X X X                   X X X       X X X                   X X X       X X X                                     \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X       X X X             X X X X X X       X X X       X X X       X X X X X X                                           \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X       X X X             X X X X X X       X X X       X X X       X X X X X X                                           \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X       X X X             X X X X X X       X X X       X X X       X X X X X X                                           \n"
		"                        X X X                               X X X             X X X X X X       X X X       X X X       X X X                   X X X X X X X X X X X X X X X                         \n"
		"                        X X X                               X X X             X X X X X X       X X X       X X X       X X X                   X X X X X X X X X X X X X X X                         \n"
		"                        X X X                               X X X             X X X X X X       X X X       X X X       X X X                   X X X X X X X X X X X X X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X             X X X       X X X X X X       X X X X X X X X X X X X X X X       X X X X X X X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X             X X X       X X X X X X       X X X X X X X X X X X X X X X       X X X X X X X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X       X X X             X X X       X X X X X X       X X X X X X X X X X X X X X X       X X X X X X X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X             X X X             X X X       X X X X X X X X X X X X X X X             X X X X X X X X X X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X             X X X             X X X       X X X X X X X X X X X X X X X             X X X X X X X X X X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X             X X X             X X X       X X X X X X X X X X X X X X X             X X X X X X X X X X X X                         \n"
		"                        X X X       X X X X X X X X X       X X X             X X X       X X X X X X X X X X X X             X X X X X X X X X                   X X X                               \n"
		"                        X X X       X X X X X X X X X       X X X             X X X       X X X X X X X X X X X X             X X X X X X X X X                   X X X                               \n"
		"                        X X X       X X X X X X X X X       X X X             X X X       X X X X X X X X X X X X             X X X X X X X X X                   X X X                               \n"
		"                        X X X                               X X X                   X X X X X X       X X X       X X X                   X X X X X X X X X X X X X X X                               \n"
		"                        X X X                               X X X                   X X X X X X       X X X       X X X                   X X X X X X X X X X X X X X X                               \n"
		"                        X X X                               X X X                   X X X X X X       X X X       X X X                   X X X X X X X X X X X X X X X                               \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X X X X X X X       X X X X X X       X X X       X X X                         X X X X X X X X X                         \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X X X X X X X       X X X X X X       X X X       X X X                         X X X X X X X X X                         \n"
		"                        X X X X X X X X X X X X X X X X X X X X X       X X X X X X X X X       X X X X X X       X X X       X X X                         X X X X X X X X X                         \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
		"                                                                                                                                                                                                      \n"
	);
}

TEST(QRWriterTest, LibreOfficeQrCodeGenDialog)
{
	{
		// Following similar to `GenerateQRCode()` in "cui/source/dialogs/QrCodeGenDialog.cxx" at
		//   https://github.com/LibreOffice/core
		int bqrEcc = 1;
		int aQRBorder = 1;
		// Shortened version of "samples/qrcode-2/29.png"
		// "MEBKM:TITLE:hypeモバイル;URL:http\://live.fdgm.jp/u/event/hype/hype_top.html;;"
		std::string QRText("MEBKM:TITLE:hype\u30E2\u30D0\u30A4\u30EB;URL:http\\://live.fdgm.jp/u/event/hype/hype_top.html;;");
		BarcodeFormat format = BarcodeFormat::QRCode;
#ifdef ZXING_USE_ZINT
		std::string options = JsonProp("eci", "UTF8") + JsonProp("ecLevel", 1) + JsonProp("margin", aQRBorder);
		auto cOpts = CreatorOptions(format, options);
		auto barcode = CreateBarcodeFromText(QRText, cOpts);
		auto actual = ToString(barcode.symbolMatrix(), ' ', 'X', true); // Bits inverted
		EXPECT_EQ(actual, // Different encodation than non-Zint
"                                                                              \n"
"  X X X X X X X   X   X   X X X X   X X X           X   X X   X X X X X X X   \n"
"  X           X     X X X X X     X   X       X   X       X   X           X   \n"
"  X   X X X   X   X X X           X       X       X X X X     X   X X X   X   \n"
"  X   X X X   X   X X X   X   X X     X X   X X   X     X     X   X X X   X   \n"
"  X   X X X   X   X     X X   X   X       X X       X X       X   X X X   X   \n"
"  X           X       X X   X   X X   X X X X   X X   X X     X           X   \n"
"  X X X X X X X   X   X   X   X   X   X   X   X   X   X   X   X X X X X X X   \n"
"                      X   X     X X             X X X X                       \n"
"  X X X X     X   X   X X X X   X   X         X X   X   X X X     X X X   X   \n"
"  X     X X           X   X X X X   X X X X X   X   X   X X X X X   X X       \n"
"  X X     X X X       X X X X     X     X     X     X         X   X   X X     \n"
"        X   X   X     X           X X X X X X     X     X X                   \n"
"  X X     X   X           X   X X   X         X X X         X X X X       X   \n"
"  X   X X X X     X     X X   X   X     X X X   X   X X X     X X X X     X   \n"
"  X   X     X X X X X   X   X   X X X X     X   X     X X X     X   X X X     \n"
"    X X   X       X X   X     X   X X X   X                   X       X   X   \n"
"    X X X X   X X X X     X   X X   X   X X X         X X       X   X   X     \n"
"  X   X   X       X   X       X   X X   X     X   X     X   X X       X       \n"
"  X X X       X             X X X           X X X X X   X X   X     X X X X   \n"
"            X   X X X X X   X X       X X   X X X X X   X X X X X   X X X     \n"
"    X   X     X X     X     X X   X X X     X   X   X     X       X     X X   \n"
"  X   X   X X   X X               X X X X                 X X X   X           \n"
"              X   X   X       X   X   X       X   X X     X   X   X X     X   \n"
"  X     X X X   X   X X X X X X X         X X X   X X X           X X X X X   \n"
"  X X X     X X       X X       X     X X         X   X X X X X     X     X   \n"
"    X X     X   X   X X X   X   X X X     X X         X X X   X   X X   X X   \n"
"    X         X X X   X   X   X       X X X X   X   X X   X     X X   X X     \n"
"  X   X X   X   X         X X     X X     X         X X X     X X X X X   X   \n"
"        X     X   X X   X           X X X X   X     X     X X X X X     X     \n"
"                  X X     X X     X X     X X X   X X   X X       X       X   \n"
"  X X X X X X X     X X X X       X     X X X X X X   X   X   X   X X     X   \n"
"  X           X     X X   X       X X X     X     X X   X X       X X X X X   \n"
"  X   X X X   X     X   X X X   X X     X   X X X     X   X X X X X   X   X   \n"
"  X   X X X   X   X X     X X X     X X         X X X   X   X X   X   X X X   \n"
"  X   X X X   X   X     X X X   X X   X X X   X X X     X X       X X   X     \n"
"  X           X   X X X             X     X X     X   X X X X     X   X X X   \n"
"  X X X X X X X   X       X   X       X X   X X X X     X X   X X X X   X X   \n"
"                                                                              \n"
		);
#else
#ifdef _MSC_VER
#pragma warning(suppress : 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
		auto writer = MultiFormatWriter(format).setMargin(aQRBorder).setEccLevel(bqrEcc);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		writer.setEncoding(CharacterSet::UTF8);
		BitMatrix bitmatrix = writer.encode(QRText, 0, 0);
		auto actual = ToString(bitmatrix, 'X', ' ', true);
		EXPECT_EQ(actual,
"                                                                              \n"
"  X X X X X X X       X   X X X X       X X X X X             X X X X X X X   \n"
"  X           X   X   X     X X X X X   X     X   X   X X X   X           X   \n"
"  X   X X X   X           X X   X   X X X       X   X   X X   X   X X X   X   \n"
"  X   X X X   X   X X X   X   X X     X   X X X   X   X X X   X   X X X   X   \n"
"  X   X X X   X     X           X X   X   X X X         X X   X   X X X   X   \n"
"  X           X   X X   X X         X             X X   X X   X           X   \n"
"  X X X X X X X   X   X   X   X   X   X   X   X   X   X   X   X X X X X X X   \n"
"                    X X X     X   X X   X X     X   X X X X                   \n"
"  X X X X X   X X X X   X             X       X   X X   X X X   X   X   X     \n"
"    X   X X       X X X   X X X X           X   X           X           X     \n"
"    X X   X X X     X       X X X   X     X   X   X X X   X       X X         \n"
"  X     X         X X     X X   X       X X     X   X         X X X   X   X   \n"
"      X X   X X           X   X X X   X   X       X     X X   X X       X X   \n"
"    X       X     X X X               X     X   X           X   X   X         \n"
"    X X     X X   X   X X X       X X   X X   X X X X X           X   X   X   \n"
"  X X X   X       X X   X     X       X X X X X X X   X         X   X X   X   \n"
"  X X   X X X X X     X X       X X         X         X   X   X     X X X X   \n"
"  X X   X   X   X X X X   X X X             X                           X X   \n"
"  X   X   X X X     X       X X   X     X         X X X X   X   X X     X X   \n"
"  X   X   X X   X         X X       X   X X     X     X X       X         X   \n"
"              X     X     X   X X   X     X   X   X   X   X   X X   X   X X   \n"
"  X X   X X X     X   X         X X     X X X X X           X     X     X X   \n"
"  X     X   X X X     X X X         X   X     X     X X X       X X X   X     \n"
"  X   X     X     X   X X     X   X     X   X X X     X   X   X X   X X   X   \n"
"  X X X       X   X   X X       X X         X X               X         X X   \n"
"  X X X X X X   X   X     X X X X X   X   X X X   X     X X X     X           \n"
"  X     X X X X     X       X X     X     X     X X   X X         X       X   \n"
"  X   X X       X         X X     X X X X X     X   X X     X X X       X X   \n"
"  X     X X   X     X X   X   X X   X         X   X   X   X X X X X X   X X   \n"
"                  X X           X X     X X X X         X X       X       X   \n"
"  X X X X X X X   X X X X X     X X X   X     X X X X     X   X   X X   X X   \n"
"  X           X     X X X     X   X     X X X   X   X X X X       X   X       \n"
"  X   X X X   X   X X X X       X     X   X X X     X   X X X X X X X         \n"
"  X   X X X   X   X   X   X X X X     X   X X X X       X     X   X     X X   \n"
"  X   X X X   X   X X X     X X     X X       X X X X   X X   X   X X     X   \n"
"  X           X   X   X   X X     X X X X X X X X X X     X X   X X     X     \n"
"  X X X X X X X   X X X   X   X X   X         X     X X X               X X   \n"
"                                                                              \n"
		);
#endif
	}
}
