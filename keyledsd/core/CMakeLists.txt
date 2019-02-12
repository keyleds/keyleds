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
# Sources

# List of sources
set(core_SRCS
    src/keyledsd/Configuration.cxx
    src/keyledsd/Device.cxx
    src/keyledsd/EffectManager.cxx
    src/keyledsd/LayoutDescription.cxx
    src/keyledsd/RenderLoop.cxx
    src/tools/AnimationLoop.cxx
    src/tools/DynamicLibrary.cxx
    src/tools/Paths.cxx
    src/tools/XWindow.cxx
    src/tools/YAMLParser.cxx
    src/logging.cxx
)

##############################################################################
# Dependencies

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

find_library(LIBYAML yaml)
IF(NOT LIBYAML)
    MESSAGE(SEND_ERROR "libyaml is required for keyledsd")
ENDIF(NOT LIBYAML)


configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

add_library(core STATIC ${core_SRCS})
target_include_directories(core PUBLIC "include" PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(core PUBLIC ${X11_Xlib_INCLUDE_PATH} ${X11_Xinput_INCLUDE_PATH})
target_link_libraries(core common
    ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${LIBYAML} ${X11_X11_LIB} ${X11_Xinput_LIB}
)
