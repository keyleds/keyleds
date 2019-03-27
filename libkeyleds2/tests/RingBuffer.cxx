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
#include "libkeyleds/RingBuffer.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <string>


TEST(RingBuffer, requirements_int) {
    using RingBuffer = typename libkeyleds::RingBuffer<int, 4>;

    static_assert(std::is_default_constructible_v<RingBuffer>);
    static_assert(!std::is_copy_constructible_v<RingBuffer>);
    static_assert(!std::is_move_constructible_v<RingBuffer>);

    static_assert(std::is_same_v<int, typename RingBuffer::value_type>);
    static_assert(std::is_unsigned_v<typename RingBuffer::size_type>);
    static_assert(std::is_reference_v<typename RingBuffer::reference>);
    static_assert(std::is_reference_v<typename RingBuffer::const_reference>);
    static_assert(std::is_const_v<std::remove_reference_t<typename RingBuffer::const_reference>>);

    static_assert(noexcept(std::declval<RingBuffer>().push(42)));
    static_assert(noexcept(std::declval<RingBuffer>().emplace()));
    static_assert(noexcept(std::declval<RingBuffer>().emplace(42)));
    static_assert(noexcept(std::declval<RingBuffer>().pop()));
    static_assert(noexcept(std::declval<RingBuffer>().clear()));
    static_assert(noexcept(std::declval<RingBuffer>().empty()));
    static_assert(noexcept(std::declval<RingBuffer>().size()));

    SUCCEED();
}

TEST(RingBuffer, requirements_string) {
    using RingBuffer = typename libkeyleds::RingBuffer<std::string, 4>;

    static_assert(std::is_default_constructible_v<RingBuffer>);
    static_assert(!std::is_copy_constructible_v<RingBuffer>);
    static_assert(!std::is_move_constructible_v<RingBuffer>);

    static_assert(std::is_same_v<std::string, typename RingBuffer::value_type>);
    static_assert(std::is_unsigned_v<typename RingBuffer::size_type>);
    static_assert(std::is_reference_v<typename RingBuffer::reference>);
    static_assert(std::is_reference_v<typename RingBuffer::const_reference>);
    static_assert(std::is_const_v<std::remove_reference_t<typename RingBuffer::const_reference>>);

    static_assert(!noexcept(std::declval<RingBuffer>().push(std::declval<std::string &>())));
    static_assert(noexcept(std::declval<RingBuffer>().push(std::declval<std::string>())));
    static_assert(noexcept(std::declval<RingBuffer>().emplace()));
    static_assert(!noexcept(std::declval<RingBuffer>().emplace(std::declval<std::string &>())));
    static_assert(noexcept(std::declval<RingBuffer>().emplace(std::declval<std::string>())));
    static_assert(!noexcept(std::declval<RingBuffer>().emplace(42, 'x')));
    static_assert(!noexcept(std::declval<RingBuffer>().emplace("foo")));
    static_assert(noexcept(std::declval<RingBuffer>().pop()));
    static_assert(noexcept(std::declval<RingBuffer>().clear()));
    static_assert(noexcept(std::declval<RingBuffer>().empty()));
    static_assert(noexcept(std::declval<RingBuffer>().size()));

    SUCCEED();
}
TEST(RingBuffer, integer) {
    using RingBuffer = typename libkeyleds::RingBuffer<int, 4>;

    auto buffer = RingBuffer();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.capacity(), 4);
    EXPECT_EQ(buffer.max_size(), 4);

    buffer.push(42);
    buffer.push(43);
    buffer.push(44);
    buffer.push(45);
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.size(), 4);
    EXPECT_EQ(buffer.capacity(), 4);
    EXPECT_EQ(buffer.max_size(), 4);
    EXPECT_EQ(buffer.front(), 42);

    buffer.pop();
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.front(), 43);
    buffer.pop();
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.front(), 44);

    for (int idx = 0; idx < 10; ++idx) {
        buffer.push(idx);
        EXPECT_EQ(buffer.size(), 3);
        buffer.pop();
        EXPECT_EQ(buffer.size(), 2);
    }

    buffer.clear();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.capacity(), 4);
    EXPECT_EQ(buffer.max_size(), 4);
}


template <typename T> struct LifecycleTpl
{
    LifecycleTpl() { defaultConstructor += 1; }
    LifecycleTpl(const LifecycleTpl &) { copyConstructor += 1; }
    LifecycleTpl(LifecycleTpl &&) { moveConstructor += 1; }
    ~LifecycleTpl() { destructor += 1; }
    LifecycleTpl & operator=(const LifecycleTpl &) = delete;
    LifecycleTpl & operator=(LifecycleTpl &&) = delete;
    static unsigned defaultConstructor;
    static unsigned copyConstructor;
    static unsigned moveConstructor;
    static unsigned destructor;
};
template <typename T> unsigned LifecycleTpl<T>::defaultConstructor = 0;
template <typename T> unsigned LifecycleTpl<T>::copyConstructor = 0;
template <typename T> unsigned LifecycleTpl<T>::moveConstructor = 0;
template <typename T> unsigned LifecycleTpl<T>::destructor = 0;

TEST(RingBuffer, lifecycle) {
    using Lifecycle = LifecycleTpl<RingBuffer_lifecycle_Test>;
    using RingBuffer = typename libkeyleds::RingBuffer<Lifecycle, 4>;

    auto buffer = RingBuffer();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(Lifecycle::defaultConstructor, 0);
    EXPECT_EQ(Lifecycle::copyConstructor, 0);
    EXPECT_EQ(Lifecycle::moveConstructor, 0);
    EXPECT_EQ(Lifecycle::destructor, 0);

    buffer.emplace();
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(Lifecycle::defaultConstructor, 1);
    EXPECT_EQ(Lifecycle::copyConstructor, 0);
    EXPECT_EQ(Lifecycle::moveConstructor, 0);
    EXPECT_EQ(Lifecycle::destructor, 0);

    buffer.push(Lifecycle());
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(Lifecycle::defaultConstructor, 2);
    EXPECT_EQ(Lifecycle::copyConstructor, 0);
    EXPECT_EQ(Lifecycle::moveConstructor, 1);
    EXPECT_EQ(Lifecycle::destructor, 1);

    auto obj = Lifecycle();
    EXPECT_EQ(Lifecycle::defaultConstructor, 3);
    buffer.push(obj);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(Lifecycle::defaultConstructor, 3);
    EXPECT_EQ(Lifecycle::copyConstructor, 1);
    EXPECT_EQ(Lifecycle::moveConstructor, 1);
    EXPECT_EQ(Lifecycle::destructor, 1);

    buffer.pop();
    EXPECT_EQ(Lifecycle::defaultConstructor, 3);
    EXPECT_EQ(Lifecycle::copyConstructor, 1);
    EXPECT_EQ(Lifecycle::moveConstructor, 1);
    EXPECT_EQ(Lifecycle::destructor, 2);

    buffer.clear();
    EXPECT_EQ(Lifecycle::destructor, 4);
}
