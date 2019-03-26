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
#include "AsyncTest.h"

#include <thread>

using std::literals::chrono_literals::operator""ms;

namespace testing {

AsyncTest::AsyncTest()
{
    uv_loop_init(&loop);
}

AsyncTest::~AsyncTest()
{
    for (unsigned attempt = 0; uv_run(&loop, UV_RUN_NOWAIT) && attempt < 50; ++attempt) {
        std::this_thread::sleep_for(1ms);
    }
    if (uv_loop_close(&loop) < 0) {
        ADD_FAILURE() <<"Failed to close loop";
    }
}

bool AsyncTest::run_until(std::function<bool()> predicate, milliseconds timeout)
{
    uv_timer_t  timer;
    uv_timer_init(&loop, &timer);
    uv_timer_start(&timer, [](uv_timer_t * handle){ uv_stop(handle->loop); }, timeout.count(), 0);

    uv_prepare_t checker;
    uv_prepare_init(&loop, &checker);
    checker.data = &predicate;
    uv_prepare_start(&checker, [](uv_prepare_t * handle){
        if ((*reinterpret_cast<std::function<bool()>*>(handle->data))()) {
            uv_stop(handle->loop);
        }
    });

    uv_run(&loop, UV_RUN_DEFAULT);

    uv_close(reinterpret_cast<uv_handle_t*>(&checker), nullptr);
    uv_close(reinterpret_cast<uv_handle_t*>(&timer), nullptr);
    uv_run(&loop, UV_RUN_NOWAIT);

    return predicate();
}

}
