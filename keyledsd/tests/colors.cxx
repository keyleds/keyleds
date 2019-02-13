/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "keyledsd/colors.h"
#include <gtest/gtest.h>
#include <sstream>

using keyleds::RGBColor;
using keyleds::RGBAColor;

TEST(RGBColorTest, construct) {
    auto color = RGBColor{0x11, 0x22, 0x33};
    EXPECT_EQ(0x11, color.red);
    EXPECT_EQ(0x22, color.green);
    EXPECT_EQ(0x33, color.blue);

    auto copy = color;
    EXPECT_EQ(0x11, copy.red);
    EXPECT_EQ(0x22, copy.green);
    EXPECT_EQ(0x33, copy.blue);
}

TEST(RGBColorTest, output) {
    std::ostringstream buffer1;
    buffer1 <<RGBColor{0x11, 0x22, 0x33};
    EXPECT_EQ("#112233", buffer1.str());

    std::ostringstream buffer2;
    buffer2 <<RGBColor{0x00, 0x01, 0x02};
    EXPECT_EQ("#000102", buffer2.str());
}

TEST(RGBColorTest, parse) {
    EXPECT_EQ((RGBColor{0x11, 0x22, 0x33}), RGBColor::parse("112233")); // hexa numerals
    EXPECT_EQ((RGBColor{0xab, 0xcd, 0xef}), RGBColor::parse("abcdef")); // hexa letters
    EXPECT_EQ((RGBColor{0xab, 0xcd, 0xef}), RGBColor::parse("AbcDEF")); // case insensitive
    EXPECT_EQ((RGBColor{0x00, 0xff, 0xff}), RGBColor::parse("cyan"));   // color name
    EXPECT_EQ((RGBColor{0x00, 0xff, 0xff}), RGBColor::parse("CyAn"));   // case insensitive
    EXPECT_FALSE(RGBColor::parse("123"));
    EXPECT_FALSE(RGBColor::parse("0000gg"));
    EXPECT_FALSE(RGBColor::parse("foobar"));
}

TEST(RGBAColorTest, construct) {
    auto color = RGBAColor{0x11, 0x22, 0x33, 0x44};
    EXPECT_EQ(0x11, color.red);
    EXPECT_EQ(0x22, color.green);
    EXPECT_EQ(0x33, color.blue);
    EXPECT_EQ(0x44, color.alpha);

    auto copy = color;
    EXPECT_EQ(0x11, copy.red);
    EXPECT_EQ(0x22, copy.green);
    EXPECT_EQ(0x33, copy.blue);
    EXPECT_EQ(0x44, copy.alpha);

    auto opaque = RGBAColor(RGBColor{0x11, 0x22, 0x33});
    EXPECT_EQ(0x11, opaque.red);
    EXPECT_EQ(0x22, opaque.green);
    EXPECT_EQ(0x33, opaque.blue);
    EXPECT_EQ(0xff, opaque.alpha);
}

TEST(RGBAColorTest, output) {
    std::ostringstream buffer1;
    buffer1 <<RGBAColor{0x11, 0x22, 0x33, 0x44};
    EXPECT_EQ("#11223344", buffer1.str());

    std::ostringstream buffer2;
    buffer2 <<RGBAColor{0x00, 0x01, 0x02, 0x03};
    EXPECT_EQ("#00010203", buffer2.str());
}

TEST(RGBAColorTest, parse) {
    // Alpha channel formats
    EXPECT_EQ((RGBAColor{0x11, 0x22, 0x33, 0x44}), RGBAColor::parse("11223344")); // hexa numerals
    EXPECT_EQ((RGBAColor{0xab, 0xcd, 0xef, 0xab}), RGBAColor::parse("abcdefab")); // hexa letters
    EXPECT_EQ((RGBAColor{0xab, 0xcd, 0xef, 0xab}), RGBAColor::parse("AbcDEFAb")); // case insensitive
    // RGBColor should be valid too (and fully opaque)
    EXPECT_EQ((RGBAColor{0x11, 0x22, 0x33, 0xff}), RGBAColor::parse("112233")); // hexa numerals
    EXPECT_EQ((RGBAColor{0xab, 0xcd, 0xef, 0xff}), RGBAColor::parse("abcdef")); // hexa letters
    EXPECT_EQ((RGBAColor{0xab, 0xcd, 0xef, 0xff}), RGBAColor::parse("AbcDEF")); // case insensitive
    EXPECT_EQ((RGBAColor{0x00, 0xff, 0xff, 0xff}), RGBAColor::parse("cyan"));   // color name
    EXPECT_EQ((RGBAColor{0x00, 0xff, 0xff, 0xff}), RGBAColor::parse("CyAn"));   // case insensitive
    // Invalid entries
    EXPECT_FALSE(RGBAColor::parse("123"));
    EXPECT_FALSE(RGBAColor::parse("000000gg"));
    EXPECT_FALSE(RGBAColor::parse("foobar"));
}
