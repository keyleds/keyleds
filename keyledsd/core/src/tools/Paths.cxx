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
#include "tools/Paths.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include "config.h"

using tools::paths::XDG;

/****************************************************************************/

struct XDGVariables {
    XDG      type;           ///< which XDG variable
    const char *    home;           ///< environment variable name
    const char *    homeDefault;    ///< default value if not set
    const char *    dirs;           ///< environment variable name
    const char *    dirsDefault;    ///< default value if not set
    const char *    extra;          ///< hard-coded value from compile-time conf
};

static constexpr std::array<XDGVariables, 4> variables = {{
    { XDG::Cache,
      "XDG_CACHE_HOME", "${HOME}/.cache",
      nullptr, nullptr, nullptr },
    { XDG::Config,
      "XDG_CONFIG_HOME", "${HOME}/.config",
      "XDG_CONFIG_DIRS", "/etc/xdg",
      SYS_CONFIG_DIR },
    { XDG::Data,
      "XDG_DATA_HOME", "${HOME}/.local/share",
      "XDG_DATA_DIRS", "/usr/local/share/:/usr/share/",
      SYS_DATA_DIR },
    { XDG::Runtime,
      "XDG_RUNTIME_DIR", "/tmp",
      nullptr, nullptr, nullptr }
}};
static_assert(variables.back().type == XDG::Runtime,
              "Last variable is not the expected one - is length correct?");

static std::string expandVars(const std::string & value)
{
    static const std::regex varRe("\\$\\{([^}]+)\\}");

    auto it = std::sregex_iterator(value.begin(), value.end(), varRe);
    const auto endIt = std::sregex_iterator();
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

static std::string canonicalPath(const std::string & path)
{
#if _POSIX_C_SOURCE >= 200809L
    std::unique_ptr<char[], void(*)(void*)> result(
        realpath(path.c_str(), nullptr), free
    );
    if (result == nullptr) { return {}; }
    return std::string(result.get());
#else
    char buffer[PATH_MAX];
    if (realpath(path.c_str(), buffer) == nullptr) { return {}; }
    return std::string(buffer);
#endif
}

/****************************************************************************/

std::vector<std::string> tools::paths::getPaths(XDG type, bool extra)
{
    auto specIt = std::find_if(variables.begin(), variables.end(),
                               [type](auto & item) { return item.type == type; });
    if (specIt == variables.end()) {
        throw std::out_of_range("Invalid XDG variable");
    }

    std::vector<std::string> paths;

    // Lookup main directory
    if (specIt->home != nullptr) {
        const char * cvalue = std::getenv(specIt->home);
        if (cvalue != nullptr && cvalue[0] != '\0') {
            paths.emplace_back(std::string(cvalue));
        } else {
            paths.emplace_back(expandVars(std::string(specIt->homeDefault)));
        }
    }

    // If not writing, search additional directories
    if (extra && specIt->dirs != nullptr) {
        const char * cvalue = std::getenv(specIt->dirs);
        auto svalue = std::string(cvalue != nullptr ? cvalue : "");
        if (svalue.empty()) {
            svalue = expandVars(std::string(specIt->dirsDefault));
        }

        std::istringstream buf(svalue);
        std::string item;
        while (std::getline(buf, item, ':')) { paths.emplace_back(item); }

        if (specIt->extra) { paths.emplace_back(specIt->extra); }
    }
    return paths;
}

void tools::paths::open_filebuf(std::filebuf & buf, XDG type, const std::string & path,
                                std::ios::openmode mode, std::string * actualPath)
{
    if (path.empty()) { throw std::runtime_error("empty path"); }

    // Handle simple cases not using dynamic lookup
    if (path[0] == '/' || path[0] == '.') {
        if (buf.open(path, mode) != nullptr) {
            if (actualPath != nullptr) { *actualPath = path; }
        }
        return;
    }

    auto dirs = getPaths(type, (mode & std::ios::out) == 0);

    // Actually look for file
    for (const auto & dir : dirs) {
        auto fullPath = canonicalPath(dir + "/" + path);
        if (!fullPath.empty() && buf.open(fullPath, mode)) {
            if (actualPath) { *actualPath = fullPath; }
            return;
        }
    }
}
