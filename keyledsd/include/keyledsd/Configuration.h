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
#ifndef KEYLEDSD_CONFIGURATION_H_603C2B68
#define KEYLEDSD_CONFIGURATION_H_603C2B68

#include <map>
#include <regex>
#include <string>
#include <vector>
#include "logging.h"

namespace keyleds {

class Context;

/** Complete service configuration
 *
 * Holds all configuration data to run an instance of the service,
 * along with helper methods to load it from a file or program
 * arguments.
 */
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
                        Configuration(Logger::level_t logLevel,
                                      bool autoQuit,
                                      bool noDBus,
                                      path_list pluginPaths,
                                      path_list layoutPaths,
                                      device_map devices,
                                      group_map groups,
                                      profile_map profiles);

public:
                        Configuration() = default;

          Logger::level_t logLevel() const { return m_logLevel; }
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
    Logger::level_t     m_logLevel;     ///< Maximum level of messsages to log
    bool                m_autoQuit;     ///< If set, the service will exit when last device is removed.
    bool                m_noDBus;       ///< If set, DBus adapters are not loaded on startup

    path_list           m_pluginPaths;  ///< List of directories to search for plugins
    path_list           m_layoutPaths;  ///< List of directories to search for layout files
    device_map          m_devices;      ///< Map of device serials to device names
    group_map           m_groups;       ///< Map of key group names to lists of key names
    profile_map         m_profiles;     ///< Map of profile names to profile configurations
};

/****************************************************************************/

/** Profile configuration
 *
 * Holds the configuration of a single keyboard profile. Each profile
 * has a unique identifier used to be able to differentiate two otherwise
 * similar profiles.
 */
class Configuration::Profile final
{
public:
    /// Filters a context to determine whether a profile should be enabled
    class Lookup final
    {
    public:
        Lookup() = default;
        Lookup(std::string title, std::string className, std::string instanceName);
        const std::string & titleFilter() const { return m_titleFilter; }
        const std::string & classNameFilter() const { return m_classNameFilter; }
        const std::string & instanceNameFilter() const { return m_instanceNameFilter; }

        bool                match(const Context &) const;

    private:
        std::string         m_titleFilter;          ///< Regexp matched against context's "title" item
        std::string         m_classNameFilter;      ///< Regexp matched against context's "class" item
        std::string         m_instanceNameFilter;   ///< Regexp matched against context's "instance" item
        mutable bool        m_didCompileRE;         ///< Set when regexps have been compiled into objects below
        mutable std::regex  m_titleRE;              ///< Compiled version of m_titleFilter
        mutable std::regex  m_classNameRE;          ///< Compiled version of m_classNameFilter
        mutable std::regex  m_instanceNameRE;       ///< Compiled version of m_instanceNameFilter
    };

    typedef unsigned int id_type;
    typedef std::vector<std::string> device_list;
    typedef Configuration::group_map group_map;
    typedef std::vector<Configuration::Plugin> plugin_list;
public:
                        Profile(std::string name,
                                bool isDefault,
                                Lookup lookup,
                                device_list devices,
                                group_map groups,
                                plugin_list plugins);
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
    id_type             m_id;           ///< Unique throughout a service lifetime
    std::string         m_name;         ///< User-readable name
    bool                m_isDefault;    ///< If set, m_lookup is ignored and this profile applies
                                        ///  if and only if no non-default profile applies.
    Lookup              m_lookup;       ///< Matched against a context to determine whether to apply the profile
    device_list         m_devices;      ///< List of device names this profile is restricted to
    group_map           m_groups;       ///< Map of profile-specific key group names to lists of key names
    plugin_list         m_plugins;      ///< List of plugin configurations
};

/****************************************************************************/

/** Plugin configuration
 *
 * Holds the configuration of a single rendering plugin. It is a simple string
 * map, and a plugin name used to look it up in the plugin manager.
 */
class Configuration::Plugin final
{
public:
    typedef std::map<std::string, std::string> conf_map;
public:
                        Plugin(std::string name, conf_map items);
    const std::string & name() const { return m_name; }
    const conf_map &    items() const { return m_items; }
private:
    std::string         m_name;         ///< Plugin name as registered in plugin manager
    conf_map            m_items;        ///< Flat string map passed through to plugin
};

};

#endif
