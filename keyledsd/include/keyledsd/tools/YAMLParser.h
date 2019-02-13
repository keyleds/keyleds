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

/****************************************************************************/

#include <iosfwd>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace keyleds::tools {

/****************************************************************************/

/** Abstract YAML parser
 *
 * Implements YAML parsing functionnality. It should be derived and hooks
 * have to be implemented to respond to events. Invoking parse() will emit relevant
 * events as it goes through a YAML-formatted input stream.
 */
class YAMLParser
{
public:
    using line_t = std::size_t;
    using col_t = std::size_t;

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
    virtual         ~YAMLParser() = 0;
    virtual void    parse(std::istream & stream);

protected:
    // Those methods are called while parsing
    virtual void    streamStart() = 0;
    virtual void    streamEnd() = 0;
    virtual void    documentStart() = 0;
    virtual void    documentEnd() = 0;
    virtual void    sequenceStart(std::string_view tag, std::string_view anchor) = 0;
    virtual void    sequenceEnd() = 0;
    virtual void    mappingStart(std::string_view tag, std::string_view anchor) = 0;
    virtual void    mappingEnd() = 0;
    virtual void    alias(std::string_view anchor) = 0;
    virtual void    scalar(std::string_view value, std::string_view tag,
                           std::string_view anchor) = 0;

    /// Builds a ParseError using internal state to set file position of the error
    ParseError      makeError(const std::string & what)
                    { return ParseError(what, m_line, m_column); }
private:
    line_t          m_line;     ///< Line of token being parsed. Only in event methods.
    col_t           m_column;   ///< Column of token being parsed. Only in event methods.
};

/****************************************************************************/

/** State-stack YAML parser
 * Tracks current position in YAML document through a stack of states.
 */
class StackYAMLParser : public YAMLParser
{
public:
    void                addScalarAlias(std::string anchor, std::string value);
    const std::string & getScalarAlias(std::string_view anchor);
    template<class T> T & as() noexcept { return static_cast<T&>(*this); }

protected:
    class State;
    class MappingState;
protected:
    explicit StackYAMLParser(std::unique_ptr<State> initial);
            ~StackYAMLParser() override = 0;

    State & finalState();
    template <typename T> T & finalState();

    void streamStart() override {}
    void streamEnd() override {}
    void documentStart() override {}
    void documentEnd() override {}
    void sequenceStart(std::string_view tag, std::string_view anchor) override;
    void sequenceEnd() override;
    void mappingStart(std::string_view tag, std::string_view anchor) override;
    void mappingEnd() override;
    void alias(std::string_view anchor) override;
    void scalar(std::string_view value, std::string_view, std::string_view anchor) override;

    ParseError          makeError(const std::string & what);

private:
    using state_ptr = std::unique_ptr<State>;
    std::stack<state_ptr, std::vector<state_ptr>>       m_state;
    std::vector<std::pair<std::string, std::string>>    m_scalarAliases;
};


class StackYAMLParser::State
{
public:
               State(const State &) = delete;
    State &    operator=(const State &) = delete;
    virtual    ~State() = 0;

    virtual std::unique_ptr<State> sequenceStart(StackYAMLParser &, std::string_view anchor);
    virtual std::unique_ptr<State> mappingStart(StackYAMLParser &, std::string_view anchor);
    virtual void subStateEnd(StackYAMLParser &, State &);
    virtual void alias(StackYAMLParser &, std::string_view anchor);
    virtual void scalar(StackYAMLParser &, std::string_view value, std::string_view anchor);

    virtual void print(std::ostream &) const = 0;

    template<class T> T & as() noexcept { return static_cast<T&>(*this); }
protected:
    State() = default;
};

class StackYAMLParser::MappingState : public State
{
public:
    std::unique_ptr<State> sequenceStart(StackYAMLParser &, std::string_view anchor) final override;
    std::unique_ptr<State> mappingStart(StackYAMLParser &, std::string_view anchor) final override;
    void alias(StackYAMLParser &, std::string_view) final override;
    void scalar(StackYAMLParser &, std::string_view value, std::string_view) final override;
    void subStateEnd(StackYAMLParser &, State &) override;
protected:
    virtual std::unique_ptr<State> sequenceEntry(StackYAMLParser &, std::string_view key, std::string_view anchor);
    virtual std::unique_ptr<State> mappingEntry(StackYAMLParser &, std::string_view key, std::string_view anchor);
    virtual void aliasEntry(StackYAMLParser &, std::string_view key, std::string_view anchor);
    virtual void scalarEntry(StackYAMLParser &, std::string_view key,
                             std::string_view value, std::string_view anchor);
protected:
    const std::string & currentKey() const { return m_currentKey; }
private:
    std::string m_currentKey;
};

/****************************************************************************/

template <typename T> T & StackYAMLParser::finalState()
{
    return finalState().as<T>();
}

} // namespace keyleds::tools

#endif
