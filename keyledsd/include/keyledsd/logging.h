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
#ifndef LOGGING_H_2BAC1A63
#define LOGGING_H_2BAC1A63

#include <atomic>
#include <sstream>
#include <string>
#include <vector>

/****************************************************************************/

namespace keyleds::logging {

class Policy;
class Logger;
using level_t = unsigned int;

/****************************************************************************/

/** Logging configuration singleton
 *
 * Tracks all Logger instances and provides a way to set logging policy based
 * on logger name. Not thread-safe, which means threads may not instantiate
 * loggers or call setPolicy.
 */
class Configuration final
{
                    Configuration();
public:
                    Configuration(const Configuration &) = delete;
    Configuration & operator=(const Configuration &) = delete;
    static Configuration & instance();          ///< singleton instance

    /// Define global logging policy. Pass nullptr for default;
    void            setPolicy(const Policy *);

    /// Define local logging policy for module of given name. Pass nullptr to revert module
    /// to using global policy.
    void            setPolicy(std::string name, const Policy *);

    const Policy &  policyFor(const char * name) const;

private:
    static const Policy & defaultPolicy();      ///< Used when setPolicy(nullptr) is invoked.
private:
    std::atomic<const Policy *> m_globalPolicy; ///< Used by loggers with no policy. Never null.
    std::vector<std::pair<std::string, const Policy *>> m_policies;
};


/*****************************************************************************
* Policies
*****************************************************************************/

/** Policy interface
 *
 * A policy is an object that can write log entries down in some way. They are
 * not managed by the logging framework (they are passed as const pointers,
 * and the framework never creates nor deletes them, except for the single
 * Configuration::defaultPolicy. Policy instances must be thread-safe.
 */
class Policy
{
public:
    virtual bool    canSkip(level_t) const = 0;
    virtual void    write(level_t, const std::string & name, const std::string & msg) const = 0;
protected:
    ~Policy();
};

/****************************************************************************/

/** Logger policy that writes into a system file descriptor
 *
 * Will also use ECMA-48 color codes if it detects the file descriptor is
 * associated to an interactive terminal.
 */
class FilePolicy : public Policy
{
public:
                    FilePolicy(int fd, level_t, bool ownsFd = false);
    virtual         ~FilePolicy();
    bool            canSkip(level_t) const override;
    void            write(level_t, const std::string &, const std::string &) const override;
protected:
    const int       m_fd;           ///< File descriptor number
    bool            m_ownsFd;       ///< Whether to close(2) m_fd on destruction
    bool            m_tty;          ///< Whether m_fd is an interactive terminal
    level_t         m_minLevel;     ///< Minimum log level to write
};

/*****************************************************************************
* Logger
*****************************************************************************/

/** Main logging interface
 *
 * Tracks current module configuration and registers it to the global
 * configuration holder. Typically, one static Logger instance is created using
 * LOGGING macro at the top of each compilation unit.
 */
class Logger final
{
public:
    explicit constexpr Logger(const char * name) noexcept : m_name(name) {}
    constexpr const char * name() const { return m_name; }

    auto & policy() const
        { return Configuration::instance().policyFor(m_name); }
    void    print(level_t level, const std::string & msg) const
        { policy().write(level, m_name, msg); }
private:
    const char *    m_name;             ///< Module name (for prefixing log entries)
};

/*****************************************************************************
* Log levels and printing
*****************************************************************************/

/** Log severity description
 *
 * Describes log severity: associated value and printing method.
 */
template <level_t L> struct level {
    static constexpr level_t value = L;
    template <typename...Args> static void print(const Logger & logger, Args && ...args);
};
template <level_t L> constexpr level_t level<L>::value;


template <level_t L> template <typename...Args>
void level<L>::print(const Logger & logger, Args && ...args)
{
    auto & policy = logger.policy();
    if (policy.canSkip(L)) { return; }
    std::ostringstream buffer;
    (buffer << ... << args);
    policy.write(value, logger.name(), buffer.str());
}

// See man sd-daemon
using emergency = level<0u>;
using alert = level<1u>;
using critical = level<2u>;
using error = level<3u>;
using warning = level<4u>;
using notice = level<5u>;
using info = level<6u>;
using debug = level<7u>;

#ifdef NDEBUG
template <> template<typename...Args> void debug::print(const Logger &, Args &&...) {}
#endif

/****************************************************************************/
// Module interface

#define LOGGING(name) static constexpr auto l_logger = keyleds::logging::Logger(name)

#define CRITICAL(...)   keyleds::logging::critical::print(l_logger, __VA_ARGS__)
#define ERROR(...)      keyleds::logging::error::print(l_logger, __VA_ARGS__)
#define WARNING(...)    keyleds::logging::warning::print(l_logger, __VA_ARGS__)
#define NOTICE(...)     keyleds::logging::notice::print(l_logger, __VA_ARGS__)
#define INFO(...)       keyleds::logging::info::print(l_logger, __VA_ARGS__)
#define DEBUG(...)      keyleds::logging::debug::print(l_logger, __VA_ARGS__)

/****************************************************************************/

} // namespace keyleds::logging

#endif
