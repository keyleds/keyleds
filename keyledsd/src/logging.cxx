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
#include "keyledsd/logging.h"
#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

using keyleds::logging::Configuration;
using keyleds::logging::Policy;
using keyleds::logging::FilePolicy;

/****************************************************************************/

Configuration::Configuration()
 : m_globalPolicy(&defaultPolicy())
{}

Configuration & Configuration::instance()
{
    static Configuration singleton;
    return singleton;
}

void Configuration::setPolicy(const Policy * policy)
{
    m_globalPolicy = (policy != nullptr ? policy : &defaultPolicy());
}

void Configuration::setPolicy(std::string name, const Policy * policy)
{
    using std::swap;
    auto it = std::find_if(m_policies.begin(), m_policies.end(),
                           [&name](const auto & item) { return item.first == name; });
    if (it == m_policies.end()) {
        if (policy) {
            m_policies.emplace_back(std::move(name), policy);
        }
    } else {
        if (policy) {
            it->second = policy;
        } else {
            if (it != m_policies.end() - 1) {
                swap(*it, m_policies.back());
                m_policies.pop_back();
            }
        }
    }
}

const Policy & Configuration::policyFor(const char * name) const
{
    auto it = std::find_if(m_policies.begin(), m_policies.end(),
                           [&name](const auto & item) { return item.first == name; });
    if (it != m_policies.end()) {
        return *it->second;
    }
    return *m_globalPolicy;
}

const Policy & Configuration::defaultPolicy()
{
    static const auto policy = logging::FilePolicy(STDERR_FILENO, info::value);
    return policy;
}

/****************************************************************************/

Policy::~Policy() = default;

/****************************************************************************/

static constexpr std::array<const char *, 6> levels = {{
    "\033[1;31m<C>\033[;39m",
    "\033[1;31m<E>\033[;39m",
    "\033[33m<W>\033[39m",
    "\033[1m<I>\033[m",
    "\033[1m<I>\033[m",
    "\033[2m<D>\033[m"
}};
static const std::string nameEnter = "\033[1m";
static const std::string nameExit = "\033[m";

FilePolicy::FilePolicy(int fd, level_t minLevel, bool ownsFd)
 : m_fd(fd),
   m_ownsFd(ownsFd),
   m_tty(isatty(fd) == 1),
   m_minLevel(minLevel)
{
}

FilePolicy::~FilePolicy()
{
    if (m_ownsFd) { close(m_fd); }
}

/** Write a log entry to a file descriptor
 *
 * This hopes to be atomic by combining the whole message in one write(2) system
 * call. It is still possible that the write is partial though. In that case,
 * it will be repeated and might lead to mixed log entries. Still, this avoids
 * locking while logging.
 */
void FilePolicy::write(level_t level, const std::string & name, const std::string & msg) const
{
    if (level > m_minLevel) { return; }
    std::ostringstream buffer;
    if (m_tty) {
        buffer <<levels.at(level)
               <<nameEnter <<name <<':' <<nameExit
               <<' ' <<msg <<'\n';
    } else {
        buffer <<'<' <<level <<'>' <<name <<": " <<msg <<'\n';
    }

    const auto & data = buffer.str();

    std::size_t todo = data.size();
    std::size_t done = 0;

    while (todo > 0) {
        auto written = ::write(m_fd, data.c_str() + done, todo);
        if (written < 0) { break; } // Do not handle the error
        todo -= static_cast<std::size_t>(written);
        done += static_cast<std::size_t>(written);;
    }
}

