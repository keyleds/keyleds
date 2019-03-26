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
#ifndef LIBKEYLEDS_HIDPPENDPOINT_H_B9BDF291
#define LIBKEYLEDS_HIDPPENDPOINT_H_B9BDF291

#include "utils.h"
#include <chrono>
#include <functional>
#include <memory>

struct uv_loop_s;
using uv_loop_t = struct uv_loop_s;

namespace libkeyleds::hid {

/// A hidraw endpoint that sends and receives reports asynchronously
class Endpoint final
{
    class implementation;
public:
    struct Frame final {
        uint8_t *   data;
        unsigned    size;
    };
    using frame_filter = std::function<bool(Frame)>;
    using event_handler = std::function<void(int)>;
    using milliseconds = std::chrono::duration<unsigned, std::milli>;
public:
            Endpoint(uv_loop_t & loop, FileDescriptor, std::size_t maxReportSize);
            ~Endpoint();

    Endpoint & operator=(Endpoint &&) = default;

    void    setTimeout(milliseconds);

    void    registerFrameFilter(void *, frame_filter);
    void    unregisterFrameFilter(void *);

    bool    post(Frame, frame_filter, event_handler error) noexcept;
    void    flush();

private:
    std::unique_ptr<implementation> m_impl;
};

} // namespace keyleds::device

#endif
