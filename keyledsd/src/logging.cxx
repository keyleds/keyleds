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
#include <map>
#include <sstream>
#include <stdexcept>
#include "logging.h"

using logging::Configuration;
using logging::Policy;
using logging::Logger;

/****************************************************************************/

Configuration::Configuration()
 : m_globalPolicy(&defaultPolicy())
{}

Configuration & Configuration::instance()
{
    static Configuration singleton;
    return singleton;
}

void Configuration::registerLogger(Logger * logger)
{
    m_loggers[logger->name()] = logger;
}

void Configuration::unregisterLogger(Logger * logger)
{
    m_loggers.erase(logger->name());
}

void Configuration::setPolicy(const Policy * policy)
{
    m_globalPolicy = (policy != nullptr ? policy : &defaultPolicy());
}

void Configuration::setPolicy(const std::string & name, const Policy * policy)
{
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        it->second->setPolicy(policy);
    }
}

const Policy & Configuration::defaultPolicy()
{
    static const auto policy = logging::FilePolicy(STDERR_FILENO, info::value);
    return policy;
}

/****************************************************************************/

static const std::map<logging::level_t, std::string> levels = {
    { logging::critical::value, "\033[1;31m<C>\033[;39m" },
    { logging::error::value, "\033[1;31m<E>\033[;39m" },
    { logging::warning::value, "\033[33m<W>\033[39m" },
    { logging::info::value, "\033[1m<I>\033[m" },
    { logging::debug::value, "\033[2m<D>\033[m" }
};
static const std::string nameEnter = "\033[1m";
static const std::string nameExit = "\033[m";

logging::FilePolicy::FilePolicy(int fd, level_t minLevel, bool ownsFd)
 : m_fd(fd),
   m_ownsFd(ownsFd),
   m_tty(isatty(fd) == 1),
   m_minLevel(minLevel)
{
}

logging::FilePolicy::~FilePolicy()
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
void logging::FilePolicy::write(level_t level, const std::string & name, const std::string & msg) const
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
        ssize_t written = ::write(m_fd, data.c_str() + done, todo);
        if (written < 0) { break; } // Do not handle the error
        todo -= written;
        done += written;
    }
}

/****************************************************************************/

Logger::Logger(std::string name, const Policy * policy)
 : m_name(std::move(name)),
   m_policy(policy)
{
    Configuration::instance().registerLogger(this);
}

Logger::~Logger()
{
    Configuration::instance().unregisterLogger(this);
}

void Logger::setPolicy(const Policy * policy)
{
    m_policy.store(policy);
}

void Logger::print(level_t level, const std::string & msg)
{
    const auto * policy = m_policy.load();
    if (policy == nullptr) {
        policy = &Configuration::instance().globalPolicy();
    }
    policy->write(level, m_name, msg);
}
