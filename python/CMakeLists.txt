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
project (python-keyleds VERSION 0.1 LANGUAGES C)

##############################################################################
# Sources

# List of sources
set(keyleds_SRCS
    keyleds.c
)

##############################################################################
# Dependencies

# Build python bindings unless disabled or dependincies are missing
find_package(PythonLibs)
IF (NOT PYTHONLIBS_FOUND)
    MESSAGE(WARNING "Python libs not found, not building python bindings")
    RETURN()
ENDIF()
set(keyleds_INCLUDES ${keyleds_INCLUDES} ${PYTHON_INCLUDE_DIRS})
set(keyleds_DEPS ${keyleds_DEPS} ${PYTHON_LIBRARIES})

find_program(CYTHON cython3 cython)
IF (NOT CYTHON)
    MESSAGE(WARNING "cython is missing, not building python bindings")
    RETURN()
ENDIF()


##############################################################################
# Commands

add_custom_command(OUTPUT keyleds.c
                   COMMAND ${CYTHON}
                   ARGS $(C_INCLUDES) -o keyleds.c ${CMAKE_CURRENT_SOURCE_DIR}/keyleds.pyx
                   MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/keyleds.pyx
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/keyleds.pxd
                   COMMENT "Generating C source keyleds.c")

##############################################################################
# Targets

# Python bindings
add_library(keyleds MODULE ${keyleds_SRCS})
set_target_properties(keyleds PROPERTIES PREFIX "")
target_include_directories(keyleds PRIVATE ${keyleds_INCLUDES})
target_link_libraries(keyleds libkeyleds ${keyleds_DEPS})

install(TARGETS keyleds LIBRARY DESTINATION lib)
