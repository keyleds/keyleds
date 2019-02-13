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
#ifndef KEYLEDSD_TOOLS_DYNAMIC_LIBRARY_H_A9903F9F
#define KEYLEDSD_TOOLS_DYNAMIC_LIBRARY_H_A9903F9F

#include <string>

namespace keyleds::tools {

/****************************************************************************/

/** Simple dynamic library wrapper
 *
 * A simple class to use RAII on dynamic libraries and wrap all OS details
 * of dynamic library handling.
 */
class DynamicLibrary final
{
    using handle_type = void *;
    static constexpr handle_type invalid_handle = nullptr;
private:
    explicit        DynamicLibrary(handle_type);
public:
                    DynamicLibrary() = default;
                    DynamicLibrary(DynamicLibrary &&) noexcept;
    DynamicLibrary& operator=(DynamicLibrary &&) noexcept;
                    ~DynamicLibrary();

    static DynamicLibrary load(const std::string & name, std::string * error = nullptr);

    const void *    getSymbol(const char * name);
    template <typename T> const T * getSymbol(const char * name)
     { return static_cast<const T *>(getSymbol(name)); }

                    operator bool() const { return m_handle != invalid_handle; }

private:
    handle_type     m_handle = invalid_handle;
};

/****************************************************************************/

} // namespace keyleds::tools

#endif
