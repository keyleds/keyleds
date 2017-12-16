# Keyleds -- Gaming keyboard tool
# Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 3.0)

##############################################################################
# Settings

option(WITH_LUA "build LUA scripting engine" ON)

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories("${CMAKE_CURRENT_BINARY_DIR}" "include")

##############################################################################
# Dependencies

find_package(PkgConfig)

if(WITH_LUA)
    pkg_search_module(LUA REQUIRED luajit lua-5.3 lua-5.2)
endif(WITH_LUA)

##############################################################################
# Targets

foreach(module breathe feedback fill stars wave)
    add_library(fx_${module} MODULE src/${module}.cxx)
    target_link_libraries(fx_${module} common)
    set_target_properties(fx_${module} PROPERTIES PREFIX "")
    set(module_TARGETS ${module_TARGETS} fx_${module})
endforeach()


IF(WITH_LUA)
    add_library(fx_lua MODULE
        src/lua/Environment.cxx
        src/lua/LuaEffect.cxx
        src/lua/lua_Interpolator.cxx
        src/lua/lua_Key.cxx
        src/lua/lua_KeyDatabase.cxx
        src/lua/lua_KeyGroup.cxx
        src/lua/lua_RGBAColor.cxx
        src/lua/lua_RenderTarget.cxx
        src/lua/lua_Thread.cxx
        src/lua/lua_common.cxx
        src/lua/lua_types.cxx
        src/lua.cxx
    )
    target_compile_options(fx_lua PRIVATE ${LUA_CFLAGS_OTHER})
    target_include_directories(fx_lua PRIVATE ${LUA_INCLUDE_DIRS})
    target_link_libraries(fx_lua common ${LUA_LIBRARIES})
    set_target_properties(fx_lua PROPERTIES PREFIX "")
    set(module_TARGETS ${module_TARGETS} fx_lua)
ENDIF(WITH_LUA)

##############################################################################
# Installing stuff

install(TARGETS ${module_TARGETS} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
IF(WITH_LUA)
    install(DIRECTORY effects/
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/effects
            FILES_MATCHING PATTERN "*.lua")
ENDIF(WITH_LUA)
