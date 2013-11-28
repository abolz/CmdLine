// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "ConvertUTF.h"

#include <vector>

#include <gtest/gtest.h>

using support::utf::decodeUTF8Sequence;
using support::utf::decodeUTF16Sequence;

static bool testUTF8(std::vector<int> seq)
{
    auto B = std::begin(seq);
    auto E = std::end(seq);

    while (B != E)
    {
        uint32_t U = 0;

        if (!decodeUTF8Sequence(B, E, U))
            return false;
    }

    return true;
}

TEST(ConvertUTFTest, UTF8_Pass)
{
    // Table 3-7. Well-Formed UTF-8 Byte Sequences
    EXPECT_TRUE(testUTF8({ 0x00 }));                    // U+000000..U+00007F
    EXPECT_TRUE(testUTF8({ 0x7F }));
    EXPECT_TRUE(testUTF8({ 0xC2, 0x80 }));              // U+000080..U+0007FF
    EXPECT_TRUE(testUTF8({ 0xDF, 0xBF }));
    EXPECT_TRUE(testUTF8({ 0xE0, 0xA0, 0x80 }));        // U+000800..U+000FFF
    EXPECT_TRUE(testUTF8({ 0xE0, 0xBF, 0xBF }));
    EXPECT_TRUE(testUTF8({ 0xE1, 0x80, 0x80 }));        // U+001000..U+00CFFF
    EXPECT_TRUE(testUTF8({ 0xEC, 0xBF, 0xBF }));
    EXPECT_TRUE(testUTF8({ 0xED, 0x80, 0x80 }));        // U+00D000..U+00D7FF
    EXPECT_TRUE(testUTF8({ 0xED, 0x9F, 0xBF }));
    EXPECT_TRUE(testUTF8({ 0xEE, 0x80, 0x80 }));        // U+00E000..U+00FFFF
    EXPECT_TRUE(testUTF8({ 0xEF, 0xBF, 0xBF }));
    EXPECT_TRUE(testUTF8({ 0xF0, 0x90, 0x80, 0x80 }));  // U+010000..U+03FFFF
    EXPECT_TRUE(testUTF8({ 0xF0, 0xBF, 0xBF, 0xBF }));
    EXPECT_TRUE(testUTF8({ 0xF1, 0x80, 0x80, 0x80 }));  // U+040000..U+0FFFFF
    EXPECT_TRUE(testUTF8({ 0xF3, 0xBF, 0xBF, 0xBF }));
    EXPECT_TRUE(testUTF8({ 0xF4, 0x80, 0x80, 0x80 }));  // U+100000..U+10FFFF
    EXPECT_TRUE(testUTF8({ 0xF4, 0x8F, 0xBF, 0xBF }));
}

TEST(ConvertUTFTest, UTF8_Fail1)
{
    // 3.5 Impossible bytes
    EXPECT_FALSE(testUTF8({ 0xFE }));
    EXPECT_FALSE(testUTF8({ 0xFF }));
    EXPECT_FALSE(testUTF8({ 0xFE, 0xFE, 0xFF, 0xFF }));
}

TEST(ConvertUTFTest, UTF8_Fail2)
{
    // 4.1 Examples of an overlong ASCII character
    EXPECT_FALSE(testUTF8({ 0xC0, 0xAF }));
    EXPECT_FALSE(testUTF8({ 0xE0, 0x80, 0xAF }));
    EXPECT_FALSE(testUTF8({ 0xF0, 0x80, 0x80, 0xAF }));
    EXPECT_FALSE(testUTF8({ 0xF8, 0x80, 0x80, 0x80, 0xAF }));
    EXPECT_FALSE(testUTF8({ 0xF8, 0x80, 0x80, 0x80, 0x80, 0xAF }));

    // 4.2 Maximum overlong sequences
    EXPECT_FALSE(testUTF8({ 0xC1, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xE0, 0x9F, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xF0, 0x8F, 0xBF, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xF8, 0x87, 0xBF, 0xBF, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xFC, 0x83, 0xBF, 0xBF, 0xBF, 0xBF }));

    // 4.3 Overlong representation of the NUL character
    EXPECT_FALSE(testUTF8({ 0xC0, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xE0, 0x80, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xF0, 0x80, 0x80, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xF8, 0x80, 0x80, 0x80, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xFC, 0x80, 0x80, 0x80, 0x80, 0x80 }));
}

TEST(ConvertUTFTest, UTF8_Fail3)
{
    // 5.1 Single UTF-16 surrogates
    EXPECT_FALSE(testUTF8({ 0xED, 0xA0, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAD, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAE, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAF, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xB0, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xBE, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xBF, 0xBF }));
}

TEST(ConvertUTFTest, UTF8_Fail4)
{
    // 5.2 Paired UTF-16 surrogates
    EXPECT_FALSE(testUTF8({ 0xED, 0xA0, 0x80, 0xED, 0xB0, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xA0, 0x80, 0xED, 0xBF, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAD, 0xBF, 0xED, 0xB0, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAD, 0xBF, 0xED, 0xBF, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAE, 0x80, 0xED, 0xB0, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAE, 0x80, 0xED, 0xBF, 0xBF }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAF, 0xBF, 0xED, 0xB0, 0x80 }));
    EXPECT_FALSE(testUTF8({ 0xED, 0xAF, 0xBF, 0xED, 0xBF, 0xBF }));
}
