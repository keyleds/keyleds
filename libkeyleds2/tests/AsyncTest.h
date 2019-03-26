/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017-2019 Julien Hartmann, juli1.hartmann@gmail.com
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
#ifndef TESTS_ASYNCTEST_H_5012AA73
#define TESTS_ASYNCTEST_H_5012AA73

#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <uv.h>

namespace testing {

/****************************************************************************/

class AsyncTest : public Test
{
protected:
    using milliseconds = std::chrono::duration<unsigned, std::milli>;
    static constexpr auto defaultTimeout = milliseconds(100);

protected:
    AsyncTest();
    ~AsyncTest() override;

    bool run_until(std::function<bool()> predicate, milliseconds timeout = defaultTimeout);

protected:
    uv_loop_t   loop;
};

/****************************************************************************/

} // namespace testing

#endif
