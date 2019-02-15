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

#include <iosfwd>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace keyleds::service {

/****************************************************************************/

/** Complete service configuration
 *
 * Holds all configuration data to run an instance of the service,
 * along with helper methods to load it from a file or program
 * arguments.
 */
struct Configuration final
{
    struct EffectGroup;
    struct Effect;
    struct KeyGroup;
    struct Profile;

    class ParseError : public std::runtime_error { using runtime_error::runtime_error; };
    using string_list = std::vector<std::string>;
    using path_list = std::vector<std::string>;
    using device_map = std::vector<std::pair<std::string, std::string>>;
    using key_group_list = std::vector<KeyGroup>;
    using effect_group_list = std::vector<EffectGroup>;
    using profile_list = std::vector<Profile>;

    static Configuration parse(std::istream &);
    static Configuration loadFile(const std::string & path);

    std::string         path;           ///< Path of configuration on disk
    string_list         plugins;        ///< List of plugins to load on startup
    path_list           pluginPaths;    ///< List of directories to search for plugins
    device_map          devices;        ///< Map of device serials to device names
    key_group_list      keyGroups;      ///< Map of key group names to lists of key names
    effect_group_list   effectGroups;   ///< Map of effect group names to configurations
    profile_list        profiles;       ///< List of profile configurations
};

std::string getDeviceName(const Configuration & config, const std::string & serial);

/****************************************************************************/

/** EffectGroup configuration
 */
struct Configuration::EffectGroup final
{
    using key_group_list = Configuration::key_group_list;
    using effect_list = std::vector<Effect>;

    std::string     name;         ///< User-readable name
    key_group_list  keyGroups;    ///< Map of key group names to lists of key names
    effect_list     effects;      ///< List of effect configurations for this group
};

/****************************************************************************/

/** KeyGroup configuration
 */
struct Configuration::KeyGroup final
{
    using key_list = std::vector<std::string>;

    std::string name;         ///< User-readable name
    key_list    keys;         ///< List of key names
};

/****************************************************************************/

/** Profile configuration
 *
 * Holds the configuration of a single keyboard profile. A profile defines
 * a set of conditions that can be used to match a, and a set of
 * effect groups to apply when a context matches. The set of conditions is known
 * as a Lookup.
 */
struct Configuration::Profile final
{
public:
    /// Filters a context to determine whether a profile should be enabled
    class Lookup final
    {
        struct Entry;
        using entry_list = std::vector<Entry>;
        using string_map = std::vector<std::pair<std::string, std::string>>;
    public:
                            Lookup() = default;
        explicit            Lookup(string_map filters);
                            Lookup(Lookup &&) noexcept = default;
        Lookup &            operator=(Lookup &&) noexcept = default;
                            ~Lookup();

        bool                match(const string_map &) const;
    private:
        static entry_list   buildRegexps(string_map);
    private:
        entry_list  m_entries;
    };

    using device_list = std::vector<std::string>;
    using effect_group_list = std::vector<std::string>;

public:
    std::string         name;         ///< User-readable name
    Lookup              lookup;       ///< Determines when to apply the profile
    device_list         devices;      ///< List of device names this profile is restricted to
    effect_group_list   effectGroups; ///< List of effect group names this profile activates
};

/****************************************************************************/

/** Effect configuration
 *
 * Holds the configuration of a single rendering effect. It is a simple string
 * map, and a effect name used to look it up in the effect manager.
 */
struct Configuration::Effect final
{
    using string_map = std::vector<std::pair<std::string, std::string>>;

    std::string name;       ///< Effect name as registered in effect manager
    string_map  items;      ///< Flat string map passed through to effect
};

/****************************************************************************/

} // namespace keyleds::service

#endif
