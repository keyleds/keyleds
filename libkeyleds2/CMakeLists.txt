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
project(libkeyleds2 VERSION 0.1 LANGUAGES CXX)


##############################################################################
# Sources

set(libkeyleds2_SRCS
    src/HIDParser.cxx
)

set(test-libkeyleds2_SRCS
    tests/HIDParser.cxx
)

##############################################################################
# Options & dependencies

add_compile_options(-std=c++17 -fno-rtti -fstrict-enums -fvisibility=hidden)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

add_library(libkeyleds2 SHARED ${libkeyleds2_SRCS})
target_include_directories(libkeyleds2 PUBLIC "include")
target_include_directories(libkeyleds2 PRIVATE ${PROJECT_BINARY_DIR} ${LIBUV_INCLUDE_DIRS})
target_link_libraries(libkeyleds2 PUBLIC ${LIBUV_LIBRARIES})
set_target_properties(libkeyleds2 PROPERTIES PREFIX "" VERSION ${PROJECT_VERSION}
                                             SOVERSION ${PROJECT_VERSION_MAJOR})

##############################################################################
# Tests

IF(WITH_TESTS)
    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)

    add_executable(test-libkeyleds2 ${test-libkeyleds2_SRCS})
    target_include_directories(test-libkeyleds2 SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
    target_link_libraries(test-libkeyleds2 libkeyleds2 ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    add_test(NAME libkeyleds2 COMMAND test-libkeyleds2)
ENDIF(WITH_TESTS)

add_executable(fuzz-hidparser tests/HIDParser_fuzz.cxx)
target_link_libraries(fuzz-hidparser libkeyleds2)

##############################################################################
# Install

install(TARGETS libkeyleds2 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
