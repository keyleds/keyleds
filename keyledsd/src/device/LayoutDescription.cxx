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
#include "keyledsd/device/LayoutDescription.h"

#include "config.h"
#include "keyledsd/logging.h"
#include "keyledsd/tools/Paths.h"
#include "keyledsd/tools/YAMLParser.h"
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <istream>
#include <limits>
#include <sstream>
#include <system_error>

LOGGING("layout");

using keyleds::device::LayoutDescription;

/****************************************************************************/
/** Builder class that creates a LayoutDescription object from a YAML file.
 *
 * It uses a variant of the State pattern where state gets pushed onto a stack
 * when entering a collection and popped back when exiting. Each state class
 * represent a type of object that can appear in the layout description file, and
 * knows how to interpret sub-items.
 */
class LayoutDescriptionParser final : public keyleds::tools::StackYAMLParser
{
    class SpuriousState;
    class SpuriousListState;
    class KeyState;
    class KeyListState;
    class KeyboardState;
    class KeyboardListState;
    class RootState;
    class InitialState;
public:
    LayoutDescriptionParser();
    LayoutDescription & result();

    using StackYAMLParser::parse;

private:
    unsigned long parseUInt(std::string_view str, unsigned long minimum, unsigned long maximum)
    {
        auto nullTerminated = std::string(str.data(), str.size());
        char * endPtr = nullptr;

        auto value = ::strtoul(nullTerminated.c_str(), &endPtr, 0);
        if (*endPtr != '\0') { throw makeError("expected an integer"); }
        if (value < minimum || maximum < value) {
            std::ostringstream buffer;
            buffer <<"value must be between " <<minimum <<" and " <<maximum;
            throw makeError(buffer.str());
        }
        return value;
    }

    template <typename T, T minimum = std::numeric_limits<T>::min(),
                          T maximum = std::numeric_limits<T>::max()>
    T parse(std::string_view str)
    {
        static_assert(std::numeric_limits<unsigned long>::min() <= minimum);
        static_assert(std::numeric_limits<unsigned long>::min() <= maximum);
        static_assert(std::numeric_limits<unsigned long>::max() >= minimum);
        static_assert(std::numeric_limits<unsigned long>::max() >= maximum);
        auto value = parseUInt(str, minimum, maximum);
        return T(value);
    }
};



/****************************************************************************/

class LayoutDescriptionParser::SpuriousState final : public MappingState
{
    using value_type = std::pair<LayoutDescription::block_type, LayoutDescription::code_type>;
public:
    void print(std::ostream & out) const override { out <<"spurious"; }

    void scalarEntry(StackYAMLParser & parser, std::string_view key,
                     std::string_view value, std::string_view anchor) override
    {
        auto & layoutParser = parser.as<LayoutDescriptionParser>();
        if (key == "zone") { m_zone = layoutParser.parse<LayoutDescription::block_type>(value); }
        else if (key == "code") { m_code = layoutParser.parse<LayoutDescription::code_type>(value); }
        else { MappingState::scalarEntry(parser, key, value, anchor); }
    }

    value_type result(LayoutDescriptionParser & parser) {
        if (!m_zone) { throw parser.makeError("missing zone in spurious entry"); }
        if (!m_code)  { throw parser.makeError("missing code in spurious entry"); }
        return { *m_zone, *m_code };
    }

private:
    std::optional<LayoutDescription::block_type>    m_zone;
    std::optional<LayoutDescription::code_type>     m_code;
};


class LayoutDescriptionParser::SpuriousListState : public State
{
    using value_type = LayoutDescription::pos_list;
public:
    void print(std::ostream & out) const override { out <<"spurious-list"; }

    std::unique_ptr<State> mappingStart(StackYAMLParser &, std::string_view) final override {
        return std::make_unique<SpuriousState>();
    }

    void subStateEnd(StackYAMLParser & parser, State & state) final override
    {
        m_value.emplace_back(state.as<SpuriousState>().result(parser.as<LayoutDescriptionParser>()));
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type  m_value;
};


class LayoutDescriptionParser::KeyState : public MappingState
{
    using value_type = LayoutDescription::Key;
public:
    void print(std::ostream & out) const override { out <<"key"; }

    void scalarEntry(StackYAMLParser & parser, std::string_view key,
                     std::string_view value, std::string_view anchor) override
    {
        auto & layoutParser = parser.as<LayoutDescriptionParser>();
        if (key == "code") { m_code = layoutParser.parse<LayoutDescription::code_type>(value); }
        else if (key == "x") { m_x = layoutParser.parse<unsigned>(value); }
        else if (key == "y") { m_y = layoutParser.parse<unsigned>(value); }
        else if (key == "width") { m_width = layoutParser.parse<unsigned>(value); }
        else if (key == "height") { m_height = layoutParser.parse<unsigned>(value); }
        else if (key == "glyph") {
            m_name.resize(value.size());
            std::transform(value.begin(), value.end(), m_name.begin(), ::toupper);
        }
        else { MappingState::scalarEntry(parser, key, value, anchor); }
    }

    value_type result(LayoutDescriptionParser & parser) {
        if (!m_code)       { throw parser.makeError("missing code in key entry"); }
        if (m_x == 0)      { throw parser.makeError("missing x value in key entry"); }
        if (m_y == 0)      { throw parser.makeError("missing y value in key entry"); }
        if (m_width == 0)  { throw parser.makeError("missing width in key entry"); }
        if (m_height == 0) { throw parser.makeError("missing height in key entry"); }
        return {
            0, *m_code,
            {m_x, m_y, m_x + m_width, m_y + m_height},
            m_name
        };
    }

private:
    std::optional<LayoutDescription::code_type> m_code;
    unsigned    m_x = 0, m_y = 0, m_width = 0, m_height = 0;
    std::string m_name;
};


class LayoutDescriptionParser::KeyListState : public State
{
    using value_type = LayoutDescription::key_list;
public:
    void print(std::ostream & out) const override { out <<"key-list"; }

    std::unique_ptr<State> mappingStart(StackYAMLParser &, std::string_view) final override {
        return std::make_unique<KeyState>();
    }

    void subStateEnd(StackYAMLParser & parser, State & state) final override
    {
        m_value.emplace_back(state.as<KeyState>().result(parser.as<LayoutDescriptionParser>()));
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type  m_value;
};


class LayoutDescriptionParser::KeyboardState final : public MappingState
{
    using value_type = LayoutDescription::key_list;
public:
    void print(std::ostream & out) const override { out <<"keyboard"; }

    void scalarEntry(StackYAMLParser & parser, std::string_view key,
                     std::string_view value, std::string_view anchor) override
    {
        auto & layoutParser = parser.as<LayoutDescriptionParser>();
        if (key == "zone") { m_zone = layoutParser.parse<LayoutDescription::block_type>(value); }
        else { MappingState::scalarEntry(parser, key, value, anchor); }
    }

    std::unique_ptr<State>
    sequenceEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) final override {
        if (key == "keys") { return std::make_unique<KeyListState>(); }
        return MappingState::sequenceEntry(parser, key, anchor);
    }

    void subStateEnd(StackYAMLParser & parser, State & state) final override
    {
        m_value = state.as<KeyListState>().result();
        MappingState::subStateEnd(parser, state);
    }

    value_type && result() {
        std::for_each(m_value.begin(), m_value.end(),
                      [zone=m_zone](auto & key) { key.block = zone; });
        return std::move(m_value);
    }

private:
    LayoutDescription::block_type   m_zone = 1;
    value_type                      m_value;
};


class LayoutDescriptionParser::KeyboardListState : public State
{
    using value_type = LayoutDescription::key_list;
public:
    void print(std::ostream & out) const override { out <<"keyboard-list"; }

    std::unique_ptr<State> mappingStart(StackYAMLParser &, std::string_view) final override {
        return std::make_unique<KeyboardState>();
    }

    void subStateEnd(StackYAMLParser &, State & state) final override
    {
        auto result = state.as<KeyboardState>().result();
        m_value.reserve(m_value.size() + result.size());
        std::move(result.begin(), result.end(), std::back_inserter(m_value));
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type  m_value;
};


class LayoutDescriptionParser::RootState final : public MappingState
{
    enum class SubState { Spurious, Keyboards };
public:
    using value_type = LayoutDescription;
public:
    void print(std::ostream & out) const override { out <<"root"; }

    std::unique_ptr<State>
    sequenceEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) override
    {
        if (key == "spurious") {
            m_currentSubState = SubState::Spurious;
            return std::make_unique<SpuriousListState>();
        }
        if (key == "keyboards") {
            m_currentSubState = SubState::Keyboards;
            return std::make_unique<KeyboardListState>();
        }
        return MappingState::sequenceEntry(parser, key, anchor);
    }

    void scalarEntry(StackYAMLParser & parser, std::string_view key,
                     std::string_view value, std::string_view anchor) override
    {
        if (key == "layout") { m_value.name = std::string(value); }
        else { MappingState::scalarEntry(parser, key, value, anchor); }
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        switch (m_currentSubState) {
        case SubState::Spurious:
            m_value.spurious = state.as<SpuriousListState>().result();
            break;
        case SubState::Keyboards:
            m_value.keys = state.as<KeyboardListState>().result();
            break;
        }
        MappingState::subStateEnd(parser, state);
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type  m_value;
    SubState    m_currentSubState;
};


class LayoutDescriptionParser::InitialState final : public State
{
    using value_type = LayoutDescription;
public:
    void print(std::ostream &) const override { }

    std::unique_ptr<State> mappingStart(StackYAMLParser &, std::string_view) override
    {
        return std::make_unique<RootState>();
    }

    void subStateEnd(StackYAMLParser &, State & state) override
    {
        m_value = state.as<RootState>().result();
    }

    value_type & result() { return m_value; }

private:
    value_type  m_value;
};

/****************************************************************************/

LayoutDescriptionParser::LayoutDescriptionParser()
 : StackYAMLParser(std::make_unique<InitialState>())
{
}

LayoutDescription & LayoutDescriptionParser::result()
{
    return finalState<InitialState>().result();
}

LayoutDescription LayoutDescription::parse(std::istream & stream)
{
    auto parser = LayoutDescriptionParser();
    try {
        parser.parse(stream);
    } catch (LayoutDescriptionParser::ParseError & error) {
        throw ParseError(error.what());
    }
    return std::move(parser.result());
}

LayoutDescription LayoutDescription::loadFile(const std::string & path)
{
    const auto prefixedPath = KEYLEDSD_DATA_PREFIX "/layouts/" + path;
    auto file = tools::paths::open<std::ifstream>(
        tools::paths::XDG::Data, prefixedPath, std::ios::binary
    );
    if (!file) { throw std::system_error(errno, std::generic_category()); }

    INFO("loading layout ", file->path);
    return parse(file->stream);
}
