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
#include "keyledsd/tools/DynamicLibrary.h"

#include <algorithm>
#include <dlfcn.h>
#include <stdexcept>

using keyleds::tools::DynamicLibrary;

/****************************************************************************/

DynamicLibrary::DynamicLibrary(handle_type handle)
 : m_handle(handle) {}

DynamicLibrary::~DynamicLibrary()
{
    if (m_handle != nullptr) { dlclose(m_handle); }
}

DynamicLibrary::DynamicLibrary(DynamicLibrary && other) noexcept
{
    using std::swap;
    swap(m_handle, other.m_handle);
}

DynamicLibrary & DynamicLibrary::operator=(DynamicLibrary && other) noexcept
{
    if (m_handle != nullptr) { dlclose(m_handle); }
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    return *this;
}

const void * DynamicLibrary::getSymbol(const char * name)
{
    return dlsym(m_handle, name);
}

DynamicLibrary DynamicLibrary::load(const std::string & name, std::string * error)
{
    handle_type handle = dlopen(name.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        if (error) {
            *error = dlerror();
            dlerror(); // let libdl clear allocated buffer
        }
        return {};
    }
    return DynamicLibrary(handle);
}
