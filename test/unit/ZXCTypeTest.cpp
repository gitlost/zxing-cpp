/*
* Copyright 2022 gitlost
*/
// SPDX-License-Identifier: Apache-2.0

#include "ZXCType.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <clocale>
#include <cwctype>

using namespace ZXing;
using namespace testing;

// General test
TEST(ZXCTypeTest, IsWGraph)
{
	// Check that nothing too bad happens for all codepoints
	for (unsigned int u = 0x0; u <= 0x10FFFF; u++) {
		int zx = zx_iswgraph(u);
		EXPECT_TRUE(zx == 1 || zx == 0);
		EXPECT_TRUE(zx == 0 || (zx == 1 && !zx_isspace(u)));
	}

	// Spaces
	EXPECT_FALSE(zx_iswgraph(' ')); // SPACE
	EXPECT_FALSE(zx_iswgraph(0xA0)); // NO-BREAK SPACE (nbsp)
	EXPECT_FALSE(zx_iswgraph(0x1680)); // OGHAM SPACE MARK
	EXPECT_FALSE(zx_iswgraph(0x2000)); // EN QUAD
	EXPECT_FALSE(zx_iswgraph(0x2001)); // EM QUAD
	EXPECT_FALSE(zx_iswgraph(0x2002)); // EN SPACE
	EXPECT_FALSE(zx_iswgraph(0x2003)); // EM SPACE
	EXPECT_FALSE(zx_iswgraph(0x2004)); // THREE-PER-EM SPACE
	EXPECT_FALSE(zx_iswgraph(0x2005)); // FOUR-PER-EM SPACE
	EXPECT_FALSE(zx_iswgraph(0x2006)); // SIX-PER-EM SPACE
	EXPECT_FALSE(zx_iswgraph(0x2007)); // FIGURE SPACE (numsp)
	EXPECT_FALSE(zx_iswgraph(0x2008)); // PUNCTUATION SPACE
	EXPECT_FALSE(zx_iswgraph(0x2009)); // THIN SPACE
	EXPECT_FALSE(zx_iswgraph(0x200A)); // HAIR SPACE
	EXPECT_FALSE(zx_iswgraph(0x2028)); // LINE SEPARATOR
	EXPECT_FALSE(zx_iswgraph(0x2029)); // PARAGRAPH SEPARATOR
	EXPECT_FALSE(zx_iswgraph(0x202F)); // NARROW NO-BREAK SPACE
	EXPECT_FALSE(zx_iswgraph(0x205F)); // MEDIUM MATHEMATICAL SPACE
	EXPECT_FALSE(zx_iswgraph(0x3000)); // IDEOGRAPHIC SPACE

	// Note this is in category Cf (Other, Format)
	EXPECT_FALSE(zx_iswgraph(0x200B)); // ZERO WIDTH SPACE

	// This is in category So (Symbol, Other) so marked as graphical (TODO should it be?)
	EXPECT_TRUE(zx_iswgraph(0x303F)); // IDEOGRAPHIC HALF FILL SPACE

	// Note this is in category Cf (Other, Format)
	EXPECT_FALSE(zx_iswgraph(0xFEFF)); // ZERO WIDTH NO-BREAK SPACE

	EXPECT_TRUE(zx_iswgraph(0xFFE0)); // FULLWIDTH CENT SIGN
	EXPECT_TRUE(zx_iswgraph(0xFFE5)); // FULLWIDTH YEN SIGN
	EXPECT_TRUE(zx_iswgraph(0xFFEE)); // HALFWIDTH WHITE CIRCLE
	EXPECT_FALSE(zx_iswgraph(0xFFEF)); // Unassigned (Cn) (BOM)
	EXPECT_TRUE(zx_iswgraph(0xFFFC)); // OBJECT REPLACEMENT CHARACTER
	EXPECT_TRUE(zx_iswgraph(0xFFFD)); // REPLACEMENT CHARACTER
	EXPECT_FALSE(zx_iswgraph(0xFFFE)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0xFFFF)); // Unassigned (Cn)

	EXPECT_TRUE(zx_iswgraph(0x1DF2A)); // LATIN SMALL LETTER T WITH MID-HEIGHT LEFT HOOK
	EXPECT_FALSE(zx_iswgraph(0x1DF2B)); // Unassigned (Cn)
	EXPECT_TRUE(zx_iswgraph(0x1EEF1)); // ARABIC MATHEMATICAL OPERATOR HAH WITH DAL
	EXPECT_FALSE(zx_iswgraph(0x1EEF2)); // Unassigned (Cn)
	EXPECT_TRUE(zx_iswgraph(0x1FBF9)); // SEGMENTED DIGIT NINE
	EXPECT_FALSE(zx_iswgraph(0x1FBFA)); // Unassigned (Cn)

	EXPECT_TRUE(zx_iswgraph(0x2FA1D)); // CJK COMPATIBILITY IDEOGRAPH-2FA1D
	EXPECT_FALSE(zx_iswgraph(0x2FA1E)); // Unassigned (Cn)

	EXPECT_TRUE(zx_iswgraph(0x323AF)); // <CJK Ideograph Extension H, Last>
	EXPECT_FALSE(zx_iswgraph(0x323B0)); // Unassigned (Cn)

	EXPECT_FALSE(zx_iswgraph(0x40000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0x50000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0x60000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0x70000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0x80000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0x90000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0xA0000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0xB0000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0xC0000)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0xD0000)); // Unassigned (Cn)

	EXPECT_TRUE(zx_iswgraph(0xE01EF)); // VARIATION SELECTOR-256
	EXPECT_FALSE(zx_iswgraph(0xE01F0)); // Unassigned (Cn)

	EXPECT_FALSE(zx_iswgraph(0xF0000)); // Unassigned (Cn)

	EXPECT_FALSE(zx_iswgraph(0x10FFFD)); // <Plane 16 Private Use, Last> (Co)
	EXPECT_FALSE(zx_iswgraph(0x10FFFE)); // Unassigned (Cn)
	EXPECT_FALSE(zx_iswgraph(0x10FFFF)); // Unassigned (Cn)

	EXPECT_FALSE(zx_iswgraph(0x110000)); // Invalid Unicode
}

// Test against `std::iswgraph()`
TEST(ZXCTypeTest, IsWGraphToStd)
{
	std::string ctype_locale(std::setlocale(LC_CTYPE, NULL)); // Use std::string to avoid assert on Windows Debug
	EXPECT_EQ(ctype_locale, std::string("C"));

#ifndef _WIN32
	std::setlocale(LC_CTYPE, "en_US.UTF-8");
	EXPECT_STREQ(std::setlocale(LC_CTYPE, NULL), "en_US.UTF-8");
#else
	std::setlocale(LC_CTYPE, ".utf8");
	EXPECT_TRUE(std::string(std::setlocale(LC_CTYPE, NULL)).find("utf8") != std::string::npos);
#endif

	for (unsigned int u = 0x0; u <= 0x10FFFF; u++) {
		int zx = zx_iswgraph(u);
		int wc = std::iswgraph(static_cast<wchar_t>(u));

		// "glibc/localedata/unicode-gen/gen_unicode_ctype.py" `is_graph()` uses `name` != "<control>" and `!is_space()`
		// where `is_space()` is standard spaces <= 0x20 and categories Zl, Zp and Zs without "<noBreak>" in
		// decomposition field, whereas `zx_iswgraph()` is using not categories C or Z with some visible category C
		// exceptions (see "core/src/ZXCType.cpp")

		if (u == 0xA0) { // NO-BREAK SPACE
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0xAD) { // SHY 
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0x61C) { // ARABIC LETTER MARK
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0x61D) { // ARABIC END OF TEXT MARK (Unicode 15)
			EXPECT_TRUE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0x6DD) { // ARABIC END OF AYAH
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0x70F) { // SYRIAC ABBREVIATION MARK
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0x180E) { // MONGOLIAN VOWEL SEPARATOR
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0x2007) { // FIGURE SPACE
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0x200B && u <= 0x200F) { // ZERO WIDTH SPACE..RIGHT-TO-LEFT MARK
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0x202A && u <= 0x202F) { // LEFT-TO-RIGHT EMBEDDING..NARROW NO-BREAK SPACE
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0x2060 && u <= 0x2064) { // WORD JOINER..INVISIBLE PLUS
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0x2066 && u <= 0x206F) { // LEFT-TO-RIGHT ISOLATE..NOMINAL DIGIT SHAPES
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0xE000 && u <= 0xF8FF) { // PUA - treating all as non-graphical (`std::iswgraph()` does opposite)
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u == 0xFEFF) { // ZERO WIDTH NO-BREAK SPACE
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0xFFF9 && u <= 0xFFFB) { // INTERLINEAR ANNOTATION ANCHOR..INTERLINEAR ANNOTATION TERMINATOR
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0x13430 && u <= 0x13438) { // EGYPTIAN HIEROGLYPH VERTICAL JOINER..EGYPTIAN HIEROGLYPH END SEGMENT
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0x1BCA0 && u <= 0x1BCA3) { // SHORTHAND FORMAT LETTER OVERLAP..SHORTHAND FORMAT UP STEP
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		} else if (u >= 0x1D173 && u <= 0x1D17A) { // MUSICAL SYMBOL BEGIN BEAM..MUSICAL SYMBOL END PHRASE
			EXPECT_FALSE(zx) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;

		// These are newish and unlikely to be in `std::iswgraph()` yet
		} else if (u == 0x61D || (u >= 0x870 && u <= 0x089F) || u == 0x08B5 || (u >= 0x08BE && u <= 0x08D2)
				|| u == 0x0B55 || u == 0x0C3C || u == 0x0C5D || u == 0x0CDD || u == 0x0CF3 || u == 0x0D04
				|| u == 0x0D81 || u == 0x0ECE || u == 0x170D || u == 0x1715 || u == 0x171F || u == 0x180F
				|| (u >= 0x1ABF && u <= 0x1ACE) || u == 0x1B4C || u == 0x1B7D || u == 0x1B7E || u == 0x1DFA
				|| u == 0x20C0 || u == 0x2B97 || u == 0x2C2F || u == 0x2C5F || (u >= 0x2E50 && u <= 0x2E5D)
				|| (u >= 0x31BB && u <= 0x31BF) || (u >= 0x4DB6 && u <= 0x4DBF) || (u >= 0x9FF0 && u <= 0x9FFF)
				|| u == 0xA7C0 || u == 0xA7C1 || (u >= 0xA7C7 && u <= 0xA7CA) || u == 0xA7D0 || u == 0xA7D1
				|| u == 0xA7D3 || (u >= 0xA7D5 && u <= 0xA7D9) || (u >= 0xA7F2 && u <= 0xA7F6) || u == 0xA82C
				|| (u >= 0xAB68 && u <= 0xAB6B) || u == 0xFBC2 || (u >= 0xFD40 && u <= 0xFD4F) || u == 0xFDCF
				|| u == 0xFDFE || u == 0xFDFF || u == 0x1019C || (u >= 0x10570 && u <= 0x105BC)
				|| (u >= 0x10780 && u <= 0x107BA) || (u >= 0x10E80 && u <= 0x10EA9) || (u >= 0x10EAB && u <= 0x10EAD)
				|| u == 0x10EB0 || u == 0x10EB1 || (u >= 0x10EFD && u <= 0x10EFF) || (u >= 0x10F70 && u <= 0x10FCB)
				|| (u >= 0x11070 && u <= 0x11075) || u == 0x110C2 || u == 0x11147 || u == 0x111CE || u == 0x111CF
				|| (u >= 0x1123F && u <= 0x11241) || u == 0x1145A || u == 0x11460 || u == 0x11461 || u == 0x116B9
				|| (u >= 0x11740 && u <= 0x11746) || (u >= 0x11900 && u <= 0x11906) || u == 0x11909
				|| (u >= 0x1190C && u <= 0x11916) || (u >= 0x11918 && u <= 0x11938) || (u >= 0x1193B && u <= 0x11946)
				|| (u >= 0x11950 && u <= 0x11959) || (u >= 0x11AB0 && u <= 0x11ABF) || (u >= 0x11B00 && u <= 0x11B09)
				|| (u >= 0x11F00 && u <= 0x11F3A) || (u >= 0x11F3E && u <= 0x11FB0) || (u >= 0x12F90 && u <= 0x12FF2)
				|| u == 0x1342F || (u >= 0x13439 && u <= 0x13455) || (u >= 0x16A70 && u <= 0x16ABE)
				|| (u >= 0x16AC0 && u <= 0x16AC9) || u == 0x16FE4 || u == 0x16FF0 || u == 0x16FF1
				|| (u >= 0x18AF3 && u <= 0x18CD5) || (u >= 0x18D00 && u <= 0x18D08) || (u >= 0x1AFF0 && u <= 0x1AFFE)
				|| (u >= 0x1B11F && u <= 0x1B132) || u == 0x1B155 || (u >= 0x1CF00 && u <= 0x1CFC3) || u == 0x1D1E9
				|| u == 0x1D1EA || (u >= 0x1D2C0 && u <= 0x1D2D3) || (u >= 0x1DF00 && u <= 0x1DF2A)
				|| (u >= 0x1E030 && u <= 0x1E06D) || u == 0x1E08F || (u >= 0x1E290 && u <= 0x1E2AE)
				|| (u >= 0x1E4D0 && u <= 0x1E4F9) || (u >= 0x1E7E0 && u <= 0x1E7EE) || (u >= 0x1E7F0 && u <= 0x1E7FE)
				|| (u >= 0x1F10D && u <= 0x1F10F) || u == 0x1F1AD || u == 0x1F6D6 || u == 0x1F6D7
				|| (u >= 0x1F6DC && u <= 0x1F6DF) || u == 0x1F6FB || u == 0x1F6FC || (u >= 0x1F774 && u <= 0x1F77F)
				|| u == 0x1F7D9 || u == 0x1F7F0 || u == 0x1F8B0 || u == 0x1F8B1 || u == 0x1F90C || u == 0x1F972
				|| (u >= 0x1F977 && u <= 0x1F979) || u == 0x1F9A3 || u == 0x1F9A4 || (u >= 0x1F9AB && u <= 0x1F9AD)
				|| u == 0x1F9CB || u == 0x1F9CC || (u >= 0x1FA7 || u <= 0x1FA77) || u == 0x1FA7B || u == 0x1FA7C
				|| (u >= 0x1FA83 && u <= 0x1FA88) || (u >= 0x1FA96 && u <= 0x1FAC5) || (u >= 0x1FACE && u <= 0x1FADB)
				|| (u >= 0x1FAE0 && u <= 0x1FAE8) || (u >= 0x1FAF0 && u <= 0x1FAF8) || (u >= 0x1FB00 && u <= 0x1FBCA)
				|| (u >= 0x1FBF0 && u <= 0x1FBF9)) {
		} else {
			EXPECT_EQ(zx, wc) << "u U+" << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << u;
		}
	}

	std::setlocale(LC_CTYPE, ctype_locale.c_str()); // Restore
}
