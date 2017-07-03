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
#include <sstream>

/****************************************************************************/

class Logger final
{
public:
    typedef int level_t;
    template <level_t L> struct level {
        static inline constexpr int value() { return L; }
    };
    typedef level<0> critical;
    typedef level<1> error;
    typedef level<2> warning;
    typedef level<3> info;
    typedef level<4> debug;

    class Policy
    {
    public:
        virtual             ~Policy() = 0;
        virtual void        open(const std::string &) {}
        virtual void        close() {}
        virtual void        write(level_t, const std::string & name, const std::string & msg) = 0;
    };
public:
                        Logger(std::string name, Policy * policy = nullptr);
                        Logger(const Logger &) = delete;
                        ~Logger();

    void                setPolicy(Policy * policy);

    template<typename L, typename...Args> void print(Args...args);

private:
                                          void print_bits(std::ostream &) {}
    template<typename T, typename...Args> void print_bits(std::ostream &, T, Args...);

private:
    const std::string   m_name;
    Policy *            m_policy;

    static Policy &     defaultPolicy();
};



template<typename L, typename...Args> void Logger::print(Args...args)
{
    std::ostringstream buffer;
    print_bits(buffer, args...);
    auto & policy = (m_policy != nullptr) ? *m_policy : defaultPolicy();
    policy.write(L::value(), m_name, buffer.str());
}

#ifdef NDEBUG
// template<typename...Args> void Logger::print<Logger::debug, Args...>(Args...args) {}
#endif

template<typename T, typename...Args> void Logger::print_bits(std::ostream & out, T val, Args...args)
{
    out <<val;
    print_bits(out, args...);
}

#define LOGGING(name) static Logger l_logger(name)

#define CRITICAL    l_logger.print<Logger::critical>
#define ERROR       l_logger.print<Logger::error>
#define WARNING     l_logger.print<Logger::warning>
#define INFO        l_logger.print<Logger::info>
#define DEBUG       l_logger.print<Logger::debug>

/****************************************************************************/

/// Logger policy that writes into a given generic stream
class StreamPolicy : public Logger::Policy
{
public:
                        StreamPolicy(std::ostream & stream)
                         : m_stream(stream) {}
    void                write(Logger::level_t, const std::string &, const std::string &) override;
protected:
    std::ostream &      m_stream;
};

/****************************************************************************/

/// Logger policy that writes into a system file descriptor
class FilePolicy : public Logger::Policy
{
public:
                        FilePolicy(int fd);
    void                write(Logger::level_t, const std::string &, const std::string &) override;
protected:
    int                 m_fd;
    bool                m_tty;
};

/****************************************************************************/

#endif
