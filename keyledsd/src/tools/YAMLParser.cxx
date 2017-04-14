/** C++ wrapper for libYAML
 *
 * Internally use [libYAML](http://pyyaml.org/wiki/LibYAML) to parse files and
 * pass generated events to a polymorphic parser object.
 *
 * Parse errors are converted into YAMLParser::ParseError exceptions.
 */
#include <yaml.h>
#include <istream>
#include <memory>
#include <sstream>
#include "tools/YAMLParser.h"

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
    Parser(std::istream & stream)
    {
        m_done = false;
        m_hasEvent = false;
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

    bool done() const { return m_done; }

private:
    static int readHandler(void * stream_ptr, unsigned char * buffer,
                           size_t size, size_t * size_read)
    {
        std::istream & stream = *reinterpret_cast<std::istream*>(stream_ptr);
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
    if (str == NULL) { return ""; }
    return reinterpret_cast<const char *>(str);
}

/****************************************************************************/

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

std::string YAMLParser::ParseError::makeMessage(const std::string & what, size_t line, size_t col)
{
    std::ostringstream error;
    error <<what <<" line " <<line + 1 <<" column " <<col + 1;
    return error.str();
}

std::string YAMLParser::ParseError::makeMessage(const std::string & what, size_t line, size_t col,
                                                const std::string & context, size_t ctx_line)
{
    std::ostringstream error;
    error <<what
          <<" line " <<line + 1 <<" column " <<col + 1
          <<" " <<context <<" from line " <<ctx_line + 1;
    return error.str();
}
