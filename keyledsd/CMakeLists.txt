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
project(keyledsd VERSION 0.6.1 LANGUAGES CXX)
include(CheckCSourceCompiles)

##############################################################################
# Options

option(NO_DBUS "Do not compile DBus support" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++14 -fPIC -fno-rtti -fstrict-enums -fvisibility=hidden")

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64 OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL i686)
    set(KEYLEDSD_USE_MMX 1)
    set(KEYLEDSD_USE_SSE2 1)
endif()

set(keyledsd_STATIC_MODULES breathe feedback fill wave)
set(keyledsd_DYNAMIC_MODULES stars)

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories("${PROJECT_BINARY_DIR}" "include")

# List of sources
set(keyledsd_SRCS
    src/keyledsd/device/Device.cxx
    src/keyledsd/device/KeyDatabase.cxx
    src/keyledsd/device/LayoutDescription.cxx
    src/keyledsd/device/RenderLoop.cxx
    src/keyledsd/effect/EffectManager.cxx
    src/keyledsd/effect/EffectService.cxx
    src/keyledsd/effect/StaticModuleRegistry.cxx
    src/keyledsd/Configuration.cxx
    src/keyledsd/DeviceManager.cxx
    src/keyledsd/DisplayManager.cxx
    src/keyledsd/Service.cxx
    src/keyledsd/colors.cxx
    src/tools/accelerated.c
    src/tools/accelerated_plain.c
    src/tools/AnimationLoop.cxx
    src/tools/DeviceWatcher.cxx
    src/tools/DynamicLibrary.cxx
    src/tools/FileWatcher.cxx
    src/tools/Paths.cxx
    src/tools/XContextWatcher.cxx
    src/tools/XInputWatcher.cxx
    src/tools/XWindow.cxx
    src/tools/YAMLParser.cxx
    src/logging.cxx
    src/main.cxx
)
if(NOT NO_DBUS)
    set(keyledsd_SRCS ${keyledsd_SRCS}
        src/keyledsd/dbus/DeviceManagerAdaptor.cxx
        src/keyledsd/dbus/ServiceAdaptor.cxx)
endif()
if(KEYLEDSD_USE_MMX)
    set(keyledsd_SRCS ${keyledsd_SRCS} src/tools/accelerated_mmx.c)
    set_source_files_properties("src/tools/accelerated_mmx.c"
                                PROPERTIES COMPILE_FLAGS "-mmmx")
endif()
if(KEYLEDSD_USE_SSE2)
    set(keyledsd_SRCS ${keyledsd_SRCS} src/tools/accelerated_sse2.c)
    set_source_files_properties("src/tools/accelerated_sse2.c"
                                PROPERTIES COMPILE_FLAGS "-msse2")
endif()
foreach(module ${keyledsd_STATIC_MODULES})
    set(keyledsd_SRCS ${keyledsd_SRCS} src/plugins/${module}.cxx)
endforeach()

set(keyledsd_DEPS ${CMAKE_DL_LIBS})

##############################################################################
# Feature detection

check_c_source_compiles("int main() {
  return __builtin_cpu_supports(\"sse2\");
}" HAVE_BUILTIN_CPU_SUPPORTS)

check_c_source_compiles("
int foo_actual() { return 0; }
static int (*resolve_foo(void))()  { return foo_actual; }
int foo() __attribute__((ifunc(\"resolve_foo\")));
int main() { return foo(); }
" HAVE_IFUNC_ATTRIBUTE)

##############################################################################
# Dependencies

set(QT_MODULES Core)
IF(NOT NO_DBUS)
    set(QT_MODULES ${QT_MODULES} DBus)
ENDIF()

find_package(Qt5 COMPONENTS ${QT_MODULES} REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})
set(keyledsd_DEPS ${keyledsd_DEPS} Qt5Core)
qt5_wrap_cpp(moc_sources
    include/keyledsd/device/Device.h
    include/keyledsd/DeviceManager.h
    include/keyledsd/DisplayManager.h
    include/keyledsd/Service.h
    include/tools/DeviceWatcher.h
    include/tools/FileWatcher.h
    include/tools/XContextWatcher.h
    include/tools/XInputWatcher.h
)
set(keyledsd_SRCS ${keyledsd_SRCS} ${moc_sources})

IF(NOT NO_DBUS)
    include_directories(${Qt5DBus_INCLUDE_DIRS})
    set(keyledsd_DEPS ${keyledsd_DEPS} Qt5DBus)
    qt5_wrap_cpp(moc_sources
        include/keyledsd/dbus/DeviceManagerAdaptor.h
        include/keyledsd/dbus/ServiceAdaptor.h
    )
    set(keyledsd_SRCS ${keyledsd_SRCS} ${moc_sources})
ENDIF()

find_package(Threads REQUIRED)
set(keyledsd_DEPS ${keyledsd_DEPS} ${CMAKE_THREAD_LIBS_INIT})

find_library(LIBUDEV udev)
IF(NOT LIBUDEV)
    MESSAGE(SEND_ERROR "libudev is required for keyledsd")
ENDIF(NOT LIBUDEV)
set(keyledsd_DEPS ${keyledsd_DEPS} ${LIBUDEV})

find_library(LIBYAML yaml)
IF(NOT LIBYAML)
    MESSAGE(SEND_ERROR "libyaml is required for keyledsd")
ENDIF(NOT LIBYAML)
set(keyledsd_DEPS ${keyledsd_DEPS} ${LIBYAML})

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
set(keyledsd_DEPS ${keyledsd_DEPS} ${LIBXML2_LIBRARIES})

find_package(X11)
include_directories(${X11_Xlib_INCLUDE_PATH} ${X11_Xinput_INCLUDE_PATH})
set(keyledsd_DEPS ${keyledsd_DEPS} ${X11_LIBRARIES} ${X11_Xinput_LIB})

configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

# Binary
add_executable(${PROJECT_NAME} ${keyledsd_SRCS})
target_compile_definitions(${PROJECT_NAME} PRIVATE KEYLEDSD_MODULES_STATIC=1)
target_link_libraries(${PROJECT_NAME} libkeyleds ${keyledsd_DEPS})

foreach(module ${keyledsd_DYNAMIC_MODULES})
    add_library(${module} MODULE src/plugins/${module}.cxx)
    set_target_properties(${module} PROPERTIES PREFIX fx_)
endforeach()

# Installing stuff
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${keyledsd_DYNAMIC_MODULES} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
install(DIRECTORY layouts/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/layouts
        FILES_MATCHING PATTERN "*.xml")
install(FILES keyledsd.conf.sample keyledsd.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME})
