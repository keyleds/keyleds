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

#include <regex>
#include <string>
#include <utility>
#include <vector>

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
    class Effect;
    class Plugin;
    class Profile;

    using key_list = std::vector<std::string>;

    using path_list = std::vector<std::string>;
    using device_map = std::vector<std::pair<std::string, std::string>>;
    using group_map = std::vector<std::pair<std::string, key_list>>;
    using effect_list = std::vector<Effect>;
    using profile_list = std::vector<Profile>;
private:
                        Configuration(unsigned logLevel,
                                      bool autoQuit,
                                      bool noDBus,
                                      path_list pluginPaths,
                                      path_list layoutPaths,
                                      device_map devices,
                                      group_map groups,
                                      effect_list effects,
                                      profile_list profiles);

public:
                        Configuration() = default;

          unsigned      logLevel() const { return m_logLevel; }
          bool          autoQuit() const { return m_autoQuit; }
          void          setAutoQuit(bool v) { m_autoQuit = v; }
          bool          noDBus() const { return m_noDBus; }

    const path_list &   pluginPaths() const { return m_pluginPaths; }
    const path_list &   layoutPaths() const { return m_layoutPaths; }
    const device_map &  devices() const { return m_devices; }
    const group_map &   groups() const { return m_groups; }
    const effect_list & effects() const { return m_effects; }
    const profile_list& profiles() const { return m_profiles; }

public:
    static Configuration loadFile(const std::string & path);
    static Configuration loadArguments(int & argc, char * argv[]);

private:
    unsigned            m_logLevel;     ///< Maximum level of messsages to log
    bool                m_autoQuit;     ///< If set, the service will exit when last device is removed.
    bool                m_noDBus;       ///< If set, DBus adapters are not loaded on startup

    path_list           m_pluginPaths;  ///< List of directories to search for plugins
    path_list           m_layoutPaths;  ///< List of directories to search for layout files
    device_map          m_devices;      ///< Map of device serials to device names
    group_map           m_groups;       ///< Map of key group names to lists of key names
    effect_list         m_effects;      ///< Map of effect names to effect configurations
    profile_list        m_profiles;     ///< List of profile configurations
};

/****************************************************************************/

/** Effect configuration
 */
class Configuration::Effect final
{
public:
    using group_map = Configuration::group_map;
    using plugin_list = std::vector<Plugin>;
public:
                        Effect(std::string name,
                               group_map groups,
                               plugin_list plugins);

    const std::string & name() const { return m_name; }
    const group_map &   groups() const { return m_groups; }
    const plugin_list & plugins() const { return m_plugins; }

private:
    std::string         m_name;         ///< User-readable name
    group_map           m_groups;       ///< Map of key group names to lists of key names
    plugin_list         m_plugins;      ///< List of plugin configurations for this effect
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
        struct Entry {
            std::string key;
            std::string value;
            std::regex  regex;
        };
        using entry_list = std::vector<Entry>;
    public:
        using filter_map = std::vector<std::pair<std::string, std::string>>;
    public:
                            Lookup() = default;
                            Lookup(filter_map filters);

        bool                match(const Context &) const;
    private:
        static entry_list   buildRegexps(filter_map);
    private:
        entry_list  m_entries;
    };

    using device_list = std::vector<std::string>;
    using effect_list = std::vector<std::string>;
public:
                        Profile(std::string name,
                                Lookup lookup,
                                device_list devices,
                                effect_list effects);

    const std::string & name() const { return m_name; }
    const Lookup &      lookup() const { return m_lookup; }
    const device_list & devices() const { return m_devices; }
    const effect_list & effects() const { return m_effects; }

private:
    std::string         m_name;         ///< User-readable name
    Lookup              m_lookup;       ///< Matched against a context to determine whether to apply the profile
    device_list         m_devices;      ///< List of device names this profile is restricted to
    effect_list         m_effects;      ///< List of effect names this profile activates
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
    using conf_map = std::vector<std::pair<std::string, std::string>>;
public:
                        Plugin(std::string name, conf_map items);
    const std::string & name() const { return m_name; }
    const conf_map &    items() const { return m_items; }
    const std::string & operator[](const std::string &) const;
private:
    std::string         m_name;         ///< Plugin name as registered in plugin manager
    conf_map            m_items;        ///< Flat string map passed through to plugin
};

};

#endif
