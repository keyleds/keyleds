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
project (keyleds LANGUAGES C CXX)

##############################################################################
# Settings

include(GNUInstallDirs)

option(WITH_PYTHON "build python bindings" OFF)
option(WITH_KEYLEDSD "build keyledsd daemon" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

##############################################################################
# Work around Qt script selecting wrong tools while cross-compiling

IF(WITH_KEYLEDSD AND CMAKE_CROSSCOMPILING)
    execute_process(COMMAND qtchooser -qt=5 -print-env
                    COMMAND grep ^QTTOOLDIR=
                    COMMAND sed s/^QTTOOLDIR=\"\\\(.*\\\)\"$/\\1/
                    OUTPUT_VARIABLE NATIVE_QT_BINDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(QT_MOC_EXECUTABLE ${NATIVE_QT_BINDIR}/moc)
    add_executable(Qt5::moc IMPORTED)
    set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION ${QT_MOC_EXECUTABLE})
    add_executable(Qt5::rcc IMPORTED)
    set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION ${NATIVE_QT_BINDIR}/rcc)
    add_executable(Qt5::qdbuscpp2xml IMPORTED)
    set_property(TARGET Qt5::qdbuscpp2xml PROPERTY IMPORTED_LOCATION ${NATIVE_QT_BINDIR}/qdbuscpp2xml)
    add_executable(Qt5::qdbusxml2cpp IMPORTED)
    set_property(TARGET Qt5::qdbuscpp2xml PROPERTY IMPORTED_LOCATION ${NATIVE_QT_BINDIR}/qdbusxml2cpp)
ENDIF(WITH_KEYLEDSD AND CMAKE_CROSSCOMPILING)

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
