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
#include <unistd.h>
#include <iostream>
#include <map>
#include "logging.h"



Logger::Logger(std::string name, Policy * policy)
 : m_name(name),
   m_policy(policy)
{
    if (m_policy != nullptr) { m_policy->open(m_name); }
}

Logger::~Logger()
{
    if (m_policy != nullptr) { m_policy->close(); }
}

void Logger::setPolicy(Policy * policy)
{
    if (m_policy != nullptr) { m_policy->close(); }
    m_policy = policy;
    if (m_policy != nullptr) { m_policy->open(m_name); }
}

Logger::Policy & Logger::defaultPolicy()
{
    static FilePolicy policy = FilePolicy(STDERR_FILENO);
    return policy;
}

/****************************************************************************/

Logger::Policy::~Policy() {}

/****************************************************************************/

void StreamPolicy::write(Logger::level_t, const std::string & name, const std::string & msg)
{
    m_stream <<name <<": " <<msg <<std::endl;
}

/****************************************************************************/

static const std::map<Logger::level_t, std::string> levels = {
    { Logger::critical::value(), "\033[1;31m<C>\033[;39m" },
    { Logger::error::value(), "\033[1;31m<E>\033[;39m" },
    { Logger::warning::value(), "\033[33m<W>\033[39m" },
    { Logger::info::value(), "\033[1m<I>\033[m" },
    { Logger::debug::value(), "\033[2m<D>\033[m" }
};
static const std::string nameEnter = "\033[1m";
static const std::string nameExit = "\033[m";

FilePolicy::FilePolicy(int fd)
 : m_fd(fd),
   m_tty(isatty(fd) == 1)
{
}

void FilePolicy::write(Logger::level_t level, const std::string & name, const std::string & msg)
{
    std::ostringstream buffer;
    if (m_tty) {
        buffer <<levels.at(level)
               <<nameEnter <<name <<':' <<nameExit
               <<' ' <<msg <<'\n';
    } else {
        buffer <<'<' <<level <<'>' <<name <<": " <<msg <<'\n';
    }

    const auto & data = buffer.str();

    size_t todo = data.size();
    size_t done = 0;

    while (todo > 0) {
        ssize_t written = ::write(m_fd, data.c_str() + done, todo);
#ifdef NDEBUG
        if (written < 0) { break; }
#else
        if (written < 0) { throw std::runtime_error("failed to write log"); }
#endif
        todo -= written;
        done += written;
    }
}
