#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <system_error>
#include <unistd.h>
#include "keyledsd/Configuration.h"
#include "tools/YAMLParser.h"
#include "config.h"

using keyleds::Configuration;

/****************************************************************************/

class BuildState;
typedef std::unique_ptr<BuildState> state_ptr;

class ConfigurationBuilder : public YAMLParser
{
    std::stack<state_ptr>               m_state;
    std::map<std::string, std::string>  m_scalarAliases;
public:
    ConfigurationBuilder();

    void addScalarAlias(std::string anchor, std::string value);
    const std::string & getScalarAlias(const std::string & anchor);

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

    ParseError makeError(const std::string & what) const { return YAMLParser::makeError(what); }

public:
    Configuration::path_list    m_pluginPaths;
    Configuration::path_list    m_layoutPaths;
    Configuration::stack_map    m_stacks;
    Configuration::device_map   m_devices;
};



struct BuildState
{
    virtual state_ptr sequenceStart(ConfigurationBuilder & builder, const std::string &)
        { throw builder.makeError("unexpected sequence"); }
    virtual state_ptr mappingStart(ConfigurationBuilder & builder, const std::string &)
        { throw builder.makeError("unexpected mapping"); }
    virtual void subStateEnd(ConfigurationBuilder &, BuildState &) {}
    virtual void alias(ConfigurationBuilder & builder, const std::string &)
        { throw builder.makeError("unexpected alias"); }
    virtual void scalar(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected scalar"); }

    template<class T> T & as() { return static_cast<T&>(*this); }
};

class MappingBuildState : public BuildState
{
public:
    virtual state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected sequence"); }
    virtual state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected mapping"); }
    virtual void aliasEntry(ConfigurationBuilder & builder, const std::string &, const std::string &)
        { throw builder.makeError("unexpected alias"); }
    virtual void scalarEntry(ConfigurationBuilder & builder, const std::string &,
                             const std::string &, const std::string &)
        { throw builder.makeError("unexpected scalar"); }

    state_ptr sequenceStart(ConfigurationBuilder & builder, const std::string & anchor) override
    {
        if (m_currentKey.empty()) { throw builder.makeError("unexpected sequence"); }
        return sequenceEntry(builder, m_currentKey, anchor);
    }

    state_ptr mappingStart(ConfigurationBuilder & builder, const std::string & anchor) override
    {
        if (m_currentKey.empty()) { throw builder.makeError("unexpected mapping"); }
        return mappingEntry(builder, m_currentKey, anchor);
    }

    void subStateEnd(ConfigurationBuilder &, BuildState &) override { m_currentKey.clear(); }

    void alias(ConfigurationBuilder & builder, const std::string & anchor) override
    {
        if (m_currentKey.empty()) {
            m_currentKey = builder.getScalarAlias(anchor);
        } else {
            aliasEntry(builder, m_currentKey, anchor);
            m_currentKey.clear();
        }
    }

    void scalar(ConfigurationBuilder & builder, const std::string & value,
                const std::string & anchor) override
    {
        if (m_currentKey.empty()) {
            m_currentKey = value;
        } else {
            scalarEntry(builder, m_currentKey, value, anchor);
            m_currentKey.clear();
        }
    }

protected:
    std::string     m_currentKey;
};

/****************************************************************************/

class StringSequenceBuildState : public BuildState
{
public:
    typedef std::vector<std::string>    value_type;
public:
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

class StringMappingBuildState : public MappingBuildState
{
public:
    typedef std::map<std::string, std::string> value_type;
public:
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

class PluginState final : public StringMappingBuildState
{
public:
    Configuration::Plugin result(ConfigurationBuilder & builder)
    {
        value_type value = StringMappingBuildState::result();
        auto it_name = value.find("plugin");
        if (it_name == value.end()) { throw builder.makeError("plugin configuration must have a name"); }
        std::string name = it_name->second;
        value.erase(it_name);
        return Configuration::Plugin(std::move(name), std::move(value));
    }
};

class StackState final : public BuildState
{
public:
    StackState(std::string name) : m_name(name) {}

    state_ptr mappingStart(ConfigurationBuilder &, const std::string &) override
    {
        return std::make_unique<PluginState>();
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        m_plugins.emplace_back(state.as<PluginState>().result(builder));
    }

    void scalar(ConfigurationBuilder &, const std::string & value, const std::string &) override
    {
        m_plugins.emplace_back(Configuration::Plugin(value, Configuration::Plugin::conf_map()));
    }

    Configuration::Stack result()
    {
        return Configuration::Stack(std::move(m_name), std::move(m_plugins));
    }

private:
    std::string                       m_name;
    Configuration::Stack::plugin_list m_plugins;
};

class StackListState final : public MappingBuildState
{
public:
    typedef std::map<std::string, Configuration::Stack> value_type;
public:
    state_ptr sequenceEntry(ConfigurationBuilder &, const std::string & key, const std::string &) override
    {
        return std::make_unique<StackState>(key);
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        m_value.emplace(std::make_pair(m_currentKey, state.as<StackState>().result()));
        MappingBuildState::subStateEnd(builder, state);
    }

    value_type &&   result() { return std::move(m_value); }

private:
    value_type      m_value;
};


class RootState final : public MappingBuildState
{
public:
    state_ptr sequenceEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string &) override
    {
        if (key == "plugin_paths" || key == "layout_paths")  {
            return std::make_unique<StringSequenceBuildState>();
        }
        throw builder.makeError("unknown section");
    }

    state_ptr mappingEntry(ConfigurationBuilder & builder, const std::string & key,
                           const std::string &) override
    {
        if (key == "stacks")        { return std::make_unique<StackListState>(); }
        else if (key == "devices")  { return std::make_unique<StringMappingBuildState>(); }
        throw builder.makeError("unknown section");
    }

    void subStateEnd(ConfigurationBuilder & builder, BuildState & state) override
    {
        if (m_currentKey == "plugin_paths") {
            builder.m_pluginPaths = state.as<StringSequenceBuildState>().result();
        } else if (m_currentKey == "layout_paths") {
            builder.m_layoutPaths = state.as<StringSequenceBuildState>().result();
        } else if (m_currentKey == "stacks") {
            builder.m_stacks = state.as<StackListState>().result();
        } else if (m_currentKey == "devices") {
            builder.m_devices = state.as<StringMappingBuildState>().result();
        }
        MappingBuildState::subStateEnd(builder, state);
    }
};


class InitialState final : public BuildState
{
public:
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
    m_scalarAliases[anchor] = value;
}

const std::string & ConfigurationBuilder::getScalarAlias(const std::string & anchor) {
    auto it = m_scalarAliases.find(anchor);
    if (it == m_scalarAliases.end()) {
        throw makeError("unknown anchor or invalid anchor target");
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

/****************************************************************************/

const std::string Configuration::c_defaultStackName("default");

const Configuration::Stack & Configuration::stackFor(const std::string & serial) const
{
    auto it = m_deviceStacks.find(serial);
    const std::string & name = (it != m_deviceStacks.end() ? it->second : c_defaultStackName);

    auto stackIt = m_stacks.find(name);
    return stackIt != m_stacks.end() ? stackIt->second : defaultStack();
}

Configuration Configuration::loadFile(const std::string & path)
{
    auto builder = ConfigurationBuilder();
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::system_error(errno, std::generic_category());
    }
    builder.parse(file);
    return Configuration(
        std::move(builder.m_pluginPaths),
        std::move(builder.m_layoutPaths),
        std::move(builder.m_stacks),
        std::move(builder.m_devices)
    );
}

Configuration Configuration::loadArguments(int & argc, char * argv[])
{
    int opt;
    std::ostringstream msgBuf;

    const char * configPath = nullptr;
    bool autoQuit = false;

    ::opterr = 0;
    while ((opt = ::getopt(argc, argv, ":c:hs")) >= 0) {
        switch(opt) {
        case 'c':
            if (configPath != nullptr) {
                throw std::runtime_error("-c option can only be specified once");
            }
            configPath = optarg;
            break;
        case 'h':
            std::cout <<"Usage: " <<argv[0] <<" [-c path] [-s]" <<std::endl;
            ::exit(EXIT_SUCCESS);
        case 's':
            autoQuit = true;
            break;
        case ':':
            msgBuf <<argv[0] <<": option -- '" <<(char)::optopt <<"' requires an argument";
            throw std::runtime_error(msgBuf.str());
        default:
            msgBuf <<argv[0] <<": invalid option -- '" <<(char)::optopt <<"'";
            throw std::runtime_error(msgBuf.str());
        }
    }

    Configuration config = loadFile(configPath != nullptr ? configPath : KEYLEDSD_CONFIG_PATH);
    config.m_autoQuit = autoQuit;
    return config;
}

const Configuration::Stack & Configuration::defaultStack()
{
    static Configuration::Stack defaultStack(c_defaultStackName);
    return defaultStack;
}
