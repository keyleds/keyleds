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

#include <algorithm>
#include <cassert>
#include <istream>
#include <sstream>
#include <yaml.h>

using tools::YAMLParser;
using tools::StackYAMLParser;
using namespace std::literals::string_literals;

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
    {
        if (yaml_parser_initialize(&m_parser) == 0) {
            throw std::runtime_error("YAML parser initialization failed");
        }
        yaml_parser_set_input(&m_parser, readHandler, &stream);
    }

    Parser(const Parser &) = delete;
    Parser & operator=(const Parser &) = delete;

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
            stream.read(reinterpret_cast<std::istream::char_type*>(buffer), std::streamsize(size));
            *size_read = static_cast<std::size_t>(stream.gcount());
        } else {
            *size_read = 0u;
        }
        return stream.bad() ? 0 : 1;
    }

public:
    yaml_event_t    event;

private:
    yaml_parser_t   m_parser;               ///< libYAML parser object
    bool            m_done = false;         ///< Set when end of stream was reached
    bool            m_hasEvent = false;     ///< Set when @ref event holds an event
};

static std::string_view toStr(const yaml_char_t * str) {
    return str ? std::string_view(reinterpret_cast<const char *>(str))
               : std::string_view();
}

static std::string_view toStr(const yaml_char_t * str, std::size_t size) {
    return size > 0 ? std::string_view(reinterpret_cast<const char *>(str))
                    : std::string_view();
}

/****************************************************************************/

YAMLParser::~YAMLParser() = default;

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
            scalar(toStr(data.value, data.length), toStr(data.tag), toStr(data.anchor));
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

/****************************************************************************/

StackYAMLParser::StackYAMLParser(std::unique_ptr<State> initial)
{
    m_state.push(std::move(initial));
}

StackYAMLParser::~StackYAMLParser() = default;

StackYAMLParser::State & tools::StackYAMLParser::finalState()
{
    assert(m_state.size() == 1);
    return *m_state.top();
}

void StackYAMLParser::sequenceStart(std::string_view, std::string_view anchor)
{
    m_state.push(m_state.top()->sequenceStart(*this, anchor));
}

void StackYAMLParser::sequenceEnd()
{
    auto removed = std::move(m_state.top());
    m_state.pop();
    m_state.top()->subStateEnd(*this, *removed);
}

void StackYAMLParser::mappingStart(std::string_view, std::string_view anchor)
{
    m_state.push(m_state.top()->mappingStart(*this, anchor));
}

void StackYAMLParser::mappingEnd()
{
    auto removed = std::move(m_state.top());
    m_state.pop();
    m_state.top()->subStateEnd(*this, *removed);
}

void StackYAMLParser::alias(std::string_view anchor)
{
    m_state.top()->alias(*this, anchor);
}

void StackYAMLParser::scalar(std::string_view value, std::string_view, std::string_view anchor)
{
    m_state.top()->scalar(*this, value, anchor);
}

void StackYAMLParser::addScalarAlias(std::string anchor, std::string value)
{
    m_scalarAliases.emplace_back(std::move(anchor), std::move(value));
}

const std::string & StackYAMLParser::getScalarAlias(std::string_view anchor)
{
    auto it = std::find_if(m_scalarAliases.begin(), m_scalarAliases.end(),
                           [anchor](const auto & alias) { return alias.first == anchor; });
    if (it == m_scalarAliases.end()) {
        throw makeError("unknown anchor or invalid anchor target");
    }
    return it->second;
}

StackYAMLParser::ParseError StackYAMLParser::makeError(const std::string & what)
{
    auto state = std::vector<decltype(m_state)::value_type>();
    state.reserve(m_state.size());
    while (!m_state.empty()) {
        state.emplace_back(std::move(m_state.top()));
        m_state.pop();
    }
    std::ostringstream msg;
    msg <<what <<" in ";
    std::for_each(state.rbegin() + 1, state.rend(),
                  [&msg](const auto & statep){ msg <<'/'; statep->print(msg); });
    return YAMLParser::makeError(msg.str());
}

/****************************************************************************/

StackYAMLParser::State::~State() = default;

std::unique_ptr<StackYAMLParser::State>
StackYAMLParser::State::sequenceStart(StackYAMLParser & parser, std::string_view)
    { throw parser.makeError("unexpected sequence"s); }
std::unique_ptr<StackYAMLParser::State>
StackYAMLParser::State::mappingStart(StackYAMLParser & parser, std::string_view)
    { throw parser.makeError("unexpected mapping"s); }
void StackYAMLParser::State::subStateEnd(StackYAMLParser &, State &) {}
void StackYAMLParser::State::alias(StackYAMLParser & parser, std::string_view)
    { throw parser.makeError("unexpected alias"s); }
void StackYAMLParser::State::scalar(StackYAMLParser & parser, std::string_view, std::string_view)
    { throw parser.makeError("unexpected scalar"s); }


std::unique_ptr<StackYAMLParser::State>
StackYAMLParser::MappingState::sequenceStart(StackYAMLParser & parser, std::string_view anchor)
{
    if (!m_currentKey.empty()) { return sequenceEntry(parser, m_currentKey, anchor); }
    return State::sequenceStart(parser, anchor);
}

std::unique_ptr<StackYAMLParser::State>
StackYAMLParser::MappingState::mappingStart(StackYAMLParser & parser, std::string_view anchor)
{
    if (!m_currentKey.empty()) { return mappingEntry(parser, m_currentKey, anchor); }
    return State::mappingStart(parser, anchor);
}

void StackYAMLParser::MappingState::alias(StackYAMLParser & parser, std::string_view anchor)
{
    if (!m_currentKey.empty()) {
        aliasEntry(parser, m_currentKey, anchor);
        m_currentKey.clear();
    } else {
        m_currentKey = parser.getScalarAlias(anchor);
    }
}

void StackYAMLParser::MappingState::scalar(StackYAMLParser & parser, std::string_view value, std::string_view anchor)
{
    if (!m_currentKey.empty()) {
        scalarEntry(parser, m_currentKey, value, anchor);
        m_currentKey.clear();
    } else {
        m_currentKey = value;
    }
}

void StackYAMLParser::MappingState::subStateEnd(StackYAMLParser &, State &)
{
    m_currentKey.clear();
}

std::unique_ptr<StackYAMLParser::State>
StackYAMLParser::MappingState::sequenceEntry(StackYAMLParser & parser, std::string_view, std::string_view anchor)
{
    return State::sequenceStart(parser, anchor);
}

std::unique_ptr<StackYAMLParser::State>
StackYAMLParser::MappingState::mappingEntry(StackYAMLParser & parser, std::string_view, std::string_view anchor)
{
    return State::mappingStart(parser, anchor);
}

void StackYAMLParser::MappingState::aliasEntry(StackYAMLParser & parser, std::string_view, std::string_view anchor)
{
    return State::alias(parser, anchor);
}

void StackYAMLParser::MappingState::scalarEntry(StackYAMLParser & parser, std::string_view,
                                                std::string_view value, std::string_view anchor)
{
    return State::scalar(parser, value, anchor);
}
