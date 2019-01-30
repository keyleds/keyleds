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
/** System-specific path resolution
 *
 * Implements tools for finding files based on their purpose. On UNIX systems,
 * the intent is to follow the XDG Base Directory Specification:
 * https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 */
#ifndef TOOLS_PATH_H_B703CD61
#define TOOLS_PATH_H_B703CD61

#include <ios>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace tools::paths {

/****************************************************************************/

namespace detail {
    /// SFINAE-defined structure giving default openmode for a stream depending on its base type
    template <typename T, typename Enable = void> struct stream_traits {
        static constexpr bool is_file = false;
        static constexpr std::ios::openmode default_mode = static_cast<std::ios::openmode>(0);
    };

    template <typename T>
    struct stream_traits<T, typename std::enable_if_t<std::is_base_of_v<std::fstream, T>>> {
        static constexpr bool is_file = true;
        static constexpr typename T::openmode default_mode = std::ios::in | std::ios::out;
    };
    template <typename T>
    struct stream_traits<T, typename std::enable_if_t<std::is_base_of_v<std::ifstream, T>>> {
        static constexpr bool is_file = true;
        static constexpr typename T::openmode default_mode = std::ios::in;
    };
    template <typename T>
    struct stream_traits<T, typename std::enable_if_t<std::is_base_of_v<std::ofstream, T>>> {
        static constexpr bool is_file = true;
        static constexpr typename T::openmode default_mode = std::ios::out;
    };

    template <typename T> struct open_file_return {
        std::remove_cv_t<T> stream;
        std::string         path;
    };
} // namespace detail

/// Represents a file use type, typically associated to one or more system-dependent
/// locations.
enum class XDG { Cache, Config, Data, Runtime };

/// Attempts to open the specified file of given type
/// If path is absolute or starts with a dot, it is used as is. Otherwise,
/// system-dependent locations are searched depenting on file type and open mode.
/// On error, returns closed filebuf and empty path
std::optional<std::pair<std::filebuf, std::string>>
open_filebuf(XDG type, const std::string &, std::ios::openmode);

/// Returns the list of system-dependent locations that would be searched for
/// files of the given type. Those may differ depending on open mode, thus the read flag.
std::vector<std::string> getPaths(XDG type, bool read);


/// Opens the given file, using the file described by given path and XDG type.
template <typename T> std::optional<detail::open_file_return<T>>
open(XDG type, const std::string & path, typename T::openmode mode)
{
    static_assert(detail::stream_traits<T>::is_file);
    mode |= detail::stream_traits<T>::default_mode;

    auto filebuf = open_filebuf(type, path, mode);
    if (!filebuf) { return std::nullopt; }

    auto result = std::optional<detail::open_file_return<T>>{{ {}, std::move(filebuf->second) }};
    *result->stream.rdbuf() = std::move(filebuf->first);

    return result;
}

/****************************************************************************/

} // namespace tools::paths

#endif
