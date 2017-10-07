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

/** Complete service configuration
 *
 * Holds all configuration data to run an instance of the service,
 * along with helper methods to load it from a file or program
 * arguments.
 */
class Configuration final
{
public:
    class EffectGroup;
    class Effect;
    class KeyGroup;
    class Profile;

    using path_list = std::vector<std::string>;
    using device_map = std::vector<std::pair<std::string, std::string>>;
    using key_group_list = std::vector<KeyGroup>;
    using effect_group_list = std::vector<EffectGroup>;
    using profile_list = std::vector<Profile>;
private:
                            Configuration(std::string path,
                                          path_list pluginPaths,
                                          device_map devices,
                                          key_group_list groups,
                                          effect_group_list effectGroups,
                                          profile_list profiles);
public:
                            Configuration() = default;

    const std::string &     path() const { return m_path; }
    const path_list &       pluginPaths() const { return m_pluginPaths; }
    const device_map &      devices() const { return m_devices; }
    const key_group_list &  keyGroups() const { return m_keyGroups; }
    const effect_group_list & effectGroups() const { return m_effectGroups; }
    const profile_list&     profiles() const { return m_profiles; }

public:
    static Configuration    loadFile(const std::string & path);

private:
    std::string             m_path;         ///< Configuration file path, if loaded from disk
    path_list               m_pluginPaths;  ///< List of directories to search for plugins
    device_map              m_devices;      ///< Map of device serials to device names
    key_group_list          m_keyGroups;    ///< Map of key group names to lists of key names
    effect_group_list       m_effectGroups; ///< Map of effect group names to configurations
    profile_list            m_profiles;     ///< List of profile configurations
};

/****************************************************************************/

/** EffectGroup configuration
 */
class Configuration::EffectGroup final
{
public:
    using key_group_list = Configuration::key_group_list;
    using effect_list = std::vector<Effect>;
public:
                            EffectGroup(std::string name,
                                        key_group_list keyGroups,
                                        effect_list effects);

    const std::string &     name() const { return m_name; }
    const key_group_list &  keyGroups() const { return m_keyGroups; }
    const effect_list &     effects() const { return m_effects; }

private:
    std::string             m_name;         ///< User-readable name
    key_group_list          m_keyGroups;    ///< Map of key group names to lists of key names
    effect_list             m_effects;      ///< List of effect configurations for this group
};

/****************************************************************************/

/** KeyGroup configuration
 */
class Configuration::KeyGroup final
{
public:
    using key_list = std::vector<std::string>;
public:
                            KeyGroup(std::string name, key_list keys);

    const std::string &     name() const { return m_name; }
    const key_list &        keys() const { return m_keys; }

private:
    std::string             m_name;         ///< User-readable name
    key_list                m_keys;         ///< List of key names
};

/****************************************************************************/

/** Profile configuration
 *
 * Holds the configuration of a single keyboard profile. A profile defines
 * a set of conditions that can be used to match a, and a set of
 * effect groups to apply when a context matches. The set of conditions is known
 * as a Lookup.
 */
class Configuration::Profile final
{
public:
    /// Filters a context to determine whether a profile should be enabled
    class Lookup final
    {
        struct Entry {
            std::string key;        ///< context entry key
            std::string value;      ///< string representation of the regex
            std::regex  regex;      ///< regex to match context entry value against
        };
        using entry_list = std::vector<Entry>;
    public:
        using string_map = std::vector<std::pair<std::string, std::string>>;
    public:
                            Lookup() = default;
                            Lookup(string_map filters);

        bool                match(const string_map &) const;
    private:
        static entry_list   buildRegexps(string_map);
    private:
        entry_list  m_entries;
    };

    using device_list = std::vector<std::string>;
    using effect_group_list = std::vector<std::string>;
public:
                        Profile(std::string name,
                                Lookup lookup,
                                device_list devices,
                                effect_group_list effectGroups);

    const std::string &     name() const { return m_name; }
    const Lookup &          lookup() const { return m_lookup; }
    const device_list &     devices() const { return m_devices; }
    const effect_group_list & effectGroups() const { return m_effectGroups; }

private:
    std::string             m_name;         ///< User-readable name
    Lookup                  m_lookup;       ///< Determines when to apply the profile
    device_list             m_devices;      ///< List of device names this profile is restricted to
    effect_group_list       m_effectGroups; ///< List of effect group names this profile activates
};

/****************************************************************************/

/** Effect configuration
 *
 * Holds the configuration of a single rendering effect. It is a simple string
 * map, and a effect name used to look it up in the effect manager.
 */
class Configuration::Effect final
{
public:
    using string_map = std::vector<std::pair<std::string, std::string>>;
public:
                        Effect(std::string name, string_map items);
    const std::string & name() const { return m_name; }
    const string_map &  items() const { return m_items; }
private:
    std::string         m_name;         ///< Effect name as registered in effect manager
    string_map          m_items;        ///< Flat string map passed through to effect
};

};

#endif
