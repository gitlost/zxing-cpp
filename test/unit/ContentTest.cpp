/*
* Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "Content.h"
#include "ECI.h"

#include "gtest/gtest.h"

using namespace ZXing;
using namespace testing;

#if __cplusplus > 201703L
namespace std {
bool operator==(const string& lhs, const char8_t* rhs)
{
	return lhs == reinterpret_cast<const char*>(rhs);
}
} // namespace std
#endif

TEST(ContentTest, Base)
{
	{ // Null
		Content c;
#if defined(ZXING_READERS) || (defined(ZXING_EXPERIMENTAL_API) && defined(ZXING_USE_ZINT))
		EXPECT_EQ(c.guessEncoding(), CharacterSet::Unknown);
#else
		EXPECT_EQ(c.guessEncoding(), CharacterSet::ISO8859_1);
#endif
		EXPECT_EQ(c.symbology.toString(), "");
		EXPECT_TRUE(c.empty());
	}

#ifdef ZXING_READERS
	{ // set latin1
		Content c;
		c.switchEncoding(CharacterSet::ISO8859_1);
		c.append(ByteArray{'A', 0xE9, 'Z'});
		EXPECT_EQ(c.utf8(), u8"A\u00E9Z");
	}

	{ // set CharacterSet::ISO8859_5
		Content c;
		c.switchEncoding(CharacterSet::ISO8859_5);
		c.append(ByteArray{'A', 0xE9, 'Z'});
		EXPECT_EQ(c.utf8(), u8"A\u0449Z");
	}

	{ // switch to CharacterSet::ISO8859_5
		Content c;
		c.append(ByteArray{'A', 0xE9, 'Z'});
		EXPECT_FALSE(c.hasECI);
		c.switchEncoding(CharacterSet::ISO8859_5);
		EXPECT_FALSE(c.hasECI);
		c.append(ByteArray{'A', 0xE9, 'Z'});
		EXPECT_EQ(c.utf8(), u8"A\u00E9ZA\u0449Z");
	}
#endif
}

#ifdef ZXING_READERS
TEST(ContentTest, GuessEncoding)
{
	{ // guess latin1
		Content c;
		c.append(ByteArray{'A', 0xE9, 'Z'});
		EXPECT_EQ(c.guessEncoding(), CharacterSet::ISO8859_1);
		EXPECT_EQ(c.utf8(), u8"A\u00E9Z");
		EXPECT_EQ(c.bytesECI(), c.bytes);
	}

	{ // guess Shift_JIS
		Content c;
		c.append(ByteArray{'A', 0x83, 0x65, 'Z'});
		EXPECT_EQ(c.guessEncoding(), CharacterSet::Shift_JIS);
		EXPECT_EQ(c.utf8(), u8"A\u30C6Z");
	}
}
#endif

#ifdef ZXING_READERS
TEST(ContentTest, ECI)
{
	{ // switch to ECI::ISO8859_5
		Content c;
		c.symbology = {'d', '1', 3}; // DataMatrix
		c.append(ByteArray{'A', 0xE9, 'Z'});
		c.switchEncoding(ECI::ISO8859_5);
		c.append(ByteArray{'A', 0xE9, 'Z'});
		EXPECT_TRUE(c.hasECI);
		EXPECT_EQ(c.utf8(), u8"A\u00E9ZA\u0449Z");
		EXPECT_EQ(c.bytesECI().asString(), std::string_view("]d4\\000003A\xE9Z\\000007A\xE9Z"));
	}

	{ // switch ECI -> latin1 for unknown (instead of Shift_JIS)
		Content c;
		c.symbology = {'d', '1', 3}; // DataMatrix
		c.append(ByteArray{'A', 0x83, 0x65, 'Z'});
		c.switchEncoding(ECI::ISO8859_5);
		c.append(ByteArray{'A', 0xE9, 'Z'});
		EXPECT_EQ(c.utf8(), u8"A\u0083\u0065ZA\u0449Z");
		EXPECT_EQ(c.bytesECI().asString(), std::string_view("]d4\\000003A\x83\x65Z\\000007A\xE9Z"));
	}

	{ // double '\'
		Content c;
		c.symbology = {'d', '1', 3}; // DataMatrix
		c.append("C:\\Test");
		EXPECT_EQ(c.utf8(), u8"C:\\Test");
		EXPECT_EQ(c.bytesECI().asString(), std::string_view("]d1C:\\Test"));
		c.switchEncoding(ECI::UTF8);
		c.append("Täßt");
		EXPECT_EQ(c.bytesECI().asString(), std::string_view("]d4\\000003C:\\\\Test\\000026Täßt"));
	}
}
#endif
