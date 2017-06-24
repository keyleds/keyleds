#ifndef KEYLEDSD_CONFIGURATION_H
#define KEYLEDSD_CONFIGURATION_H

#include <map>
#include <regex>
#include <string>
#include <vector>

namespace keyleds {

class Context;

class Configuration final
{
public:
    class Plugin;
    class Profile;
    typedef std::vector<std::string> key_list;

    typedef std::vector<std::string> path_list;
    typedef std::map<std::string, std::string> device_map;
    typedef std::map<std::string, key_list> group_map;
    typedef std::map<std::string, Profile> profile_map;
private:
                        Configuration(bool autoQuit,
                                      bool noDBus,
                                      path_list && pluginPaths,
                                      path_list && layoutPaths,
                                      device_map && devices,
                                      group_map && groups,
                                      profile_map && profiles)
                            : m_autoQuit(autoQuit),
                              m_noDBus(noDBus),
                              m_pluginPaths(pluginPaths),
                              m_layoutPaths(layoutPaths),
                              m_devices(devices),
                              m_groups(groups),
                              m_profiles(profiles) {}

public:
                        Configuration() = default;

          bool          autoQuit() const { return m_autoQuit; }
          void          setAutoQuit(bool v) { m_autoQuit = v; }
          bool          noDBus() const { return m_noDBus; }

          path_list &   pluginPaths() { return m_pluginPaths; }
    const path_list &   pluginPaths() const { return m_pluginPaths; }
          path_list &   layoutPaths() { return m_layoutPaths; }
    const path_list &   layoutPaths() const { return m_layoutPaths; }
          device_map &  devices() { return m_devices; }
    const device_map &  devices() const { return m_devices; }
          group_map &   groups() { return m_groups; }
    const group_map &   groups() const { return m_groups; }
          profile_map & profiles() { return m_profiles; }
    const profile_map & profiles() const { return m_profiles; }

public:
    static Configuration loadFile(const std::string & path);
    static Configuration loadArguments(int & argc, char * argv[]);

private:
    bool                m_autoQuit;
    bool                m_noDBus;

    path_list           m_pluginPaths;
    path_list           m_layoutPaths;
    device_map          m_devices;
    group_map           m_groups;
    profile_map         m_profiles;
};

/****************************************************************************/

class Configuration::Profile final
{
public:
    class Lookup final
    {
    public:
        Lookup() {}
        Lookup(std::string title, std::string className, std::string instanceName)
         : m_titleFilter(title),
           m_classNameFilter(className),
           m_instanceNameFilter(instanceName),
           m_didCompileRE(false) {}
        const std::string & titleFilter() const { return m_titleFilter; }
        const std::string & classNameFilter() const { return m_classNameFilter; }
        const std::string & instanceNameFilter() const { return m_instanceNameFilter; }

        bool                match(const Context &) const;

    private:
        std::string         m_titleFilter;
        std::string         m_classNameFilter;
        std::string         m_instanceNameFilter;
        mutable bool        m_didCompileRE;
        mutable std::regex  m_titleRE;
        mutable std::regex  m_classNameRE;
        mutable std::regex  m_instanceNameRE;
    };

    typedef unsigned int id_type;
    typedef std::vector<std::string> device_list;
    typedef std::map<std::string, key_list> group_map;
    typedef std::vector<Configuration::Plugin> plugin_list;
public:
                        Profile(std::string name,
                                bool isDefault,
                                Lookup && lookup,
                                device_list && devices,
                                group_map && groups,
                                plugin_list && plugins)
                         : m_id(makeId()),
                           m_name(name),
                           m_isDefault(isDefault),
                           m_lookup(lookup),
                           m_devices(devices),
                           m_groups(groups),
                           m_plugins(plugins) {}
          id_type       id() const noexcept { return m_id; }
    const std::string & name() const { return m_name; }
          bool          isDefault() const { return m_isDefault; }
    const Lookup &      lookup() const { return m_lookup; }
    const device_list & devices() const { return m_devices; }
    const group_map &   groups() const { return m_groups; }
    const plugin_list & plugins() const { return m_plugins; }

private:
    static id_type      makeId();

private:
    id_type             m_id;
    std::string         m_name;
    bool                m_isDefault;
    Lookup              m_lookup;
    device_list         m_devices;
    group_map           m_groups;
    plugin_list         m_plugins;
};

/****************************************************************************/

class Configuration::Plugin final
{
public:
    typedef std::map<std::string, std::string> conf_map;
public:
                        Plugin(std::string name, conf_map && items)
                            : m_name(name), m_items(items) {}
    const std::string & name() const { return m_name; }
    const conf_map &    items() const { return m_items; }
private:
    std::string         m_name;
    conf_map            m_items;
};

};

#endif
