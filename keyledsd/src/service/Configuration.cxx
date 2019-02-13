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
#include "keyledsd/service/Configuration.h"

#include "keyledsd/tools/Paths.h"
#include "keyledsd/tools/YAMLParser.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <fstream>
#include <istream>
#include <system_error>

using keyleds::service::Configuration;

/****************************************************************************/

struct Configuration::Profile::Lookup::Entry
{
    std::string key;        ///< context entry key
    std::string value;      ///< string representation of the regex
    std::regex  regex;      ///< regex to match context entry value against
};

/****************************************************************************/
/****************************************************************************/
/** Builder class that creates a Configuration object from a YAML file.
 *
 * It uses a variant of the State pattern where state gets pushed onto a stack
 * when entering a collection and popped back when exiting. Each state class
 * represent a type of object that can appear in the configuration file, and
 * knows how to interpret sub-items.
 */
class ConfigurationParser final : public keyleds::tools::StackYAMLParser
{
    class StringSequenceBuildState;
    class StringMappingBuildState;
    class KeyGroupListState;
    class EffectListState;
    class EffectGroupState;
    class EffectGroupListState;
    class ProfileState;
    class ProfileListState;
    class RootState;
    class InitialState;
public:
    ConfigurationParser();

    void addGroupAlias(std::string anchor, Configuration::KeyGroup::key_list);
    const Configuration::KeyGroup::key_list & getGroupAlias(std::string_view anchor);

    Configuration & result();

private:
    std::vector<std::pair<std::string, Configuration::KeyGroup::key_list>> m_groupAliases;
};

/****************************************************************************/
/****************************************************************************/
// Generic build states for string sequence and string map

class ConfigurationParser::StringSequenceBuildState final : public State
{
public:
    using value_type = std::vector<std::string>;
public:
    void print(std::ostream & out) const override { out <<"string-sequence"; }

    void alias(StackYAMLParser & parser, std::string_view anchor) override
    {
        m_value.emplace_back(parser.getScalarAlias(anchor));
    }

    void scalar(StackYAMLParser & parser, std::string_view value, std::string_view anchor) override
    {
        m_value.emplace_back(value);
        parser.addScalarAlias(std::string(anchor), std::string(value));
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};

class ConfigurationParser::StringMappingBuildState final : public MappingState
{
public:
    using value_type = std::vector<std::pair<std::string, std::string>>;
public:
    void print(std::ostream & out) const override { out <<"string-mapping"; }

    void aliasEntry(StackYAMLParser & parser, std::string_view key,
                    std::string_view anchor) override
    {
        m_value.emplace_back(key, parser.getScalarAlias(anchor));
    }

    void scalarEntry(StackYAMLParser & parser, std::string_view key,
                     std::string_view value, std::string_view anchor) override
    {
        m_value.emplace_back(key, value);
        if (!anchor.empty()) { parser.addScalarAlias(std::string(anchor), std::string(value)); }
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};


/****************************************************************************/
/****************************************************************************/
// Specific types for keyledsd configuration

/// Configuration builder state: withing a key group list
class ConfigurationParser::KeyGroupListState final : public MappingState
{
public:
    using value_type = Configuration::key_group_list;
public:
    void print(std::ostream & out) const override { out <<"group-list"; }

    std::unique_ptr<State> sequenceEntry(StackYAMLParser &, std::string_view, std::string_view anchor) override
    {
        m_currentAnchor = anchor;
        return std::make_unique<StringSequenceBuildState>();
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        auto & builder = parser.as<ConfigurationParser>();
        auto keys = state.as<StringSequenceBuildState>().result();
        for (auto & key : keys) {
            std::transform(key.begin(), key.end(), key.begin(), ::toupper);
        }

        if (!m_currentAnchor.empty()) { builder.addGroupAlias(m_currentAnchor, keys); }
        m_value.push_back({currentKey(), std::move(keys)});
        MappingState::subStateEnd(builder, state);
    }

    void aliasEntry(StackYAMLParser & parser, std::string_view key,
                    std::string_view anchor) override
    {
        auto & builder = parser.as<ConfigurationParser>();
        m_value.push_back({std::string(key), builder.getGroupAlias(anchor)});
    }

    value_type && result() { return std::move(m_value); }

private:
    std::string     m_currentAnchor;
    value_type      m_value;
};


/// Configuration builder state: within an effect group's plugin list
class ConfigurationParser::EffectListState final : public State
{
public:
    using value_type = Configuration::EffectGroup::effect_list;
public:
    void print(std::ostream & out) const override { out <<"plugin-list"; }

    std::unique_ptr<State> mappingStart(StackYAMLParser &, std::string_view) override
    {
        return std::make_unique<StringMappingBuildState>();
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        auto & builder = parser.as<ConfigurationParser>();
        auto conf = state.as<StringMappingBuildState>().result();
        auto it_name = std::find_if(conf.cbegin(), conf.cend(),
                                    [](auto & item) { return item.first == "effect" ||
                                                             item.first == "plugin"; });
        if (it_name == conf.end()) { throw builder.makeError("plugin configuration must have a name"); }

        auto name = it_name->second;
        conf.erase(it_name);

        m_value.push_back({std::move(name), std::move(conf)});
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type      m_value;
};

/// Configuration builder state: within an effect
class ConfigurationParser::EffectGroupState final: public MappingState
{
    using EffectGroup = Configuration::EffectGroup;
    enum class SubState { None, KeyGroupList, EffectList };
public:
    explicit EffectGroupState(std::string name) : m_name(std::move(name)) {}
    void print(std::ostream & out) const override { out <<"effect(" <<m_name <<')'; }

    std::unique_ptr<State>
    sequenceEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) override
    {
        if (key == "plugins") {
            m_currentSubState = SubState::EffectList;
            return std::make_unique<EffectListState>();
        }
        return MappingState::sequenceEntry(parser, key, anchor);
    }

    std::unique_ptr<State>
    mappingEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) override
    {
        if (key == "groups") {
            m_currentSubState = SubState::KeyGroupList;
            return std::make_unique<KeyGroupListState>();
        }
        return MappingState::mappingEntry(parser, key, anchor);
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        switch(m_currentSubState) {
        case SubState::KeyGroupList:
            m_keyGroups = state.as<KeyGroupListState>().result();
            break;
        case SubState::EffectList:
            m_effects = state.as<EffectListState>().result();
            break;
        default:
            assert(false);
        }
        MappingState::subStateEnd(parser, state);
    }

    EffectGroup result()
    {
        return {std::move(m_name), std::move(m_keyGroups), std::move(m_effects)};
    }

private:
    std::string                 m_name;
    EffectGroup::key_group_list m_keyGroups;
    EffectGroup::effect_list    m_effects;
    SubState                    m_currentSubState = SubState::None;
};

/// Configuration builder state: within an effect list
class ConfigurationParser::EffectGroupListState final: public MappingState
{
public:
    using value_type = Configuration::effect_group_list;
public:
    void print(std::ostream & out) const override { out <<"effect-map"; }

    std::unique_ptr<State> mappingEntry(StackYAMLParser &, std::string_view key, std::string_view) override
    {
        return std::make_unique<EffectGroupState>(std::string(key));
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        m_value.emplace_back(state.as<EffectGroupState>().result());
        MappingState::subStateEnd(parser, state);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};

/// Configuration builder state: within a profile
class ConfigurationParser::ProfileState final: public MappingState
{
    using value_type = Configuration::Profile;
    enum class SubState { None, Lookup, DeviceList, EffectGroupList };
public:
    explicit ProfileState(std::string name) : m_name(std::move(name)) {}
    void print(std::ostream & out) const override { out <<"profile(" <<m_name <<')'; }

    std::unique_ptr<State>
    sequenceEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) override
    {
        if (key == "devices") {
            m_currentSubState = SubState::DeviceList;
            return std::make_unique<StringSequenceBuildState>();
        }
        if (key == "effects") {
            m_currentSubState = SubState::EffectGroupList;
            return std::make_unique<StringSequenceBuildState>();
        }
        return MappingState::sequenceEntry(parser, key, anchor);
    }

    std::unique_ptr<State>
    mappingEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) override
    {
        auto & builder = parser.as<ConfigurationParser>();
        if (key == "lookup") {
            if (m_name == "default") {
                throw builder.makeError("default profile cannot have filters defined");
            }
            m_currentSubState = SubState::Lookup;
            return std::make_unique<StringMappingBuildState>();
        }
        return MappingState::mappingEntry(parser, key, anchor);
    }

    void scalarEntry(StackYAMLParser & parser, std::string_view key,
                     std::string_view value, std::string_view anchor) override
    {
        if (key == "effect")    { m_effectGroups = { std::string(value) }; return; }
        MappingState::scalarEntry(parser, key, value, anchor);
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        switch(m_currentSubState) {
        case SubState::Lookup:
            m_lookup = value_type::Lookup(
                state.as<StringMappingBuildState>().result()
            );
            break;
        case SubState::DeviceList:
            m_devices = state.as<StringSequenceBuildState>().result();
            break;
        case SubState::EffectGroupList:
            m_effectGroups = state.as<StringSequenceBuildState>().result();
            break;
        default:
            assert(false);
        }
        MappingState::subStateEnd(parser, state);
    }

    value_type result()
    {
        return {std::move(m_name), std::move(m_lookup),
                std::move(m_devices), std::move(m_effectGroups)};
    }

private:
    std::string                     m_name;
    value_type::Lookup              m_lookup;
    value_type::device_list         m_devices;
    value_type::effect_group_list   m_effectGroups;
    SubState                        m_currentSubState = SubState::None;
};

/// Configuration builder state: within a profile list
class ConfigurationParser::ProfileListState final : public MappingState
{
public:
    using value_type = std::vector<Configuration::Profile>;
public:
    void print(std::ostream & out) const override { out <<"profile-map"; }

    std::unique_ptr<State>
    mappingEntry(StackYAMLParser &, std::string_view key, std::string_view) override
    {
        return std::make_unique<ProfileState>(std::string(key));
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        m_value.emplace_back(state.as<ProfileState>().result());
        MappingState::subStateEnd(parser, state);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};


/// Configuration builder state: at document root
class ConfigurationParser::RootState final : public MappingState
{
    enum class SubState {
        None, Plugins, PluginPaths, Devices, KeyGroups, EffectGroups, Profiles
    };
public:
    using value_type = Configuration;
public:
    void print(std::ostream & out) const override { out <<"root"; }

    std::unique_ptr<State>
    sequenceEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) override
    {
        if (key == "plugins") {
            m_currentSubState = SubState::Plugins;
            return std::make_unique<StringSequenceBuildState>();
        }
        if (key == "plugin-paths") {
            m_currentSubState = SubState::PluginPaths;
            return std::make_unique<StringSequenceBuildState>();
        }
        return MappingState::sequenceEntry(parser, key, anchor);
    }

    std::unique_ptr<State>
    mappingEntry(StackYAMLParser & parser, std::string_view key, std::string_view anchor) override
    {
        if (key == "devices") {
            m_currentSubState = SubState::Devices;
            return std::make_unique<StringMappingBuildState>();
        }
        if (key == "groups") {
            m_currentSubState = SubState::KeyGroups;
            return std::make_unique<KeyGroupListState>();
        }
        if (key == "effects") {
            m_currentSubState = SubState::EffectGroups;
            return std::make_unique<EffectGroupListState>();
        }
        if (key == "profiles") {
            m_currentSubState = SubState::Profiles;
            return std::make_unique<ProfileListState>();
        }
        return MappingState::mappingEntry(parser, key, anchor);
    }

    void scalarEntry(StackYAMLParser & parser, std::string_view key,
                     std::string_view value, std::string_view anchor) override
    {
        if (key == "plugin-path") {
            m_value.pluginPaths = { std::string(value) };
        } else {
            MappingState::scalarEntry(parser, key, value, anchor);
        }
    }

    void subStateEnd(StackYAMLParser & parser, State & state) override
    {
        switch (m_currentSubState) {
        case SubState::Plugins:
            m_value.plugins = state.as<StringSequenceBuildState>().result();
            break;
        case SubState::PluginPaths:
            m_value.pluginPaths = state.as<StringSequenceBuildState>().result();
            break;
        case SubState::Devices:
            m_value.devices = state.as<StringMappingBuildState>().result();
            break;
        case SubState::KeyGroups:
            m_value.keyGroups = state.as<KeyGroupListState>().result();
            break;
        case SubState::EffectGroups:
            m_value.effectGroups = state.as<EffectGroupListState>().result();
            break;
        case SubState::Profiles:
            m_value.profiles = state.as<ProfileListState>().result();
            break;
        default:
            assert(false);
        }
        MappingState::subStateEnd(parser, state);
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type  m_value;
    SubState    m_currentSubState = SubState::None;
};


/// Configuration builder state: parsing just started
class ConfigurationParser::InitialState final : public State
{
    using value_type = Configuration;
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

ConfigurationParser::ConfigurationParser()
 : StackYAMLParser(std::make_unique<InitialState>())
{
}

void ConfigurationParser::addGroupAlias(std::string anchor, Configuration::KeyGroup::key_list value)
{
    m_groupAliases.emplace_back(std::move(anchor), std::move(value));
}

const Configuration::KeyGroup::key_list & ConfigurationParser::getGroupAlias(std::string_view anchor)
{
    auto it = std::find_if(m_groupAliases.begin(), m_groupAliases.end(),
                           [&anchor](const auto & alias) { return alias.first == anchor; });
    if (it == m_groupAliases.end()) {
        throw makeError("unknown anchor or anchor is not a key group");
    }
    return it->second;
}

Configuration & ConfigurationParser::result()
{
    return finalState<InitialState>().result();
}

/****************************************************************************/

Configuration Configuration::parse(std::istream & stream)
{
    auto parser = ConfigurationParser();
    try {
        parser.parse(stream);
    } catch (ConfigurationParser::ParseError & error) {
        throw ParseError(error.what());
    }
    return std::move(parser.result());
}

Configuration Configuration::loadFile(const std::string & path)
{
    auto file = tools::paths::open<std::ifstream>(
        tools::paths::XDG::Config, path, std::ios::binary
    );
    if (!file) { throw std::system_error(errno, std::generic_category()); }

    auto result = parse(file->stream);
    result.path = file->path;
    return result;
}

/****************************************************************************/

Configuration::Profile::Lookup::Lookup(string_map filters)
 : m_entries(buildRegexps(std::move(filters)))
{}

Configuration::Profile::Lookup::~Lookup() = default;

bool Configuration::Profile::Lookup::match(const string_map & context) const
{
    // Each entry in m_entries must match corresponding item in context
    return std::all_of(
        m_entries.cbegin(), m_entries.cend(),
        [&context](const auto & entry) {
            auto it = std::find_if(
                context.begin(), context.end(),
                [&entry](const auto & ctxEntry) { return ctxEntry.first == entry.key; }
            );
            const auto & value = it != context.end() ? it->second : std::string();
            return std::regex_match(value, entry.regex);
    });
}

Configuration::Profile::Lookup::entry_list
Configuration::Profile::Lookup::buildRegexps(string_map filters)
{
    entry_list result;
    result.reserve(filters.size());
    std::transform(filters.begin(), filters.end(), std::back_inserter(result),
                   [](auto & entry) {
                       auto regex = std::regex(entry.second,
                                               std::regex::nosubs | std::regex::optimize);
                       return Entry{
                           std::move(entry.first),
                           std::move(entry.second),
                           std::move(regex)
                       };
                   });
    return result;
}
