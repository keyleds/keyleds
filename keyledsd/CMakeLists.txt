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
project(keyledsd VERSION 0.7.9 LANGUAGES CXX)

##############################################################################
# Options

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -fno-rtti -fvisibility=hidden -fstrict-enums -ffast-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wcast-align -Wcast-qual -Wconversion -Wsign-conversion -Wshadow -Wstrict-overflow=3")

##############################################################################
# Subtargets and data files

configure_file("config.h.in" "keyledsd_config.h")
include_directories(${PROJECT_BINARY_DIR})

add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(service)

install(DIRECTORY effects/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/effects
        FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY layouts/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/layouts
        FILES_MATCHING PATTERN "*.yaml")
install(FILES keyledsd.conf.sample keyledsd.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME})
