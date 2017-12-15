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
# Options

option(NO_DBUS "Do not compile DBus support" OFF)

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# List of sources
set(service_SRCS
    src/device/Logitech.cxx
    src/effect/EffectService.cxx
    src/effect/StaticModuleRegistry.cxx
    src/tools/DeviceWatcher.cxx
    src/tools/FileWatcher.cxx
    src/tools/XContextWatcher.cxx
    src/tools/XInputWatcher.cxx
    src/DeviceManager.cxx
    src/DisplayManager.cxx
    src/Service.cxx
    src/main.cxx
)
if(NOT NO_DBUS)
    set(service_SRCS ${service_SRCS}
        src/dbus/DeviceManagerAdaptor.cxx
        src/dbus/ServiceAdaptor.cxx)
endif()

##############################################################################
# Dependencies

set(QT_MODULES Core)
IF(NOT NO_DBUS)
    set(QT_MODULES ${QT_MODULES} DBus)
ENDIF()

find_package(Qt5 COMPONENTS ${QT_MODULES} REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})
set(service_DEPS ${service_DEPS} Qt5Core)
qt5_wrap_cpp(moc_sources
    include/keyledsd/device/Logitech.h
    include/keyledsd/DeviceManager.h
    include/keyledsd/DisplayManager.h
    include/keyledsd/Service.h
    include/tools/DeviceWatcher.h
    include/tools/FileWatcher.h
    include/tools/XContextWatcher.h
    include/tools/XInputWatcher.h
)
set(service_SRCS ${service_SRCS} ${moc_sources})

IF(NOT NO_DBUS)
    include_directories(${Qt5DBus_INCLUDE_DIRS})
    set(service_DEPS ${service_DEPS} Qt5DBus)
    qt5_wrap_cpp(moc_sources
        include/keyledsd/dbus/DeviceManagerAdaptor.h
        include/keyledsd/dbus/ServiceAdaptor.h
    )
    set(service_SRCS ${service_SRCS} ${moc_sources})
ENDIF()

find_library(LIBUDEV udev)
IF(NOT LIBUDEV)
    MESSAGE(SEND_ERROR "libudev is required for keyledsd")
ENDIF(NOT LIBUDEV)
set(service_DEPS ${service_DEPS} ${LIBUDEV})

configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

# Binary
add_executable(${PROJECT_NAME} ${service_SRCS})
target_compile_definitions(${PROJECT_NAME} PRIVATE KEYLEDSD_MODULES_STATIC=1)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} common core libkeyleds ${service_DEPS})

##############################################################################
# Installing stuff
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
