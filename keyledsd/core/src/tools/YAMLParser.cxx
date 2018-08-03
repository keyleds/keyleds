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
/** C++ wrapper for libYAML
 *
 * Internally use [libYAML](http://pyyaml.org/wiki/LibYAML) to parse files and
 * pass generated events to a polymorphic parser object.
 *
 * Parse errors are converted into YAMLParser::ParseError exceptions.
 */
#include "tools/YAMLParser.h"

#include <yaml.h>
#include <istream>
#include <sstream>

using tools::YAMLParser;

/****************************************************************************/
// Helpers

/** Wrapper class for yaml_parser_t object
 *
 * Using a class wrapper ensures all resources will be properly freed even
 * if an exception is throw during parsing.
 *
 * Be very careful of what can throw while modifying this class.
 */
class Parser final
{
public:
    explicit Parser(std::istream & stream)
     : m_done(false),
       m_hasEvent(false)
    {
        if (yaml_parser_initialize(&m_parser) == 0) {
            throw std::runtime_error("YAML parser initialization failed");
        }
        yaml_parser_set_input(&m_parser, readHandler, &stream);
    }

    ~Parser()
    {
        if (m_hasEvent) { yaml_event_delete(&event); }
        yaml_parser_delete(&m_parser);
    }

    void parseNext()
    {
        if (m_hasEvent) {
            yaml_event_delete(&event);
            m_hasEvent = false;
        }
        if (yaml_parser_parse(&m_parser, &event) == 0) {
            throw YAMLParser::ParseError(
                m_parser.problem,
                m_parser.problem_mark.line, m_parser.problem_mark.column,
                m_parser.context, m_parser.context_mark.line
            );
        }
        m_hasEvent = true;
        if (event.type == YAML_STREAM_END_EVENT) { m_done = true; }
    }

    bool done() const noexcept { return m_done; }

private:
    static int readHandler(void * stream_ptr, unsigned char * buffer,
                           std::size_t size, std::size_t * size_read) noexcept
    {
        auto & stream = *static_cast<std::istream*>(stream_ptr);
        if (!stream.eof()) {
            stream.read(reinterpret_cast<std::istream::char_type*>(buffer), size);
            *size_read = stream.gcount();
        } else {
            *size_read = 0;
        }
        return stream.bad() ? 0 : 1;
    }

public:
    yaml_event_t    event;

private:
    bool            m_done;         ///< Set when end of stream was reached
    yaml_parser_t   m_parser;       ///< libYAML parser object
    bool            m_hasEvent;     ///< Set when @ref event holds an event
};

static const char * toStr(const yaml_char_t * str) {
    if (str == nullptr) { return ""; }
    return reinterpret_cast<const char *>(str);
}

/****************************************************************************/

YAMLParser::~YAMLParser()
 {}

void YAMLParser::parse(std::istream & stream)
{
    auto parser = Parser(stream);

    while (!parser.done()) {
        parser.parseNext();
        m_line = parser.event.start_mark.line;
        m_column = parser.event.start_mark.column;

        switch (parser.event.type) {
        case YAML_STREAM_START_EVENT:   streamStart(); break;
        case YAML_STREAM_END_EVENT:     streamEnd(); break;
        case YAML_DOCUMENT_START_EVENT: documentStart(); break;
        case YAML_DOCUMENT_END_EVENT:   documentEnd(); break;
        case YAML_SEQUENCE_START_EVENT: {
            const auto & data = parser.event.data.sequence_start;
            sequenceStart(toStr(data.tag), toStr(data.anchor));
            break;
        }
        case YAML_SEQUENCE_END_EVENT:   sequenceEnd(); break;
        case YAML_MAPPING_START_EVENT: {
            const auto & data = parser.event.data.mapping_start;
            mappingStart(toStr(data.tag), toStr(data.anchor));
            break;
        }
        case YAML_MAPPING_END_EVENT:    mappingEnd(); break;
        case YAML_ALIAS_EVENT: {
            const auto & data = parser.event.data.alias;
            alias(toStr(data.anchor));
            break;
        }
        case YAML_SCALAR_EVENT: {
            const auto & data = parser.event.data.scalar;
            scalar(std::string(toStr(data.value), data.length),
                   toStr(data.tag), toStr(data.anchor));
            break;
        }
        default:
            throw std::logic_error("unexpected event type");
        }
    }
}

/****************************************************************************/

std::string YAMLParser::ParseError::makeMessage(const std::string & what, line_t line, col_t col)
{
    std::ostringstream error;
    error <<what <<" line " <<line + 1 <<" column " <<col + 1;
    return error.str();
}

std::string YAMLParser::ParseError::makeMessage(const std::string & what, line_t line, col_t col,
                                                const std::string & context, line_t ctx_line)
{
    std::ostringstream error;
    error <<what
          <<" line " <<line + 1 <<" column " <<col + 1
          <<" " <<context <<" from line " <<ctx_line + 1;
    return error.str();
}
