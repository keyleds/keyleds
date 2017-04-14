#ifndef KEYLEDSD_CONFIGURATION_H
#define KEYLEDSD_CONFIGURATION_H

#include <map>
#include <string>
#include <vector>

namespace keyleds {

class Configuration final
{
    static const std::string c_defaultStackName;
public:
    class Plugin;
    class Stack;

    typedef std::vector<std::string> path_list;
    typedef std::map<std::string, Stack> stack_map;
    typedef std::map<std::string, std::string> device_map;
public:
                        Configuration() {}
                        Configuration(path_list && pluginPaths,
                                      stack_map && stacks,
                                      device_map && deviceStacks)
                            : m_pluginPaths(pluginPaths),
                              m_stacks(stacks),
                              m_deviceStacks(deviceStacks) {}

    const path_list &   pluginPaths() const { return m_pluginPaths; }
          stack_map &   stacks() { return m_stacks; }
    const stack_map &   stacks() const { return m_stacks; }

    void                addDeviceStack(std::string serial, std::string name) { m_deviceStacks[serial] = name; }
    void                removeDeviceStack(const std::string & serial) { m_deviceStacks.erase(serial); }
    const Stack &       stackFor(const std::string & serial) const;

    static Configuration loadFile(const std::string & path);

private:
    static const Stack & defaultStack();

private:
    path_list           m_pluginPaths;
    stack_map           m_stacks;
    device_map          m_deviceStacks;
};

/****************************************************************************/

class Configuration::Plugin final
{
public:
    typedef std::map<std::string, std::string> conf_map;
public:
                        Plugin(std::string name, conf_map items)
                            : m_name(name), m_items(items) {}
    const std::string & name() const { return m_name; }
    const conf_map &    items() const { return m_items; }
private:
    std::string         m_name;
    conf_map            m_items;
};


class Configuration::Stack final
{
public:
    typedef std::vector<Configuration::Plugin> plugin_list;
public:
                        Stack(std::string name) : m_name(name) {}
                        Stack(std::string name, plugin_list plugins)
                            : m_name(name), m_plugins(plugins) {}
    const std::string & name() const { return m_name; }
          plugin_list & plugins() { return m_plugins; }
    const plugin_list & plugins() const { return m_plugins; }
private:
    std::string         m_name;
    plugin_list         m_plugins;
};

};

#endif
