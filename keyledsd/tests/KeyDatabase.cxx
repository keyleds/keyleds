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
#include "keyledsd/KeyDatabase.h"

#include <gtest/gtest.h>
#include <cmath>
#include <numeric>
#include <string>
#include <type_traits>

using keyleds::KeyDatabase;
using namespace std::literals::string_literals;

static constexpr double PI = 3.1415926535897932385;


class KeyDatabaseTest : public ::testing::Test
{
protected:
    const KeyDatabase m_db = KeyDatabase({
        {0, 10, "TOPLEFT"s,     {10, 10, 20, 20}},
        {1, 11, "BOTTOMRIGHT"s, {80, 80, 90, 90}},
        {2, 12, "TOPRIGHT"s,    {80, 10, 90, 20}},
        {3, 13, "BOTTOMLEFT"s,  {10, 80, 20, 90}},
        {4, 14, "INSIDE"s,      {40, 50, 50, 60}}
    });
    const unsigned NKEYS = 5;
};


TEST_F(KeyDatabaseTest, requirements) {
    static_assert(std::is_default_constructible_v<KeyDatabase>);
    static_assert(std::is_const_v<std::remove_reference_t<KeyDatabase::const_reference>>);
    static_assert(std::is_unsigned_v<KeyDatabase::size_type>);
    static_assert(std::is_signed_v<KeyDatabase::difference_type>);

    {   // const_iterator meets BidirectionalIterator
        KeyDatabase::const_iterator i, j;
        static_assert(std::is_default_constructible_v<KeyDatabase::const_iterator>);
        static_assert(std::is_copy_constructible_v<KeyDatabase::const_iterator>);
        static_assert(std::is_copy_assignable_v<KeyDatabase::const_iterator>);
        static_assert(std::is_destructible_v<KeyDatabase::const_iterator>);
        static_assert(std::is_swappable_v<KeyDatabase::const_iterator>);
        static_assert(std::is_same_v<decltype(*i), const KeyDatabase::value_type &>);
        static_assert(std::is_convertible_v<decltype(++i), KeyDatabase::const_iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i++), KeyDatabase::const_iterator>);
        static_assert(std::is_convertible_v<decltype(--i), KeyDatabase::const_iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i--), KeyDatabase::const_iterator>);
        static_assert(std::is_convertible_v<decltype(i == j), bool>);
        static_assert(std::is_convertible_v<decltype(i != j), bool>);
    }
    SUCCEED();
}

TEST_F(KeyDatabaseTest, construct) {
    auto copy = m_db;
    EXPECT_EQ(m_db[0].index, copy.begin()->index);
    EXPECT_EQ(&copy[0], &*copy.begin());
    EXPECT_EQ(copy.begin() + NKEYS, copy.end());
    EXPECT_FALSE(copy.empty());
    EXPECT_EQ(NKEYS, copy.size());
}

TEST_F(KeyDatabaseTest, iterator) {
    // Range-for to sum indices
    unsigned sum = 0;
    // cppcheck-suppress useStlAlgorithm
    for (auto & item : m_db) { sum += item.index; }
    EXPECT_EQ((m_db.size()-1) * m_db.size() / 2, sum);

    // STL algorithm to sum indices
    sum = std::accumulate(m_db.begin(), m_db.end(), 0u,
                          [](unsigned val, const auto & key) { return val + key.index; });
    EXPECT_EQ((m_db.size()-1) * m_db.size() / 2, sum);

    // STL find some arbitrary key
    auto it = std::find_if(m_db.begin(), m_db.end(),
                           [&](const auto & key) { return key.name == m_db[3].name; });
    EXPECT_EQ(m_db.begin() + 3, it);
}

TEST_F(KeyDatabaseTest, findKeyCode) {
    EXPECT_EQ(m_db.begin() + 1, m_db.findKeyCode(11));
    EXPECT_EQ(m_db.end(), m_db.findKeyCode(42));
}

TEST_F(KeyDatabaseTest, findName) {
    EXPECT_EQ(m_db.begin() + 1, m_db.findName("BOTTOMRIGHT"));
    EXPECT_EQ(m_db.end(), m_db.findName(""));
    EXPECT_EQ(m_db.end(), m_db.findName("foobar"));
}

TEST_F(KeyDatabaseTest, distance) {
    EXPECT_EQ((KeyDatabase::Rect{10, 10, 90, 90}), m_db.bounds());
    EXPECT_EQ(0, m_db.distance(m_db[0], m_db[0]));
    EXPECT_EQ(98, m_db.distance(m_db[0], m_db[1]));
    EXPECT_EQ(98, m_db.distance(m_db[1], m_db[0]));
    EXPECT_EQ(70, m_db.distance(m_db[1], m_db[2]));
    EXPECT_EQ(70, m_db.distance(m_db[0], m_db[2]));
    EXPECT_EQ(50, m_db.distance(m_db[0], m_db[4]));
}
TEST_F(KeyDatabaseTest, angle) {
    EXPECT_DOUBLE_EQ(0.0, m_db.angle(m_db[0], m_db[0]));
    EXPECT_DOUBLE_EQ(-PI/4.0, m_db.angle(m_db[0], m_db[1]));
    EXPECT_DOUBLE_EQ(3.0*PI/4.0, m_db.angle(m_db[1], m_db[0]));
    EXPECT_DOUBLE_EQ(PI/2.0, m_db.angle(m_db[1], m_db[2]));
    EXPECT_DOUBLE_EQ(0.0, m_db.angle(m_db[0], m_db[2]));
    EXPECT_DOUBLE_EQ(std::atan(-4.0/3.0), m_db.angle(m_db[0], m_db[4]));
}


class KeyGroupTest : public KeyDatabaseTest {
protected:
    const KeyDatabase::KeyGroup bottom = { "bottom"s, { m_db.begin() + 1, m_db.begin() + 3 } };
};

TEST_F(KeyGroupTest, requirements) {
    static_assert(std::is_default_constructible_v<KeyDatabase::KeyGroup>);
    static_assert(std::is_const_v<std::remove_reference_t<KeyDatabase::KeyGroup::const_reference>>);
    static_assert(std::is_unsigned_v<KeyDatabase::KeyGroup::size_type>);
    static_assert(std::is_signed_v<KeyDatabase::KeyGroup::difference_type>);

    {   // const_iterator meets BidirectionalIterator
        KeyDatabase::KeyGroup::const_iterator i, j;
        static_assert(std::is_default_constructible_v<KeyDatabase::KeyGroup::const_iterator>);
        static_assert(std::is_copy_constructible_v<KeyDatabase::KeyGroup::const_iterator>);
        static_assert(std::is_copy_assignable_v<KeyDatabase::KeyGroup::const_iterator>);
        static_assert(std::is_destructible_v<KeyDatabase::KeyGroup::const_iterator>);
        static_assert(std::is_swappable_v<KeyDatabase::KeyGroup::const_iterator>);
        static_assert(std::is_same_v<decltype(*i), const KeyDatabase::KeyGroup::value_type &>);
        static_assert(std::is_convertible_v<decltype(++i), KeyDatabase::KeyGroup::const_iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i++), KeyDatabase::KeyGroup::const_iterator>);
        static_assert(std::is_convertible_v<decltype(--i), KeyDatabase::KeyGroup::const_iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i--), KeyDatabase::KeyGroup::const_iterator>);
        static_assert(std::is_convertible_v<decltype(i == j), bool>);
        static_assert(std::is_convertible_v<decltype(i != j), bool>);
    }
    SUCCEED();
}

TEST_F(KeyGroupTest, construct) {
    // Default key group
    auto empty = KeyDatabase::KeyGroup();
    EXPECT_EQ(empty.begin(), empty.end());
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(0, empty.size());

    // Copy construction and check invariants
    auto copy = bottom;
    EXPECT_EQ(bottom[0].index, copy.begin()->index);
    EXPECT_EQ(&copy[0], &*copy.begin());
    EXPECT_EQ(&copy[0], &*copy.cbegin());
    EXPECT_EQ(copy.begin() + bottom.size(), copy.end());
    EXPECT_EQ(copy.cbegin() + bottom.size(), copy.cend());
    EXPECT_FALSE(copy.empty());
    EXPECT_EQ(bottom.size(), copy.size());

    // Create from sequence using iterators
    auto left = m_db.makeGroup("left", std::vector{"TOPLEFT"s, "foobar"s, "BOTTOMLEFT"s});
    EXPECT_EQ(2, left.size());
    EXPECT_EQ(0, left[0].index);
    EXPECT_EQ(3, left[1].index);

    // Equality operators
    EXPECT_TRUE(left == m_db.makeGroup("left",  std::vector{"TOPLEFT"s, "foobar"s, "BOTTOMLEFT"s}));
    EXPECT_TRUE(left != bottom);
}

TEST_F(KeyGroupTest, iterator) {
    // Range-for to sum indices
    unsigned sum = 0;
    // cppcheck-suppress useStlAlgorithm
    for (auto & key : bottom) { sum += key.index; }
    EXPECT_EQ(4, sum);

    // STL algorithm to sum indices
    sum = std::accumulate(bottom.begin(), bottom.end(), 0u,
                          [](unsigned val, const auto & key) { return val + key.index; });
    EXPECT_EQ(4, sum);

    // STL find some arbitrary key
    auto it = std::find_if(bottom.begin(), bottom.end(),
                           [](const auto & key) { return key.name == "BOTTOMLEFT"; });
    EXPECT_EQ(bottom.begin() + 1, it);

}

TEST_F(KeyGroupTest, clear) {
    auto copy = bottom;
    copy.clear();
    EXPECT_TRUE(copy.empty());
}

TEST_F(KeyGroupTest, erase) {
    auto copy = bottom;
    auto it = std::find_if(copy.begin(), copy.end(), [](const auto & key) { return key.name == "BOTTOMRIGHT"; });
    ASSERT_NE(copy.end(), it);
    copy.erase(it);
    EXPECT_EQ(copy, m_db.makeGroup("test", std::vector{"BOTTOMLEFT"s}));
}

TEST_F(KeyGroupTest, insert) {
    auto copy = bottom;
    copy.insert(copy.begin(), m_db.begin());
    EXPECT_EQ(copy, m_db.makeGroup("test", std::vector{m_db[0].name, "BOTTOMRIGHT"s, "BOTTOMLEFT"s}));
}

TEST_F(KeyGroupTest, pushpop) {
    auto copy = bottom;
    copy.push_back(m_db.begin());
    EXPECT_EQ(copy, m_db.makeGroup("test", std::vector{"BOTTOMRIGHT"s, "BOTTOMLEFT"s, m_db[0].name}));
    copy.pop_back();
    EXPECT_EQ(copy, m_db.makeGroup("test", std::vector{"BOTTOMRIGHT"s, "BOTTOMLEFT"s}));
}
