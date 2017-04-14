/** C++ wrapper for libyaml
 *
 * This simple wrapper presents a C++ interface for parsing YAML.
 * To use it, inherit YAMLParser and implement all methods. parseFile will then
 * invoke them while parsing the event stream of the specified file.
 */
#ifndef TOOLS_YAML_PARSER_H
#define TOOLS_YAML_PARSER_H

#include <exception>
#include <istream>
#include <string>

class YAMLParser
{
public:
    class ParseError : public std::runtime_error
    {
    public:
                    ParseError(const std::string & what, size_t line, size_t col)
                        : std::runtime_error(makeMessage(what, line, col)) {}
                    ParseError(const std::string & what, size_t line, size_t col,
                               const std::string & context, size_t ctx_line)
                        : std::runtime_error(makeMessage(what, line, col, context, ctx_line)) {}
    private:
        static std::string makeMessage(const std::string & what, size_t line, size_t col);
        static std::string makeMessage(const std::string & what, size_t line, size_t col,
                                       const std::string & context, size_t ctx_line);
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
    size_t          m_line;
    size_t          m_column;
};

#endif
