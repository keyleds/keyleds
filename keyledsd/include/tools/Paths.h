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

/****************************************************************************/

#include <ios>
#include <string>
#include <type_traits>
#include <vector>

namespace tools { namespace paths {

/****************************************************************************/

/// Represents a file use type, typically associated to one or more system-dependent
/// locations.
enum class XDG { Cache, Config, Data, Runtime };

/// Attempts to open the specified file of given type in the passed filbuf
/// If path is absolute or starths with a dot, it is used as is. Otherwise,
/// system-dependent locations are searched depenting on file type and open mode.
/// Errors leave filebuf in a bad state.
void open_filebuf(std::filebuf &, XDG type, const std::string & path, std::ios::openmode,
                  std::string * actualPath = nullptr);

/// Returns the list of system-dependent locations that would be searched for
/// files of the given type. Those may differ depending on open mode, thus the read flag.
std::vector<std::string> getPaths(XDG type, bool read);


namespace detail {
    /// SFINAE-defined structure giving default openmode for a stream depending on its base type
    template <typename T, typename Enable = void> struct stream_traits {
        static constexpr bool is_file = false;
        static constexpr std::ios::openmode default_mode = static_cast<std::ios::openmode>(0);
    };

    template <typename T>
    struct stream_traits<T, typename std::enable_if<std::is_convertible<T *, std::fstream *>::value>::type> {
        static constexpr bool is_file = true;
        static constexpr typename T::openmode default_mode = std::ios::in | std::ios::out;
    };
    template <typename T>
    struct stream_traits<T, typename std::enable_if<std::is_convertible<T *, std::ifstream *>::value>::type> {
        static constexpr bool is_file = true;
        static constexpr typename T::openmode default_mode = std::ios::in;
    };
    template <typename T>
    struct stream_traits<T, typename std::enable_if<std::is_convertible<T *, std::ofstream *>::value>::type>
    {
        static constexpr bool is_file = true;
        static constexpr typename T::openmode default_mode = std::ios::out;
    };
} // namespace detail

/// Opens the given file object, using the file described by given path and XDG type.
/// This is a simple wrapper that extracts the filebuf, feeds it to open_filebuf
/// and updates the file object's state flags to reflect the outcome.
//  @TODO: Changed for a rvalue-returning function, legacy workaround for gcc4 is
//  no longer useful.
template <typename T>
void open(T & file, XDG type, const std::string & path, typename T::openmode mode,
          std::string * actualPath = nullptr)
{
    static_assert(detail::stream_traits<T>::is_file, "open must be passed a file stream");
    mode |= detail::stream_traits<T>::default_mode;
    open_filebuf(*file.rdbuf(), type, path, mode, actualPath);
    if (!file.rdbuf()->is_open()) { file.setstate(std::ios::failbit); }
}


/****************************************************************************/

} } // namespace tools::paths

#endif
