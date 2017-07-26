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
#include <cstdlib>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include "tools/Paths.h"
#include "config.h"

struct XDGVariables {
    const char *    home;           ///< environment variable name
    const char *    homeDefault;    ///< default value if not set
    const char *    dirs;           ///< environment variable name
    const char *    dirsDefault;    ///< default value if not set
    const char *    extra;          ///< hard-coded value from compile-time conf
};
static const std::map<paths::XDG, XDGVariables> variables{
    { paths::XDG::Cache, {
        "XDG_CACHE_HOME",
        "${HOME}/.cache",
        nullptr, nullptr, nullptr
    } },
    { paths::XDG::Config, {
        "XDG_CONFIG_HOME",
        "${HOME}/.config",
        "XDG_CONFIG_DIRS",
        "/etc/xdg",
        SYS_CONFIG_DIR
    } },
    { paths::XDG::Data, {
        "XDG_DATA_HOME",
        "${HOME}/.local/share",
        "XDG_DATA_DIRS",
        "/usr/local/share/:/usr/share/",
        SYS_DATA_DIR
    } },
    { paths::XDG::Runtime, {
        "XDG_RUNTIME_DIR",
        "/tmp",
        nullptr, nullptr, nullptr
    } }
};

static std::string expandVars(const std::string & value)
{
    static const std::regex varRe("\\$\\{([^}]+)\\}");

    auto it = std::sregex_iterator(value.begin(), value.end(), varRe);
    auto endIt = std::sregex_iterator();
    auto result = value;

    std::ptrdiff_t offset = 0;
    for (; it != endIt; ++it) {
        auto creplacement = std::getenv(it->str(1).c_str());
        auto sreplacement = std::string(creplacement != nullptr ? creplacement : "");

        result.replace(it->position() + offset, it->length(), sreplacement);
        offset += sreplacement.size();
        offset -= it->length();
    }
    return result;
}

std::vector<std::string> paths::getPaths(XDG type, bool extra)
{
    const auto & spec = variables.at(type);
    std::vector<std::string> paths;

    // Lookup main directory
    if (spec.home != nullptr) {
        const char * cvalue = std::getenv(spec.home);
        if (cvalue != nullptr && cvalue[0] != '\0') {
            paths.push_back(std::string(cvalue));
        } else {
            paths.push_back(expandVars(std::string(spec.homeDefault)));
        }
    }

    // If not writing, search additional directories
    if (extra && spec.dirs != nullptr) {
        const char * cvalue = std::getenv(spec.dirs);
        auto svalue = std::string(cvalue != nullptr ? cvalue : "");
        if (svalue.empty()) {
            svalue = expandVars(std::string(spec.dirsDefault));
        }

        auto buf = std::istringstream(svalue);
        std::string item;
        while (std::getline(buf, item, ':')) { paths.push_back(item); }

        if (spec.extra) { paths.push_back(spec.extra); }
    }
    return paths;
}


std::filebuf paths::open_filebuf(XDG type, const std::string & path,
                                 std::ios::openmode mode)
{
    if (path.empty()) { throw std::runtime_error("empty path"); }
    auto buf = std::filebuf{};

    // Handle simple cases not using dynamic lookup
    if (path[0] == '/' || path[0] == '.') {
        buf.open(path, mode);
        return buf;
    }

    auto dirs = paths::getPaths(type, (mode & std::ios_base::out) == 0);

    // Actually look for file
    for (const auto & dir : dirs) {
        auto fullPath = dir + "/" + path;
        if (buf.open(fullPath, mode) != nullptr) { return buf; }
    }
    return buf;
}
