/*
 * Copyright 2017 Huy Cuong Nguyen
 * Copyright 2008 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "qrcode/QRDecoder.h"

#include "BitMatrix.h"
#include "BitMatrixIO.h"
#include "DecoderResult.h"
#include "ECI.h"

#include "gtest/gtest.h"

using namespace ZXing;
using namespace ZXing::QRCode;

TEST(RMQRDecoderTest, RMQRCodeR7x43M)
{
	const auto bitMatrix = ParseBitMatrix(
		"XXXXXXX X X X X X X XXX X X X X X X X X XXX\n"
		"X     X  X XXX  XXXXX XXX      X X XX   X X\n"
		"X XXX X X XXX X X X XXXX XXXX X  X XXXXXXXX\n"
		"X XXX X  XX    XXXXX   XXXXXX   X X   X   X\n"
		"X XXX X   XX  XXX   XXXXXXX  X X  XX  X X X\n"
		"X     X XXXXX XXX XXX XXXXX    XXXXXX X   X\n"
		"XXXXXXX X X X X X X XXX X X X X X X X XXXXX\n",
		'X', false);

	const auto result = Decode(bitMatrix);
	EXPECT_TRUE(result.isValid());
	EXPECT_EQ(result.content().text(TextMode::Plain), "ABCDEFG");
}

TEST(RMQRDecoderTest, RMQRCodeR7x43MError6Bits)
{
	const auto bitMatrix = ParseBitMatrix(
		"XXXXXXX X X X X X X XXX X X X X X X X X XXX\n"
		"X     X  X XXX  XXXXX XXX      X X XX   X X\n"
		"X XXX X X XXX   X X XXXX XXXX XX X XXXXXXXX\n" // 2
		"X XXX X  XX    XXXXX X XXXXXX   X X   X   X\n" // 3
		"X XXX X   XX  XXX   XXXXXXX  X X XXX  X X X\n" // 5
		"X     X XXXXX XXX XXX XXXX X   XXXXXX X   X\n" // 6
		"XXXXXXX X X X X X X XXX X X X X X X X XXXXX\n",
		'X', false);

	const auto result = Decode(bitMatrix);
	EXPECT_EQ(Error::Checksum, result.error());
	EXPECT_TRUE(result.text().empty());
	EXPECT_TRUE(result.content().text(TextMode::Plain).empty());
}

TEST(RMQRDecoderTest, RMQRCodeR9x59H)
{
	const auto bitMatrix = ParseBitMatrix(
		"XXXXXXX X X X X X XXX X X X X X X X X XXX X X X X X X X XXX\n"
		"X     X    X  XXXXX XXX X  X XXXXXXXX X X  X    X XXXX  X X\n"
		"X XXX X XX XXX  X XXX XXXX  X         XXXXXXX  X XXXXX X  X\n"
		"X XXX X XXXX X XX X   XX   XXXX XX  XX   X  X  X XXX     X \n"
		"X XXX X    X    X XX XXXXXX X X XX   X XX   X X XXXX  XXXXX\n"
		"X     X X  X  X  X  XXX X X   X   XX  X XXXX XX  X X  X   X\n"
		"XXXXXXX  XXXXX  XXXXXX X XX XXX X    XXXX  X    X  X XX X X\n"
		"          XXX  XXXX XX XXX    X XXXXXXX X XX XXX  XX XX   X\n"
		"XXX X X X X X X X XXX X X X X X X X X XXX X X X X X X XXXXX\n",
		'X', false);

	const auto result = Decode(bitMatrix);
	EXPECT_TRUE(result.isValid());
	EXPECT_EQ(result.content().text(TextMode::Plain), "ABCDEFGHIJKLMN");
}

TEST(RMQRDecoderTest, RMQRCodeR11x27H)
{
	const auto bitMatrix = ParseBitMatrix(
		"XXXXXXX X X X X X X X X XXX\n"
		"X     X  XX        X  X X X\n"
		"X XXX X    X  XX X   X   XX\n"
		"X XXX X XXXX XX X  XXXXXX  \n"
		"X XXX X  X X XX  XX   XXX X\n"
		"X     X XXX  X XX  XXXX  X \n"
		"XXXXXXX     X   XX  X XXXXX\n"
		"           X   X   X  X   X\n"
		"XXXX  X   X X XX XXXXXX X X\n"
		"X XX XXXXXX XXX  XXXX X   X\n"
		"XXX X X X X X X X X X XXXXX\n",
		'X', false);

	const auto result = Decode(bitMatrix);
	EXPECT_TRUE(result.isValid());
	EXPECT_EQ(result.content().text(TextMode::Plain), "ABCDEF");
}

TEST(RMQRDecoderTest, RMQRCodeR13x27M)
{
	const auto bitMatrix = ParseBitMatrix(
		"XXXXXXX X X X X X X X X XXX\n"
		"X     X    XX XX XXX   XX X\n"
		"X XXX X XX  X  XX XX XXX  X\n"
		"X XXX X  XX X XX X X   XX  \n"
		"X XXX X XXXXXXX X X      XX\n"
		"X     X   XX X  XXX  XX XX \n"
		"XXXXXXX   X   X X    X  XXX\n"
		"        XXX XX X  XX   XXX \n"
		"XXX XX XX X  X XX XX  XXXXX\n"
		" XXX  X    X X    X   X   X\n"
		"X XX X  X   XX X XX X X X X\n"
		"X   X   X  X X X X    X   X\n"
		"XXX X X X X X X X X X XXXXX\n",
		'X', false);

	const auto result = Decode(bitMatrix, CharacterSet::ISO8859_1);
	EXPECT_TRUE(result.isValid());
	EXPECT_EQ(result.content().text(TextMode::Plain), "AB貫12345AB");
	EXPECT_TRUE(result.content().hasECI);
	EXPECT_EQ(result.content().encodings[0].eci, ECI::Shift_JIS);
	EXPECT_EQ(result.content().symbology.toString(), "]Q1");
}

TEST(RMQRDecoderTest, RMQRCodeR15x59H)
{
	const auto bitMatrix = ParseBitMatrix(
		"XXXXXXX X X X X X XXX X X X X X X X X XXX X X X X X X X XXX\n"
		"X     X X XXXXXX XX XX X    X X   XX  X XX   X    X  XXX  X\n"
		"X XXX X X     XXX XXX  X XX XXXXXX  X XXXX XX XX          X\n"
		"X XXX X  X X    X    X  X  XXX X   X X   X X XXXXX XX X  X \n"
		"X XXX X X  X X XX  XXX X  X XXX    XX  X X  XXX   XX X    X\n"
		"X     X     XX  X X X  XX X XXX   XX XX X  XXX X XX XX X   \n"
		"XXXXXXX    X   X  XX XX  X XXXXX XXX   X     XX  XXX X  XXX\n"
		"        XXXXX XX XX   X X  XXX  XX X        XX    XX     X \n"
		"XXX XX  X    XX    X  X  XX XXXXXX  X XX  XXX XXXXXX XX   X\n"
		" X  XXX XX  X X   X   XXXX X XXXXXXXXXX XXX XXXX XX  X X   \n"
		"X XX X XXX  XXX  X X X     XX XXXXX    X X  XXXXXXXXX XXXXX\n"
		"  XXXX   XX     XX   XX X XXX  X  X X X XX X    XXX  XX   X\n"
		"XX   X XXXX  XXX XXXXXXX     XXX   X XXXX XX  X X    XX X X\n"
		"X  XX XXXX XXXXX XX XXXX  XXX  X XX XXX X XX  XX XX  XX   X\n"
		"XXX X X X X X X X XXX X X X X X X X X XXX X X X X X X XXXXX\n",
		'X', false);

	const auto result = Decode(bitMatrix, CharacterSet::ISO8859_1); // Hinted charset required
	EXPECT_TRUE(result.isValid());
	EXPECT_EQ(result.content().text(TextMode::Plain), "12345678Àæð¿ABCDEFGHIJKLMNOPQ");
}

TEST(RMQRDecoderTest, RMQRCodeR17x99H)
{
	const auto bitMatrix = ParseBitMatrix(
		"XXXXXXX X X X X X X X XXX X X X X X X X X X X X XXX X X X X X X X X X X X XXX X X X X X X X X X XXX\n"
		"X     X   X  XX X    XX X XXX    X       XXX  XXX XXXX XX     X X XX  XXX X X XX XXX X  XX  XX  X X\n"
		"X XXX X X X X XXX XX  XXX  XX  X    X   X  X XXXXXX X    XX X    X    XX XXXXX X X  X       X XX  X\n"
		"X XXX X   XXX  XXXX XX  XXXX X   XX   XX       XX      X X  XX XXXX   XX X    XXXX XXXXXXXXX XXXXX \n"
		"X XXX X     X  XX  X X X XXXX XX X   X X X  XX   X X  X XX  XXX XX  X X   XX    X  X X XXX X XX  XX\n"
		"X     X XX   XX  X X    XXXX XXX XXX X X XX X   X  X X  X  XX X  XXXX   XX   X X XX     X  XXX   X \n"
		"XXXXXXX   XXXX X X XX  XXX XXX  X      XX   XXX XX XX  X  X       X X  X XXXXX X     X   XXXX  XX X\n"
		"         XXXX   X X  X    XXXX X   XXXXX XX  X X    X   X    X XXX    XX    X X  X XXX XXX XXXX X  \n"
		"XX   XX X  XX   XX    XXXX  XX X  XXX  XXXX  XXXXXXXXXXX XX     XX X XX   XX    XX X  XXX  X   XXXX\n"
		" X   XXXXXXXXXX  X       XXX  X XXXX XXXXXX  X    XX   XX   XXX  XXXX   X X  X XX X XXXX X  X X XX \n"
		"X X  XX X   XX   X  XXXX X XXXX   X XX X XXXXX XXX X X X X   XX   X X  XXX XXX X     X X   X  XXXXX\n"
		" XXX XXX   X   XXXX XX  X XXXX  XX XX  XXX    XXX XXX X   XX  X X      X    X X  XXX  XXXX X XXX   \n"
		"X XX  XX     XXX    X XX  X XXXX X X    X   X    X  X  X X    XXXX X X X   X X XXX XX X  XXXXXXXXXX\n"
		" XX  X XXX XXXXX    XX  XX X    XX  X X   X   XX   X X  X XX  XX   X X XX X X XXX   XXXX  X X X   X\n"
		"X  XXXX  XXX XXXX XX  XXX XX   XX X X XXXX X  XXXXX XX XX X   XXXXX XXX XXXXX    XXX XX X XX XX X X\n"
		"X  X XXXX X  X XXX  XXX XX X   X  XXX  XXXXXXXXXX XX X   XX  X  XX  X X X X X     X     XX XXXX   X\n"
		"XXX X X X X X X X X X XXX X X X X X X X X X X X XXX X X X X X X X X X X X XXX X X X X X X X X XXXXX\n",
		'X', false);

	const auto result = Decode(bitMatrix, CharacterSet::ISO8859_1); // Hinted charset required
	EXPECT_TRUE(result.isValid());
	EXPECT_EQ(result.content().text(TextMode::Plain), "\u0093_1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}
