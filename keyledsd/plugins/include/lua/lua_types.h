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
#ifndef KEYLEDS_PLUGINS_LUA_TYPES_H_1B6DBCFA
#define KEYLEDS_PLUGINS_LUA_TYPES_H_1B6DBCFA

#include "lua.hpp"
#include <type_traits>
#include <utility>

namespace keyleds { namespace lua {

/****************************************************************************/

template <typename T> struct metatable {};

namespace detail {
    void registerType(lua_State *, const char * name, const luaL_Reg * methods,
                      const luaL_Reg * metaMethods, bool weakTable);
    bool isType(lua_State * lua, int index, const char * type);
    void lua_pushref(lua_State * lua, const void * value, const char * name,
                     const luaL_Reg * metaMethods);
}

/****************************************************************************/

/// Registers the metatable with lua
template <typename T> void registerType(lua_State * lua)
{
    using meta = metatable<typename std::remove_cv<T>::type>;
    static_assert(std::is_pointer<T>::value || !meta::weak_table::value,
                  "Using a weak table requires stored object to be a pointer");
    detail::registerType(lua, meta::name, meta::methods, meta::meta_methods, meta::weak_table::value);
}

/// Tests whether value at index is of specified type
template <typename T> bool lua_is(lua_State * lua, int index)
{
    using meta = metatable<typename std::remove_cv<T>::type>;
    return detail::isType(lua, index, meta::name);
}

/// Returns a reference to userdata object of a type, checking type beforehand
template <typename T> T & lua_check(lua_State * lua, int index)
{
    using meta = metatable<typename std::remove_cv<T>::type>;
    return *static_cast<T *>(luaL_checkudata(lua, index, meta::name));
}

/// Returns a reference to userdata object of type, without checking
template <typename T> T & lua_to(lua_State * lua, int index)
{
    return *static_cast<T *>(const_cast<void *>(lua_topointer(lua, index)));
}

/// Pushes an object into a userdata, on the stack
template <typename T>
typename std::enable_if<!metatable<typename std::remove_cv<T>::type>::weak_table::value &&
                        std::is_copy_constructible<T>::value &&
                        !std::is_reference<T>::value>::type
lua_push(lua_State * lua, const T & value)
{
    using meta = metatable<typename std::remove_cv<T>::type>;
    void * ptr = lua_newuserdata(lua, sizeof(T));
    new (ptr) T(value);
    luaL_getmetatable(lua, meta::name);
    lua_setmetatable(lua, -2);
}

template <typename T>
typename std::enable_if<!metatable<typename std::remove_cv<T>::type>::weak_table::value &&
                        std::is_move_constructible<T>::value &&
                        !std::is_reference<T>::value>::type
lua_push(lua_State * lua, T && value)
{
    using meta = metatable<typename std::remove_cv<T>::type>;
    void * ptr = lua_newuserdata(lua, sizeof(T));
    new (ptr) T(std::forward<T>(value));
    luaL_getmetatable(lua, meta::name);
    lua_setmetatable(lua, -2);
}

template <typename T>
typename std::enable_if<metatable<typename std::remove_cv<T>::type>::weak_table::value &&
                        !std::is_reference<T>::value>::type
lua_push(lua_State * lua, T value)
{
    using meta = metatable<typename std::remove_cv<T>::type>;
    detail::lua_pushref(lua, static_cast<const void *>(value), meta::name, meta::meta_methods);
}

/****************************************************************************/

} } // namespace keyleds::lua

#endif
