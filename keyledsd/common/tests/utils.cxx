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
#include "keyledsd/utils.h"
#include <chrono>
#include <gtest/gtest.h>


TEST(utilsTest, parseNumber) {
    EXPECT_EQ(123456, keyleds::parseNumber("123456"));
    EXPECT_EQ(123456, keyleds::parseNumber("0123456"));
    EXPECT_EQ(123456, keyleds::parseNumber(" 123456"));
    EXPECT_EQ(123456, keyleds::parseNumber("+123456"));
    EXPECT_TRUE(keyleds::parseNumber("0"));
    EXPECT_FALSE(keyleds::parseNumber(""));
    EXPECT_FALSE(keyleds::parseNumber("42foo"));
}

TEST(utilsTest, parseDuration) {
    using namespace std::literals::chrono_literals;

    EXPECT_EQ(123456000us, keyleds::parseDuration<std::chrono::microseconds>("123456"));
    EXPECT_EQ(123456ms, keyleds::parseDuration<std::chrono::milliseconds>("123456"));
    EXPECT_EQ(123s, keyleds::parseDuration<std::chrono::seconds>("123456"));
    EXPECT_TRUE(keyleds::parseDuration<std::chrono::milliseconds>("0"));
    EXPECT_FALSE(keyleds::parseDuration<std::chrono::milliseconds>(""));
    EXPECT_FALSE(keyleds::parseDuration<std::chrono::milliseconds>("foo"));
}
