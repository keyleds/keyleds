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

namespace logging {

class Policy;
class Logger;
using level_t = int;

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
    Configuration(Configuration &&) = delete;
    static Configuration & instance();

    void    registerLogger(Logger *);
    void    unregisterLogger(Logger *);

    void    setPolicy(const Policy *);
    void    setPolicy(const std::string & name, const Policy *);

    const Policy & globalPolicy() const { return *m_globalPolicy; }

private:
    static const Policy & defaultPolicy();      ///< Used when setPolicy(nullptr) is invoked.
private:
    std::vector<Logger *>       m_loggers;      ///< Currently known logger instances.
    std::atomic<const Policy *> m_globalPolicy; ///< Used by loggers with no policy. Never null.
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
    virtual void    write(level_t, const std::string & name, const std::string & msg) const = 0;
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
                    Logger(std::string name, const Policy * policy = nullptr);
                    ~Logger();

    const std::string & name() const { return m_name; }
    void            setPolicy(const Policy * policy);

    void            print(level_t, const std::string & msg);

private:
    const std::string   m_name;             ///< Module name (for prefixing log entries)
    std::atomic<const Policy *> m_policy;   ///< Per-module policy. May be null.
};

/*****************************************************************************
* Log levels and printing
*****************************************************************************/

/** Log severity description
 *
 * Describes log severity: associated value and printing method.
 */
template <level_t L> struct level {
    static constexpr int value = L;
    template <typename...Args> static void print(Logger & logger, Args && ...args);
};
template <level_t L> constexpr int level<L>::value;

/// @private
namespace detail {
    template<typename T> static inline void print_bits(std::ostream & out, T && val)
    {
        out << val;
    }

    template<typename T, typename...Args> static inline void print_bits(std::ostream & out,
                                                                        T && val, Args && ...args)
    {
        print_bits(out << val, std::forward<Args>(args)...);
    }
}

template <level_t L> template <typename...Args> void level<L>::print(Logger & logger, Args && ...args)
{
    std::ostringstream buffer;
    detail::print_bits(buffer, std::forward<Args>(args)...);
    logger.print(value, buffer.str());
}

// Must start from 0 and have no gaps
using critical = level<0>;
using error = level<1>;
using warning = level<2>;
using info = level<3>;
using verbose = level<4>;
using debug = level<5>;

#ifdef NDEBUG
template <> template<typename...Args> void debug::print(Logger &, Args &&...) {}
#endif

/****************************************************************************/
// Module interface

#define LOGGING(name) static logging::Logger l_logger(name)

#define CRITICAL(...)   logging::critical::print(l_logger, __VA_ARGS__)
#define ERROR(...)      logging::error::print(l_logger, __VA_ARGS__)
#define WARNING(...)    logging::warning::print(l_logger, __VA_ARGS__)
#define INFO(...)       logging::info::print(l_logger, __VA_ARGS__)
#define VERBOSE(...)    logging::verbose::print(l_logger, __VA_ARGS__)
#define DEBUG(...)      logging::debug::print(l_logger, __VA_ARGS__)

/****************************************************************************/

}

#endif
