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
#include "keyledsd/Configuration.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include "tools/Paths.h"
#include "tools/YAMLParser.h"
#include "logging.h"
#include "config.h"

using keyleds::Configuration;

/****************************************************************************/
/****************************************************************************/
/** Builder class that creates a Configuration object from a YAML file.
 *
 * It uses a variant of the State pattern where state gets pushed onto a stack
 * when entering a collection and popped back when exiting. Each state class
 * represent a type of object that can appear in the configuration file, and
 * knows how to interpret sub-items.
 *
 * Builder also keeps cross-state data such as aliases dictionary.
 */
class ConfigurationBuilder final : public tools::YAMLParser
{
public:
    class BuildState;
    using state_ptr = std::unique_ptr<BuildState>;
public:
    ConfigurationBuilder();

    void addScalarAlias(std::string anchor, std::string value);
    const std::string & getScalarAlias(const std::string & anchor);
    void addGroupAlias(std::string anchor, Configuration::KeyGroup::key_list);
    const Configuration::KeyGroup::key_list & getGroupAlias(std::string anchor);

    void streamStart() override {}
    void streamEnd() override {}
    void documentStart() override {}
    void documentEnd() override {}
    void sequenceStart(const std::string &, const std::string & anchor) override;
    void sequenceEnd() override;
    void mappingStart(const std::string &, const std::string & anchor) override;
    void mappingEnd() override;
    void alias(const std::string & anchor) override;
    void scalar(const std::string & value, const std::string &, const std::string & anchor) override;

    ParseError makeError(const std::string & what);
public:
    Configuration::path_list            m_pluginPaths;
    Configuration::device_map           m_devices;
    Configuration::key_group_list       m_keyGroups;
    Configuration::effect_group_list    m_effectGroups;
    Configuration::profile_list         m_profiles;

private:
    std::stack<state_ptr, std::vector<state_ptr>>                m_state;
    std::vector<std::pair<std::string, std::string>>             m_scalarAliases;
    std::vector<std::pair<std::string, Configuration::KeyGroup::key_list>> m_groupAliases;
};

/****************************************************************************/

/** Builder parsing state
 *
 * Tracks the current state of the configuration builder. Each possible state
 * must inherit this class and implement methods matching allowable events in
 * that state.
 */
class ConfigurationBuilder::BuildState
{
public:
    using state_ptr = std::unique_ptr<BuildState>;
    using state_type = unsigned int;

public:
    BuildState(state_type type = 0) : m_type(type) {}
    state_type type() const noexcept { return m_type; }
    virtual void print(std::ostream &) const = 0;

public:
    virtual state_ptr sequenceStart(ConfigurationBuilder & builder, const std::string &)
        { throw builder.makeError("unexpected sequence"); }
    virtual state_ptr mappingStart(ConfigurationBuilder & builder, const std::string &)
        { throw builder.makeError("unexpected mapping"); }
    virtual void subStateEnd(ConfigurationBuilder &, BuildState &) {}
    virtual void alias(ConfigurationBuilder & builder, const std::string &)
        { throw builder.makeError("unexpected alias"); }
    virtual void scalar(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected scalar"); }

    template<class T> T & as() noexcept { return static_cast<T&>(*this); }

private:
    state_type  m_type;         ///< State subtype, used by parent state to identify
                                ///  which state this object represents, when several
                                ///  substates use the same class.
};

std::ostream & operator<<(std::ostream & out, ConfigurationBuilder::BuildState & state)
{
    state.print(out);
    return out;
}

/****************************************************************************/
/****************************************************************************/
/// Generic inheritable build state for mappings
/** Pairs events two by two to associate a key scalar to the following event.
 */
class MappingBuildState : public ConfigurationBuilder::BuildState
{
public:
    MappingBuildState(state_type type) : BuildState(type) {}

    virtual state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected sequence"); }
    virtual state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected mapping"); }
    virtual void aliasEntry(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected alias"); }
    virtual void scalarEntry(ConfigurationBuilder & builder, const std::string &,
                             const std::string &, const std::string &)
        { throw builder.makeError("unexpected scalar"); }

    state_ptr sequenceStart(ConfigurationBuilder & builder, const std::string & anchor) final override
    {
        if (m_currentKey.empty()) { throw builder.makeError("unexpected sequence"); }
        return sequenceEntry(builder, m_currentKey, anchor);
    }

    state_ptr mappingStart(ConfigurationBuilder & builder, const std::string & anchor) final override
    {
        if (m_currentKey.empty()) { throw builder.makeError("unexpected mapping"); }
        return mappingEntry(builder, m_currentKey, anchor);
    }

    void subStateEnd(ConfigurationBuilder &, BuildState &) override { m_currentKey.clear(); }

    void alias(ConfigurationBuilder & builder, const std::string & anchor) final override
    {
        if (m_currentKey.empty()) {
            m_currentKey = builder.getScalarAlias(anchor);
        } else {
            aliasEntry(builder, m_currentKey, anchor);
            m_currentKey.clear();
        }
    }

    void scalar(ConfigurationBuilder & builder, const std::string & value,
                const std::string & anchor) final override
    {
        if (m_currentKey.empty()) {
            m_currentKey = value;
        } else {
            scalarEntry(builder, m_currentKey, value, anchor);
            m_currentKey.clear();
        }
    }

protected:
    const std::string & currentKey() const { return m_currentKey; }

private:
    std::string     m_currentKey;
};

/****************************************************************************/
// Generic build states for string sequence and string map

class StringSequenceBuildState final : public ConfigurationBuilder::BuildState
{
public:
    using value_type = std::vector<std::string>;
public:
    StringSequenceBuildState(state_type type = 0) : BuildState(type) {}
    void print(std::ostream & out) const override { out <<"string-sequence"; }

    void alias(ConfigurationBuilder & builder, const std::string & anchor) override
    {
        m_value.emplace_back(builder.getScalarAlias(anchor));
    }

    void scalar(ConfigurationBuilder & builder, const std::string & value,
                const std::string & anchor) override
    {
        m_value.emplace_back(value);
        builder.addScalarAlias(anchor, value);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};

class StringMappingBuildState final : public MappingBuildState
{
public:
    using value_type = std::vector<std::pair<std::string, std::string>>;
public:
    StringMappingBuildState(state_type type = 0) : MappingBuildState(type) {}
    void print(std::ostream & out) const override { out <<"string-mapping"; }

    void aliasEntry(ConfigurationBuilder & builder, const std::string & key,
                    const std::string & anchor) override
    {
        m_value.emplace_back(key, builder.getScalarAlias(anchor));
    }

    void scalarEntry(ConfigurationBuilder & builder, const std::string & key,
                     const std::string & value, const std::string & anchor) override
    {
        m_value.emplace_back(key, value);
        if (!anchor.empty()) { builder.addScalarAlias(anchor, value); }
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};


/****************************************************************************/
/****************************************************************************/
// Specific types for keyledsd configuration

/// Configuration builder state: withing a key group list
class KeyGroupListState final : public MappingBuildState
{
public:
    using value_type = Configuration::key_group_list;
public:
    KeyGroupListState(state_type type) : MappingBuildState(type) {}
    void print(std::ostream & out) const override { out <<"group-list"; }

    state_ptr sequenceEntry(ConfigurationBuilder &, const std::string &,
                            const std::string & anchor) override
    {
        m_currentAnchor = anchor;
        return std::make_unique<StringSequenceBuildState>();
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        auto keys = state.as<StringSequenceBuildState>().result();
        for (auto & key : keys) {
            std::transform(key.begin(), key.end(), key.begin(), ::toupper);
        }

        if (!m_currentAnchor.empty()) { builder.addGroupAlias(m_currentAnchor, keys); }
        m_value.emplace_back(currentKey(), std::move(keys));
        MappingBuildState::subStateEnd(builder, state);
    }

    void aliasEntry(ConfigurationBuilder & builder, const std::string & key,
                    const std::string & anchor) override
    {
        m_value.emplace_back(key, builder.getGroupAlias(anchor));
    }

    value_type && result() { return std::move(m_value); }

private:
    std::string     m_currentAnchor;
    value_type      m_value;
};


/// Configuration builder state: within an effect group's plugin list
class EffectListState final : public ConfigurationBuilder::BuildState
{
public:
    using value_type = Configuration::EffectGroup::effect_list;
public:
    EffectListState(state_type type) : BuildState(type) {}
    void print(std::ostream & out) const override { out <<"plugin-list"; }

    state_ptr mappingStart(ConfigurationBuilder &, const std::string &) override
    {
        return std::make_unique<StringMappingBuildState>();
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        auto conf_map = state.as<StringMappingBuildState>().result();
        auto it_name = std::find_if(conf_map.cbegin(), conf_map.cend(),
                                    [](auto & item) { return item.first == "effect" ||
                                                             item.first == "plugin"; });
        if (it_name == conf_map.end()) { throw builder.makeError("plugin configuration must have a name"); }

        auto name = it_name->second;
        conf_map.erase(it_name);

        m_value.emplace_back(std::move(name), std::move(conf_map));
    }

    void scalar(ConfigurationBuilder &, const std::string & value, const std::string &) override
    {
        m_value.emplace_back(Configuration::Effect(value, Configuration::Effect::conf_map()));
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type      m_value;
};

/// Configuration builder state: within an effect
class EffectGroupState final: public MappingBuildState
{
    using EffectGroup = Configuration::EffectGroup;
    enum SubState : state_type { KeyGroupList, EffectList };
public:
    EffectGroupState(std::string name, state_type type = 0)
      : MappingBuildState(type), m_name(name) {}
    void print(std::ostream & out) const override { out <<"effect(" <<m_name <<')'; }

    state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string & key,
                            const std::string & anchor) override
    {
        if (key == "plugins") { return std::make_unique<EffectListState>(SubState::EffectList); }
        return MappingBuildState::sequenceEntry(builder, key, anchor);
    }

    state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string & anchor) override
    {
        if (key == "groups") { return std::make_unique<KeyGroupListState>(SubState::KeyGroupList); }
        return MappingBuildState::mappingEntry(builder, key, anchor);
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        switch(state.type()) {
            case SubState::KeyGroupList: {
                m_keyGroups = state.as<KeyGroupListState>().result();
                break;
            }
            case SubState::EffectList: {
                m_effects = state.as<EffectListState>().result();
                break;
            }
        }
        MappingBuildState::subStateEnd(builder, state);
    }

    EffectGroup result()
    {
        return {std::move(m_name), std::move(m_keyGroups), std::move(m_effects)};
    }

private:
    std::string                 m_name;
    EffectGroup::key_group_list m_keyGroups;
    EffectGroup::effect_list    m_effects;
};

/// Configuration builder state: within an effect list
class EffectGroupListState final: public MappingBuildState
{
public:
    using value_type = Configuration::effect_group_list;
public:
    EffectGroupListState(state_type type) : MappingBuildState(type) {}
    void print(std::ostream & out) const override { out <<"effect-map"; }

    state_ptr mappingEntry(ConfigurationBuilder &, const std::string & key, const std::string &) override
    {
        return std::make_unique<EffectGroupState>(key);
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        m_value.emplace_back(state.as<EffectGroupState>().result());
        MappingBuildState::subStateEnd(builder, state);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};

/// Configuration builder state: within a profile
class ProfileState final: public MappingBuildState
{
    using Profile = Configuration::Profile;
    enum SubState : state_type { Lookup, DeviceList, EffectGroupList };
public:
    ProfileState(std::string name, state_type type = 0)
      : MappingBuildState(type), m_name(name) {}
    void print(std::ostream & out) const override { out <<"profile(" <<m_name <<')'; }

    state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string & key,
                            const std::string & anchor) override
    {
        if (key == "devices") { return std::make_unique<StringSequenceBuildState>(SubState::DeviceList); }
        if (key == "effects") { return std::make_unique<StringSequenceBuildState>(SubState::EffectGroupList); }
        return MappingBuildState::sequenceEntry(builder, key, anchor);
    }

    state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string & anchor) override
    {
        if (key == "lookup") {
            if (m_name == "default") {
                throw builder.makeError("default profile cannot have filters defined");
            }
            return std::make_unique<StringMappingBuildState>(SubState::Lookup);
        }
        return MappingBuildState::mappingEntry(builder, key, anchor);
    }

    void scalarEntry(ConfigurationBuilder & builder, const std::string & key,
                     const std::string & value, const std::string & anchor) override
    {
        if (key == "effect")    { m_effectGroups = { value }; return; }
        MappingBuildState::scalarEntry(builder, key, value, anchor);
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        switch(state.type()) {
            case SubState::Lookup: {
                m_lookup = Profile::Lookup(
                    state.as<StringMappingBuildState>().result()
                );
                break;
            }
            case SubState::DeviceList: {
                m_devices = state.as<StringSequenceBuildState>().result();
                break;
            }
            case SubState::EffectGroupList: {
                m_effectGroups = state.as<StringSequenceBuildState>().result();
                break;
            }
        }
        MappingBuildState::subStateEnd(builder, state);
    }

    Profile result()
    {
        return {std::move(m_name), std::move(m_lookup),
                std::move(m_devices), std::move(m_effectGroups)};
    }

private:
    std::string                 m_name;
    Profile::Lookup             m_lookup;
    Profile::device_list        m_devices;
    Profile::effect_group_list  m_effectGroups;
};

/// Configuration builder state: within a profile list
class ProfileListState final : public MappingBuildState
{
public:
    using value_type = std::vector<Configuration::Profile>;
public:
    ProfileListState(state_type type) : MappingBuildState(type) {}
    void print(std::ostream & out) const override { out <<"profile-map"; }

    state_ptr mappingEntry(ConfigurationBuilder &, const std::string & key, const std::string &) override
    {
        return std::make_unique<ProfileState>(key);
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        m_value.emplace_back(state.as<ProfileState>().result());
        MappingBuildState::subStateEnd(builder, state);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};


/// Configuration builder state: at document root
class RootState final : public MappingBuildState
{
    enum SubState : state_type { PluginPaths, Layouts, Devices, KeyGroups, EffectGroups, Profiles };
public:
    RootState() : MappingBuildState(0) {}
    void print(std::ostream & out) const override { out <<"root"; }

    state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string & anchor) override
    {
        if (key == "plugins") {
            return std::make_unique<StringSequenceBuildState>(SubState::PluginPaths);
        }
        return MappingBuildState::sequenceEntry(builder, key, anchor);
    }

    state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string & anchor) override
    {
        if (key == "devices")   { return std::make_unique<StringMappingBuildState>(SubState::Devices); }
        if (key == "groups")    { return std::make_unique<KeyGroupListState>(SubState::KeyGroups); }
        if (key == "effects")   { return std::make_unique<EffectGroupListState>(SubState::EffectGroups); }
        if (key == "profiles")  { return std::make_unique<ProfileListState>(SubState::Profiles); }
        return MappingBuildState::mappingEntry(builder, key, anchor);
    }

    void scalarEntry(ConfigurationBuilder & builder, const std::string & key,
                     const std::string & value, const std::string & anchor) override
    {
        if (key == "plugins")  { builder.m_pluginPaths = { value }; }
        else MappingBuildState::scalarEntry(builder, key, value, anchor);
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        switch (state.type()) {
        case SubState::PluginPaths:
            builder.m_pluginPaths = state.as<StringSequenceBuildState>().result();
            break;
        case SubState::Devices:
            builder.m_devices = state.as<StringMappingBuildState>().result();
            break;
        case SubState::KeyGroups:
            builder.m_keyGroups = state.as<KeyGroupListState>().result();
            break;
        case SubState::EffectGroups:
            builder.m_effectGroups = state.as<EffectGroupListState>().result();
            break;
        case SubState::Profiles:
            builder.m_profiles = state.as<ProfileListState>().result();
            break;
        }
        MappingBuildState::subStateEnd(builder, state);
    }
};


/// Configuration builder state: parsing just started
class InitialState final : public ConfigurationBuilder::BuildState
{
public:
    void print(std::ostream &) const override { }

    state_ptr mappingStart(ConfigurationBuilder &, const std::string &) override
    {
        return std::make_unique<RootState>();
    }
};

/****************************************************************************/

ConfigurationBuilder::ConfigurationBuilder()
{
    m_state.push(std::make_unique<InitialState>());
}

void ConfigurationBuilder::addScalarAlias(std::string anchor, std::string value) {
    m_scalarAliases.emplace_back(std::move(anchor), std::move(value));
}

const std::string & ConfigurationBuilder::getScalarAlias(const std::string & anchor) {
    auto it = std::find_if(m_scalarAliases.begin(), m_scalarAliases.end(),
                           [anchor](const auto & alias) { return alias.first == anchor; });
    if (it == m_scalarAliases.end()) {
        throw makeError("unknown anchor or invalid anchor target");
    }
    return it->second;
}

void ConfigurationBuilder::addGroupAlias(std::string anchor, Configuration::KeyGroup::key_list value) {
    m_groupAliases.emplace_back(std::move(anchor), std::move(value));
}

const Configuration::KeyGroup::key_list & ConfigurationBuilder::getGroupAlias(std::string anchor) {
    auto it = std::find_if(m_groupAliases.begin(), m_groupAliases.end(),
                           [anchor](const auto & alias) { return alias.first == anchor; });
    if (it == m_groupAliases.end()) {
        throw makeError("unknown anchor or anchor is not a key group");
    }
    return it->second;
}

void ConfigurationBuilder::sequenceStart(const std::string &, const std::string & anchor) {
    m_state.push(m_state.top()->sequenceStart(*this, anchor));
}

void ConfigurationBuilder::sequenceEnd() {
    auto removed = std::move(m_state.top());
    m_state.pop();
    m_state.top()->subStateEnd(*this, *removed);
}

void ConfigurationBuilder::mappingStart(const std::string &, const std::string & anchor) {
    m_state.push(m_state.top()->mappingStart(*this, anchor));
}

void ConfigurationBuilder::mappingEnd() {
    auto removed = std::move(m_state.top());
    m_state.pop();
    m_state.top()->subStateEnd(*this, *removed);
}

void ConfigurationBuilder::alias(const std::string & anchor) {
    m_state.top()->alias(*this, anchor);
}

void ConfigurationBuilder::scalar(const std::string & value, const std::string &, const std::string & anchor) {
    m_state.top()->scalar(*this, value, anchor);
}

ConfigurationBuilder::ParseError ConfigurationBuilder::makeError(const std::string & what)
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
                  [&msg](const auto & statep){ msg <<'/' <<*statep; });
    return YAMLParser::makeError(msg.str());
}

/****************************************************************************/

Configuration::Configuration(std::string path,
                             path_list pluginPaths,
                             device_map devices,
                             key_group_list keyGroups,
                             effect_group_list effectGroups,
                             profile_list profiles)
 : m_path(std::move(path)),
   m_pluginPaths(std::move(pluginPaths)),
   m_devices(std::move(devices)),
   m_keyGroups(std::move(keyGroups)),
   m_effectGroups(std::move(effectGroups)),
   m_profiles(std::move(profiles))
{}

Configuration Configuration::loadFile(const std::string & path)
{
    using tools::paths::XDG;

    if (path.empty()) {
        throw std::runtime_error("Empty configuration file path");
    }

    std::ifstream file;
    std::string actualPath;
    tools::paths::open(file, XDG::Config, path, std::ios::binary, &actualPath);
    if (!file) {
        throw std::system_error(errno, std::generic_category());
    }

    auto builder = ConfigurationBuilder();
    builder.parse(file);
    return Configuration(
        actualPath,
        std::move(builder.m_pluginPaths),
        std::move(builder.m_devices),
        std::move(builder.m_keyGroups),
        std::move(builder.m_effectGroups),
        std::move(builder.m_profiles)
    );
}

/****************************************************************************/

Configuration::EffectGroup::EffectGroup(std::string name,
                                        key_group_list keyGroups,
                                        effect_list effects)
 : m_name(std::move(name)),
   m_keyGroups(std::move(keyGroups)),
   m_effects(std::move(effects))
{}

/****************************************************************************/

Configuration::KeyGroup::KeyGroup(std::string name, key_list keys)
 : m_name(std::move(name)),
   m_keys(std::move(keys))
{}

/****************************************************************************/

Configuration::Profile::Profile(std::string name,
                                Lookup lookup,
                                device_list devices,
                                effect_group_list effectGroups)
 : m_name(std::move(name)),
   m_lookup(std::move(lookup)),
   m_devices(std::move(devices)),
   m_effectGroups(std::move(effectGroups))
{}

/****************************************************************************/

Configuration::Profile::Lookup::Lookup(string_map filters)
 : m_entries(buildRegexps(std::move(filters)))
{}

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

/****************************************************************************/

Configuration::Effect::Effect(std::string name, conf_map items)
 : m_name(std::move(name)), m_items(std::move(items))
{}

const std::string & Configuration::Effect::operator[](const std::string & key) const
{
    static const std::string empty;
    auto it = std::find_if(m_items.begin(), m_items.end(),
                           [key](const auto & item) { return item.first == key; });
    return it != m_items.end() ? it->second : empty;
}
