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
project(libkeyleds VERSION 1.0 LANGUAGES C)
include(CheckCSourceCompiles)
include(CheckIncludeFile)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -std=c99 -D_POSIX_C_SOURCE=200112L -fvisibility=internal")

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories(BEFORE "${PROJECT_BINARY_DIR}" "include")

# List of sources
set(libkeyleds_SRCS
    src/device.c
    src/error.c
    src/feature_core.c
    src/feature_gamemode.c
    src/feature_layout.c
    src/feature_leds.c
    src/feature_reportrate.c
    src/feature_version.c
    src/hid_parser.c
    src/keys.c
    src/logging.c
    src/strings.c
)

##############################################################################
# Dependencies

# Required dependency on Linux' hidraw device
check_include_file(linux/hidraw.h HIDRAW_FOUND)
IF(NOT HIDRAW_FOUND)
    MESSAGE(SEND_ERROR "linux/hidraw.h not found -- is the target system a Linux box")
ENDIF()

# Optional Thread-local storage for error reporting
check_c_source_compiles("__thread int tls; int main() { return 0; }" GCC_THREAD_LOCAL_FOUND)
check_c_source_compiles("_Thread_local int tls; int main() { return 0; }" C11_THREAD_LOCAL_FOUND)

# Optional enhanced strerrror if available
check_c_source_compiles("#include <string.h>\nint main() { char buf[1]; return strerror_r(0, buf, 1); }"
                        POSIX_STRERROR_R_FOUND)

# Optional automatic translation of key names to/from Linux input keycodes
check_include_file(linux/input.h INPUT_CODES_FOUND)
IF(NOT INPUT_CODES_FOUND)
    MESSAGE(WARNING "linux/input.h not found, key names will not be available")
ENDIF()

configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

# Main library
add_library(libkeyleds SHARED ${libkeyleds_SRCS})
target_include_directories(libkeyleds PUBLIC "include")
set_target_properties(libkeyleds PROPERTIES POSITION_INDEPENDENT_CODE on)
set_target_properties(libkeyleds PROPERTIES PREFIX "")
set_target_properties(libkeyleds PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(libkeyleds PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS libkeyleds LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/keyleds.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
