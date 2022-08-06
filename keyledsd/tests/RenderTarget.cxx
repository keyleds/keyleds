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
#include "keyledsd/RenderTarget.h"

#include "keyledsd/tools/accelerated.h"
#include <gtest/gtest.h>
#include <type_traits>

using keyleds::RenderTarget;
using keyleds::RGBColor;
using keyleds::RGBAColor;
namespace architecture = keyleds::tools::architecture;


TEST(RenderTargetTest, requirements) {
    static_assert(std::is_default_constructible_v<RenderTarget>);
    static_assert(std::is_const_v<std::remove_reference_t<RenderTarget::const_reference>>);
    static_assert(std::is_unsigned_v<RenderTarget::size_type>);
    static_assert(std::is_signed_v<RenderTarget::difference_type>);

    {   // const_iterator meets BidirectionalIterator
        RenderTarget::iterator i, j;
        static_assert(std::is_default_constructible_v<RenderTarget::iterator>);
        static_assert(std::is_copy_constructible_v<RenderTarget::iterator>);
        static_assert(std::is_copy_assignable_v<RenderTarget::iterator>);
        static_assert(std::is_destructible_v<RenderTarget::iterator>);
        static_assert(std::is_swappable_v<RenderTarget::iterator>);
        static_assert(std::is_same_v<decltype(*i), RenderTarget::value_type &>);
        static_assert(std::is_convertible_v<decltype(++i), RenderTarget::iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i++), RenderTarget::iterator>);
        static_assert(std::is_convertible_v<decltype(--i), RenderTarget::iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i--), RenderTarget::iterator>);
        static_assert(std::is_convertible_v<decltype(i == j), bool>);
        static_assert(std::is_convertible_v<decltype(i != j), bool>);
    }
    {   // const_iterator meets BidirectionalIterator
        RenderTarget::const_iterator i, j;
        static_assert(std::is_default_constructible_v<RenderTarget::const_iterator>);
        static_assert(std::is_copy_constructible_v<RenderTarget::const_iterator>);
        static_assert(std::is_copy_assignable_v<RenderTarget::const_iterator>);
        static_assert(std::is_destructible_v<RenderTarget::const_iterator>);
        static_assert(std::is_swappable_v<RenderTarget::const_iterator>);
        static_assert(std::is_same_v<decltype(*i), const RenderTarget::value_type &>);
        static_assert(std::is_convertible_v<decltype(++i), RenderTarget::const_iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i++), RenderTarget::const_iterator>);
        static_assert(std::is_convertible_v<decltype(--i), RenderTarget::const_iterator>);
        // cppcheck-suppress postfixOperator
        static_assert(std::is_convertible_v<decltype(i--), RenderTarget::const_iterator>);
        static_assert(std::is_convertible_v<decltype(i == j), bool>);
        static_assert(std::is_convertible_v<decltype(i != j), bool>);
    }
    SUCCEED();
}

TEST(RenderTargetTest, construct) {
    auto target = RenderTarget(7);
    EXPECT_EQ(target.begin() + 7, target.end());
    EXPECT_EQ(target.cbegin() + 7, target.cend());
    EXPECT_FALSE(target.empty());
    EXPECT_EQ(7, target.size());
    EXPECT_LE(7, target.capacity());
    EXPECT_EQ(&*target.begin(), target.data());
    EXPECT_EQ(&*target.begin(), &target[0]);
    target[0] = RGBAColor{0x11, 0x22, 0x33, 0x44};
    EXPECT_EQ(RGBAColor(0x11, 0x22, 0x33, 0x44), target.front());
    target[6] = RGBAColor{0xcc, 0xdd, 0xee, 0xff};
    EXPECT_EQ(RGBAColor(0xcc, 0xdd, 0xee, 0xff), target.back());

}

TEST(RenderTargetTest, move) {
    auto targetA = RenderTarget(7);
    targetA[0] = RGBAColor{0x11, 0x22, 0x33, 0x44};

    auto targetB = std::move(targetA); // move construction
    EXPECT_TRUE(targetA.empty());
    ASSERT_FALSE(targetB.empty());
    EXPECT_EQ(RGBAColor(0x11, 0x22, 0x33, 0x44), targetB[0]);

    targetA = std::move(targetB);   // move assignment, empty rendertarget
    EXPECT_TRUE(targetB.empty());
    ASSERT_FALSE(targetA.empty());
    EXPECT_EQ(RGBAColor(0x11, 0x22, 0x33, 0x44), targetA[0]);

    targetB = RenderTarget(13);
    // cppcheck-suppress redundantAssignment
    targetB = std::move(targetA);   // move assignment, non-empty target
    EXPECT_TRUE(targetA.empty());
    ASSERT_FALSE(targetB.empty());
    EXPECT_EQ(RGBAColor(0x11, 0x22, 0x33, 0x44), targetB[0]);
    EXPECT_EQ(7, targetB.size());

    swap(targetA, targetB);         // swap
    EXPECT_TRUE(targetB.empty());
    ASSERT_FALSE(targetA.empty());
    EXPECT_EQ(RGBAColor(0x11, 0x22, 0x33, 0x44), targetA[0]);
}

TEST(RenderTargetTest, iterator) {
    auto target = RenderTarget(7);
    for (auto & item : target) {
        // cppcheck-suppress useStlAlgorithm
        item = RGBAColor{0x11, 0x22, 0x33, 0x44};
    }
    for (auto & item : const_cast<const RenderTarget &>(target)) {
        EXPECT_EQ(RGBAColor(0x11, 0x22, 0x33, 0x44), item);
    }

    std::fill(target.begin(), target.end(), RGBAColor{0x22, 0x33, 0x44, 0x55});
    EXPECT_EQ(7, std::count(target.begin(), target.end(), RGBAColor{0x22, 0x33, 0x44, 0x55}));
}


template <typename T>
class RenderTargetAccelerationTest : public ::testing::Test {
public:
    static constexpr RenderTarget::size_type size = 101;
    using architecture = T;

    RenderTargetAccelerationTest()
     : translucentWhite(size),
       opaqueWhite(size)
    {
        std::fill(translucentWhite.begin(), translucentWhite.end(),
                  RGBAColor{0xff, 0xff, 0xff, 0x7f});
        std::fill(opaqueWhite.begin(), opaqueWhite.end(),
                  RGBAColor{0xff, 0xff, 0xff, 0xff});
    }

    const RGBColor  black = {0, 0, 0};
    const RGBColor  white = {0xff, 0xff, 0xff};
    RenderTarget    translucentWhite;
    RenderTarget    opaqueWhite;
};

namespace testing::internal {   // dirty hack to have type names despite -fno-rtti
    template <> std::string GetTypeName<architecture::plain>() { return "plain"; }
    template <> std::string GetTypeName<architecture::sse2>() { return "sse2"; }
    template <> std::string GetTypeName<architecture::avx2>() { return "avx2"; }
}
using Architectures = ::testing::Types<architecture::plain,
                                       architecture::sse2,
                                       architecture::avx2>;
TYPED_TEST_SUITE(RenderTargetAccelerationTest, Architectures);


TYPED_TEST(RenderTargetAccelerationTest, blend) {
    auto target = RenderTarget(TestFixture::size);
    std::fill(target.begin(), target.end(), TestFixture::black);
    keyleds::blend<typename TestFixture::architecture>(target, TestFixture::translucentWhite);
    EXPECT_EQ(RGBAColor(0x7f, 0x7f, 0x7f, 0xbf), target[0]);
    EXPECT_TRUE(std::all_of(target.begin(), target.end(),
                [](auto item) { return item == RGBAColor{0x7f, 0x7f, 0x7f, 0xbf}; }));
    keyleds::blend<typename TestFixture::architecture>(target, TestFixture::opaqueWhite);
    EXPECT_EQ(RGBAColor(0xff, 0xff, 0xff, 0xff), target[0]);
    EXPECT_TRUE(std::all_of(target.begin(), target.end(),
                [](auto item) { return item == RGBAColor{0xff, 0xff, 0xff, 0xff}; }));
}

TYPED_TEST(RenderTargetAccelerationTest, multiply) {
    auto target = RenderTarget(TestFixture::size);
    std::fill(target.begin(), target.end(), RGBAColor{0xff, 0x80, 0x00, 0x7f});
    keyleds::multiply<typename TestFixture::architecture>(target, TestFixture::translucentWhite);
    EXPECT_EQ(RGBAColor(0xff, 0x80, 0x00, 0x3f), target[0]);
    EXPECT_TRUE(std::all_of(target.begin(), target.end(),
                [](auto item) { return item == RGBAColor{0xff, 0x80, 0x00, 0x3f}; }));
}
