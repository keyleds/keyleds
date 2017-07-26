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
/** @file
 * @brief C++ wrapper for libyaml
 *
 * This simple wrapper presents a C++ interface for parsing YAML.
 * To use it, inherit YAMLParser and implement all methods. parseFile will then
 * invoke them while parsing the event stream of the specified file.
 */
#ifndef TOOLS_YAML_PARSER_H_A07026A5
#define TOOLS_YAML_PARSER_H_A07026A5

#include <iosfwd>
#include <stdexcept>
#include <string>

/** Abstract YAML parser
 *
 * Implements YAML parsing functionnality. It should be derived and hooks
 * have to be implemented to respond to events. Invoking parse() will emit relevant
 * events as it goes through a YAML-formatted input stream.
 */
class YAMLParser
{
public:
    typedef unsigned int line_t;
    typedef unsigned int col_t;

    class ParseError : public std::runtime_error
    {
    public:
                    ParseError(const std::string & what, line_t line, col_t col)
                        : std::runtime_error(makeMessage(what, line, col)) {}
                    ParseError(const std::string & what, line_t line, col_t col,
                               const std::string & context, line_t ctx_line)
                        : std::runtime_error(makeMessage(what, line, col, context, ctx_line)) {}
    private:
        static std::string makeMessage(const std::string & what, line_t line, col_t col);
        static std::string makeMessage(const std::string & what, line_t line, col_t col,
                                       const std::string & context, line_t ctx_line);
    };

public:
    virtual         ~YAMLParser() {}
    virtual void    parse(std::istream & stream);

protected:
    virtual void    streamStart() = 0;
    virtual void    streamEnd() = 0;
    virtual void    documentStart() = 0;
    virtual void    documentEnd() = 0;
    virtual void    sequenceStart(const std::string & tag, const std::string & anchor) = 0;
    virtual void    sequenceEnd() = 0;
    virtual void    mappingStart(const std::string & tag, const std::string & anchor) = 0;
    virtual void    mappingEnd() = 0;
    virtual void    alias(const std::string & anchor) = 0;
    virtual void    scalar(const std::string & value, const std::string & tag,
                           const std::string & anchor) = 0;

    ParseError      makeError(const std::string & what) const
                    { return ParseError(what, m_line, m_column); }
private:
    line_t          m_line;
    col_t           m_column;
};

#endif
