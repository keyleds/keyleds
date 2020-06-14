# Keyleds -- Gaming keyboard tool
# Copyright (C) 2017-2020 Julien Hartmann, juli1.hartmann@gmail.com
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
project (keyleds VERSION 1.1.1 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})

##############################################################################
# Settings

include(GNUInstallDirs)

option(WITH_KEYLEDSD "build keyledsd daemon" ON)
option(WITH_PYTHON "build python bindings" OFF)
option(WITH_TESTS "build test suite" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

add_compile_options(
    -Wall -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-Wcast-align>
    $<$<COMPILE_LANGUAGE:CXX>:-Wcast-qual>
    $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
    $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
    $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
    $<$<COMPILE_LANGUAGE:CXX>:-Wstrict-overflow=3>
)

##############################################################################
# Global dependencies

IF(WITH_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
ENDIF(WITH_TESTS)

##############################################################################
# Include subprojects

add_subdirectory(libkeyleds)
add_subdirectory(keyledsctl)
IF (WITH_KEYLEDSD)
    add_subdirectory(keyledsd)
ENDIF()
IF (WITH_PYTHON)
    add_subdirectory(python)
ENDIF()
