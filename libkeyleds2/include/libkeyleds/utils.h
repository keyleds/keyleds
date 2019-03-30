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
#ifndef LIBKEYLEDS_UTILS_H_6A8DD5E3
#define LIBKEYLEDS_UTILS_H_6A8DD5E3

#include <utility>

namespace libkeyleds {

class FileDescriptor final
{
    using fd_type = int;
    static constexpr fd_type invalid_fd = -1;
public:
                FileDescriptor() = default;
                FileDescriptor(std::nullptr_t) {}
    explicit    FileDescriptor(fd_type fd) : m_fd(fd >= 0 ? fd : invalid_fd) {}
                FileDescriptor(FileDescriptor && other) { std::swap(m_fd, other.m_fd); }
                ~FileDescriptor() { this->reset(); }

    FileDescriptor & operator=(std::nullptr_t) noexcept
        { this->reset(); return *this; }
    FileDescriptor & operator=(FileDescriptor && rhs) noexcept
        { this->reset(); std::swap(m_fd, rhs.m_fd); return *this; }

    fd_type     release() noexcept
        { auto fd = m_fd; m_fd = invalid_fd; return fd; }
    void        reset(fd_type fd = invalid_fd) noexcept
        { if (*this) { this->do_close(m_fd); } m_fd = fd; }
    void        swap(FileDescriptor & rhs) noexcept { std::swap(m_fd, rhs.m_fd); }

    fd_type     get() const noexcept { return m_fd; }
    explicit    operator bool() const noexcept { return m_fd != invalid_fd; }

private:
    static void do_close(fd_type) noexcept;

    int         m_fd = invalid_fd;
};

} // namespace libkeyleds

#endif
