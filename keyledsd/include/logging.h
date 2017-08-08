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

#include <iosfwd>
#include <map>
#include <sstream>
#include <string>

/****************************************************************************/

namespace logging {

class Policy;
class Logger;

typedef int level_t;
template <level_t L> struct level {
    static inline constexpr int value() { return L; }
};

typedef level<0> critical;
typedef level<1> error;
typedef level<2> warning;
typedef level<3> info;
typedef level<4> debug;

/****************************************************************************/

/****************************************************************************/

class Configuration final
{
    Configuration();
public:
    Configuration(const Configuration &) = delete;
    Configuration(Configuration &&) = delete;
    static Configuration & instance();

    void    registerLogger(Logger *);
    void    unregisterLogger(Logger *);

    void    setPolicy(Policy *);
    void    setPolicy(const std::string & name, Policy *);

    Policy & globalPolicy() const { return *m_globalPolicy; }

private:
    static Policy & defaultPolicy();
private:
    std::map<std::string, Logger *> m_loggers;
    Policy * m_globalPolicy;
};


/*****************************************************************************
* Policies
*****************************************************************************/

class Policy
{
public:
    virtual         ~Policy();
    virtual void    write(level_t, const std::string & name, const std::string & msg) = 0;
};

/****************************************************************************/

/// Logger policy that writes into a given generic stream
class StreamPolicy : public Policy
{
public:
                        StreamPolicy(std::ostream & stream, level_t);
    void                write(level_t, const std::string &, const std::string &) override;
protected:
    std::ostream &      m_stream;
    level_t             m_minLevel;
};

/****************************************************************************/

/// Logger policy that writes into a system file descriptor
class FilePolicy : public Policy
{
public:
                        FilePolicy(int fd, level_t);
    void                write(level_t, const std::string &, const std::string &) override;
protected:
    int                 m_fd;
    bool                m_tty;
    level_t             m_minLevel;
};

/*****************************************************************************
* Logger
*****************************************************************************/

class Logger final
{
public:
                        Logger(std::string name, Policy * policy = nullptr);
                        Logger(const Logger &) = delete;
                        ~Logger();

    const std::string & name() const { return m_name; }
    void                setPolicy(Policy * policy);

    template<typename L, typename...Args> void print(Args...args);

private:
                                          void print_bits(std::ostream &) {}
    template<typename T, typename...Args> void print_bits(std::ostream &, T, Args...);

private:
    const std::string   m_name;
    Policy *            m_policy;
};



template<typename L, typename...Args> void Logger::print(Args...args)
{
    std::ostringstream buffer;
    print_bits(buffer, args...);
    auto & policy = (m_policy != nullptr) ? *m_policy : Configuration::instance().globalPolicy();
    policy.write(L::value(), m_name, buffer.str());
}

template<typename T, typename...Args> void Logger::print_bits(std::ostream & out, T val, Args...args)
{
    out <<val;
    print_bits(out, args...);
}

#define LOGGING(name) static logging::Logger l_logger(name)

#define CRITICAL    l_logger.print<::logging::critical>
#define ERROR       l_logger.print<::logging::error>
#define WARNING     l_logger.print<::logging::warning>
#define INFO        l_logger.print<::logging::info>
#ifdef NDEBUG
#define DEBUG(...)
#else
#define DEBUG       l_logger.print<::logging::debug>
#endif

/****************************************************************************/

}

#endif
