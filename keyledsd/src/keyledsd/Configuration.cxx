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
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <system_error>
#include <unistd.h>
#include "keyledsd/Configuration.h"
#include "keyledsd/Context.h"
#include "tools/YAMLParser.h"
#include "config.h"

using keyleds::Configuration;
static const std::vector<std::string> truthy = {"yes", "1", "true"};
static const std::vector<std::string> falsy = {"no", "0", "false"};

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
class ConfigurationBuilder final : public YAMLParser
{
public:
    class BuildState;
    typedef std::unique_ptr<BuildState> state_ptr;
public:
    ConfigurationBuilder();

    void addScalarAlias(std::string anchor, std::string value);
    const std::string & getScalarAlias(const std::string & anchor);
    void addGroupAlias(std::string anchor, const Configuration::key_list &);
    const Configuration::key_list & getGroupAlias(std::string anchor);

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

    template<typename T> T parseScalar(const std::string &);
public:
    bool                        m_autoQuit;
    bool                        m_noDBus;
    Configuration::path_list    m_pluginPaths;
    Configuration::path_list    m_layoutPaths;
    Configuration::device_map   m_devices;
    Configuration::group_map    m_groups;
    Configuration::profile_map  m_profiles;

private:
    std::stack<state_ptr>               m_state;
    std::map<std::string, std::string>  m_scalarAliases;
    std::map<std::string, Configuration::key_list> m_groupAliases;
};

template<> bool ConfigurationBuilder::parseScalar<bool>(const std::string & val)
{
    if (std::find(truthy.begin(), truthy.end(), val) != truthy.end()) { return true; }
    if (std::find(falsy.begin(), falsy.end(), val) != falsy.end()) { return false; }
    throw makeError("Invalid boolean value <" + val + ">");
}

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
    typedef std::unique_ptr<BuildState> state_ptr;
    typedef unsigned int state_type;

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
    typedef std::vector<std::string>    value_type;
public:
    StringSequenceBuildState(state_type type = 0) : BuildState(type) {}
    void print(std::ostream & out) const override { out <<"string-sequence"; }

    void alias(ConfigurationBuilder & builder, const std::string & anchor) override
    {
        m_value.push_back(builder.getScalarAlias(anchor));
    }

    void scalar(ConfigurationBuilder & builder, const std::string & value,
                const std::string & anchor) override
    {
        m_value.push_back(value);
        builder.addScalarAlias(anchor, value);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};

class StringMappingBuildState final : public MappingBuildState
{
public:
    typedef std::map<std::string, std::string> value_type;
public:
    StringMappingBuildState(state_type type = 0) : MappingBuildState(type) {}
    void print(std::ostream & out) const override { out <<"string-mapping"; }

    void aliasEntry(ConfigurationBuilder & builder, const std::string & key,
                    const std::string & anchor) override
    {
        m_value[key] = builder.getScalarAlias(anchor);
    }

    void scalarEntry(ConfigurationBuilder & builder, const std::string & key,
                     const std::string & value, const std::string & anchor) override
    {
        m_value[key] = value;
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
class GroupListState final : public MappingBuildState
{
public:
    typedef std::map<std::string, Configuration::key_list> value_type;
public:
    GroupListState(state_type type) : MappingBuildState(type) {}
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

        auto it = m_value.emplace(currentKey(), std::move(keys)).first;
        if (!m_currentAnchor.empty()) {
            builder.addGroupAlias(m_currentAnchor, it->second);
        }
        MappingBuildState::subStateEnd(builder, state);
    }

    void aliasEntry(ConfigurationBuilder & builder, const std::string & key,
                    const std::string & anchor) override
    {
        m_value.emplace(std::make_pair(key, builder.getGroupAlias(anchor)));
    }

    value_type && result() { return std::move(m_value); }

private:
    std::string     m_currentAnchor;
    value_type      m_value;
};


/// Configuration builder state: within a profile's plugin list
class PluginListState final : public ConfigurationBuilder::BuildState
{
public:
    typedef Configuration::Profile::plugin_list value_type;
public:
    PluginListState(state_type type) : BuildState(type) {}
    void print(std::ostream & out) const override { out <<"plugins-list"; }

    state_ptr mappingStart(ConfigurationBuilder &, const std::string &) override
    {
        return std::make_unique<StringMappingBuildState>();
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        auto conf_map = state.as<StringMappingBuildState>().result();
        auto it_name = conf_map.find("plugin");
        if (it_name == conf_map.end()) { throw builder.makeError("plugin configuration must have a name"); }

        auto name = it_name->second;
        conf_map.erase(it_name);

        m_value.emplace_back(std::move(name), std::move(conf_map));
    }

    void scalar(ConfigurationBuilder &, const std::string & value, const std::string &) override
    {
        m_value.emplace_back(Configuration::Plugin(value, Configuration::Plugin::conf_map()));
    }

    value_type && result() { return std::move(m_value); }

private:
    value_type      m_value;
};


/// Configuration builder state: within a profile
class ProfileState final: public MappingBuildState
{
    enum SubState : state_type { Lookup, DeviceList, GroupList, PluginList };
public:
    ProfileState(std::string name, state_type type = 0)
      : MappingBuildState(type), m_name(name), m_isDefault(false) {}
    void print(std::ostream & out) const override { out <<"profile(" <<m_name <<')'; }

    state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string & key, const std::string &) override
    {
        if (key == "plugins") { return std::make_unique<PluginListState>(SubState::PluginList); }
        if (key == "devices") { return std::make_unique<StringSequenceBuildState>(SubState::DeviceList); }
        throw builder.makeError("unknown section");
    }

    state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string & key, const std::string &) override
    {
        if (key == "lookup") {
            if (m_name == "default") {
                throw builder.makeError("default profile cannot have filters defined");
            }
            return std::make_unique<StringMappingBuildState>(SubState::Lookup);
        }
        if (key == "groups") { return std::make_unique<GroupListState>(SubState::GroupList); }
        throw builder.makeError("unknown section");
    }

    void scalarEntry(ConfigurationBuilder & builder, const std::string & key,
                     const std::string & value, const std::string &)
    {
        if (key == "default")   { m_isDefault = builder.parseScalar<bool>(value); }
        else throw builder.makeError("unknown setting");
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        switch(state.type()) {
            case SubState::Lookup: {
                auto items = state.as<StringMappingBuildState>().result();
                m_lookup = Configuration::Profile::Lookup(
                    items["title"],
                    items["class"],
                    items["instance"]
                );
                break;
            }
            case SubState::DeviceList: {
                m_devices = state.as<StringSequenceBuildState>().result();
                break;
            }
            case SubState::GroupList: {
                m_groups = state.as<GroupListState>().result();
                break;
            }
            case SubState::PluginList: {
                m_plugins = state.as<PluginListState>().result();
                break;
            }
        }
        MappingBuildState::subStateEnd(builder, state);
    }

    Configuration::Profile result()
    {
        return Configuration::Profile(
            std::move(m_name),
            m_isDefault,
            std::move(m_lookup),
            std::move(m_devices),
            std::move(m_groups),
            std::move(m_plugins)
        );
    }

private:
    std::string                         m_name;
    bool                                m_isDefault;
    Configuration::Profile::Lookup      m_lookup;
    Configuration::Profile::device_list m_devices;
    Configuration::Profile::group_map   m_groups;
    Configuration::Profile::plugin_list m_plugins;
};


/// Configuration builder state: within a profile list
class ProfileListState final : public MappingBuildState
{
public:
    typedef std::map<std::string, Configuration::Profile> value_type;
public:
    ProfileListState(state_type type) : MappingBuildState(type) {}
    void print(std::ostream & out) const override { out <<"profile-map"; }

    state_ptr mappingEntry(ConfigurationBuilder &, const std::string & key, const std::string &) override
    {
        return std::make_unique<ProfileState>(key);
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        m_value.emplace(std::make_pair(currentKey(), state.as<ProfileState>().result()));
        MappingBuildState::subStateEnd(builder, state);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};


/// Configuration builder state: at document root
class RootState final : public MappingBuildState
{
    enum SubState : state_type { Plugins, Layouts, Devices, Groups, Profiles };
public:
    RootState() : MappingBuildState(0) {}
    void print(std::ostream & out) const override { out <<"root"; }

    state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string &) override
    {
        if (key == "layouts") { return std::make_unique<StringSequenceBuildState>(SubState::Layouts); }
        if (key == "plugins") { return std::make_unique<StringSequenceBuildState>(SubState::Plugins); }
        throw builder.makeError("unknown section");
    }

    state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string &) override
    {
        if (key == "devices")   { return std::make_unique<StringMappingBuildState>(SubState::Devices); }
        if (key == "groups")    { return std::make_unique<GroupListState>(SubState::Groups); }
        if (key == "profiles")  { return std::make_unique<ProfileListState>(SubState::Profiles); }
        throw builder.makeError("unknown section");
    }

    void scalarEntry(ConfigurationBuilder & builder, const std::string & key,
                     const std::string & value, const std::string &)
    {
        if (key == "auto_quit")     { builder.m_autoQuit = builder.parseScalar<bool>(value); }
        else if (key == "dbus")     { builder.m_noDBus = !builder.parseScalar<bool>(value); }
        else if (key == "plugins")  { builder.m_pluginPaths = { value }; }
        else if (key == "layouts")  { builder.m_layoutPaths = { value }; }
        else throw builder.makeError("unknown setting");
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        switch (state.type()) {
        case SubState::Plugins:
            builder.m_pluginPaths = state.as<StringSequenceBuildState>().result();
            break;
        case SubState::Layouts:
            builder.m_layoutPaths = state.as<StringSequenceBuildState>().result();
            break;
        case SubState::Devices: {
            for (const auto & item : state.as<StringMappingBuildState>().result()) {
                builder.m_devices.emplace(item.second, item.first);
            }
            break;
        }
        case SubState::Groups:
            builder.m_groups = state.as<GroupListState>().result();
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
 : m_autoQuit(false), m_noDBus(false)
{
    m_state.push(std::make_unique<InitialState>());
}

void ConfigurationBuilder::addScalarAlias(std::string anchor, std::string value) {
    m_scalarAliases[anchor] = value;
}

const std::string & ConfigurationBuilder::getScalarAlias(const std::string & anchor) {
    auto it = m_scalarAliases.find(anchor);
    if (it == m_scalarAliases.end()) {
        throw makeError("unknown anchor or invalid anchor target");
    }
    return it->second;
}

void ConfigurationBuilder::addGroupAlias(std::string anchor, const Configuration::key_list & value) {
    m_groupAliases.emplace(std::make_pair(anchor, value));
}

const Configuration::key_list & ConfigurationBuilder::getGroupAlias(std::string anchor) {
    auto it = m_groupAliases.find(anchor);
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
        state.push_back(std::move(m_state.top()));
        m_state.pop();
    }
    std::ostringstream msg;
    msg <<what <<" in ";
    std::for_each(state.rbegin() + 1, state.rend(),
                  [&msg](const auto & statep){ msg <<'/' <<*statep; });
    return YAMLParser::makeError(msg.str());
}

/****************************************************************************/

Configuration Configuration::loadFile(const std::string & path)
{
    auto builder = ConfigurationBuilder();
    auto file = std::ifstream(path, std::ios::binary);
    if (!file) {
        throw std::system_error(errno, std::generic_category());
    }
    builder.parse(file);
    return Configuration(
        Logger::warning::value(),
        builder.m_autoQuit,
        builder.m_noDBus,
        std::move(builder.m_pluginPaths),
        std::move(builder.m_layoutPaths),
        std::move(builder.m_devices),
        std::move(builder.m_groups),
        std::move(builder.m_profiles)
    );
}

#ifdef _GNU_SOURCE
#include <getopt.h>
static const struct option options[] = {
    {"config",    1, nullptr, 'c' },
    {"help",      0, nullptr, 'h' },
    {"quiet",     0, nullptr, 'q' },
    {"single",    0, nullptr, 's' },
    {"verbose",   0, nullptr, 'v' },
    {"no-dbus",   0, nullptr, 'D' },
    {nullptr, 0, nullptr, 0}
};
#endif

Configuration Configuration::loadArguments(int & argc, char * argv[])
{
    int opt;
    std::ostringstream msgBuf;

    const char * configPath = nullptr;
    Logger::level_t logLevel = Logger::warning::value();
    bool autoQuit = false;
    bool noDBus = false;

    ::opterr = 0;
#ifdef _GNU_SOURCE
    while ((opt = ::getopt_long(argc, argv, ":c:hqsvD", options, nullptr)) >= 0) {
#else
    while ((opt = ::getopt(argc, argv, ":c:hqsvD")) >= 0) {
#endif
        switch(opt) {
        case 'c':
            if (configPath != nullptr) {
                throw std::runtime_error("-c option can only be specified once");
            }
            configPath = optarg;
            break;
        case 'h':
            std::cout <<"Usage: " <<argv[0] <<" [-c path] [-s] [-D]" <<std::endl;
            ::exit(EXIT_SUCCESS);
        case 'q':
            logLevel = Logger::critical::value();
            break;
        case 's':
            autoQuit = true;
            break;
        case 'v':
            logLevel += 1;
            break;
        case 'D':
            noDBus = true;
            break;
        case ':':
            msgBuf <<argv[0] <<": option -- '" <<(char)::optopt <<"' requires an argument";
            throw std::runtime_error(msgBuf.str());
        default:
            msgBuf <<argv[0] <<": invalid option -- '" <<(char)::optopt <<"'";
            throw std::runtime_error(msgBuf.str());
        }
    }

    auto config = loadFile(configPath != nullptr ? configPath : KEYLEDSD_CONFIG_PATH);
    config.m_logLevel = logLevel;
    if (autoQuit) { config.m_autoQuit = true; }
    if (noDBus) { config.m_noDBus = true; }
    return config;
}

/****************************************************************************/

Configuration::Profile::id_type Configuration::Profile::makeId()
{
    static std::atomic<id_type> counter(1);
    return ++counter;
}

/****************************************************************************/

bool Configuration::Profile::Lookup::match(const Context & context) const
{
    if (!m_didCompileRE) {
        m_titleRE.assign(m_titleFilter, std::regex::nosubs | std::regex::optimize);
        m_classNameRE.assign(m_classNameFilter, std::regex::nosubs | std::regex::optimize);
        m_instanceNameRE.assign(m_instanceNameFilter, std::regex::nosubs | std::regex::optimize);
        m_didCompileRE = true;
    }
    return (m_titleFilter.empty() || std::regex_match(context["title"], m_titleRE)) &&
           (m_classNameFilter.empty() || std::regex_match(context["class"], m_classNameRE)) &&
           (m_instanceNameFilter.empty() || std::regex_match(context["instance"], m_instanceNameRE));
}
