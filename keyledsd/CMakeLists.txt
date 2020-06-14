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
project(keyledsd VERSION 1.1.1 LANGUAGES C CXX)

##############################################################################
# Options

option(NO_DBUS "Do not compile DBus support" OFF)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64 OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL i686)
    set(KEYLEDSD_USE_SSE2 1)
    set(KEYLEDSD_USE_AVX2 1)
endif()

##############################################################################
# Sources

set(common_SRCS
    src/tools/accelerated.c
    src/tools/accelerated_plain.c
    $<$<BOOL:${KEYLEDSD_USE_SSE2}>:src/tools/accelerated_sse2.c>
    $<$<BOOL:${KEYLEDSD_USE_AVX2}>:src/tools/accelerated_avx2.c>
    src/tools/utils.cxx
    src/KeyDatabase.cxx
    src/RenderTarget.cxx
    src/colors.cxx
)
set_source_files_properties("src/tools/accelerated_sse2.c" PROPERTIES COMPILE_FLAGS "-msse2")
set_source_files_properties("src/tools/accelerated_avx2.c" PROPERTIES COMPILE_FLAGS "-mavx2")

set(core_SRCS
    src/device/Device.cxx
    src/device/LayoutDescription.cxx
    src/service/Configuration.cxx
    src/service/EffectManager.cxx
    src/service/RenderLoop.cxx
    src/tools/AnimationLoop.cxx
    src/tools/DynamicLibrary.cxx
    src/tools/Paths.cxx
    src/tools/XWindow.cxx
    src/tools/YAMLParser.cxx
    src/logging.cxx
)

set(service_SRCS
    src/device/Logitech.cxx
    $<$<NOT:$<BOOL:${NO_DBUS}>>:src/service/dbus/DeviceManager.cxx>
    $<$<NOT:$<BOOL:${NO_DBUS}>>:src/service/dbus/Service.cxx>
    src/service/DeviceManager.cxx
    src/service/DeviceManager_util.cxx
    src/service/DisplayManager.cxx
    src/service/EffectService.cxx
    src/service/Service.cxx
    src/service/StaticModuleRegistry.cxx
    src/tools/DeviceWatcher.cxx
    src/tools/Event.cxx
    src/tools/FileWatcher.cxx
    src/tools/XContextWatcher.cxx
    src/tools/XInputWatcher.cxx
    src/main.cxx
)
set_source_files_properties("src/device/Logitech.cxx" PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")

set(test-common_SRCS
    tests/tools/utils.cxx
    tests/KeyDatabase.cxx
    tests/RenderTarget.cxx
    tests/colors.cxx
)

##############################################################################
# Options & dependencies

add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
    $<$<COMPILE_LANGUAGE:C>:-std=c99>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fstrict-enums>
    -fvisibility=hidden -ffast-math
)

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

find_library(LIBYAML yaml)
IF(NOT LIBYAML)
    MESSAGE(SEND_ERROR "libyaml is required for keyledsd")
ENDIF(NOT LIBYAML)

find_library(LIBUDEV udev)
IF(NOT LIBUDEV)
    MESSAGE(SEND_ERROR "libudev is required for keyledsd")
ENDIF(NOT LIBUDEV)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

IF(NOT NO_DBUS)
    pkg_check_modules(LIBSYSTEMD REQUIRED libsystemd)
ENDIF()

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
# Targets

configure_file("include/config.h.in" "config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(common SHARED ${common_SRCS})
target_include_directories(common PUBLIC "include")
target_link_libraries(common PRIVATE gcc_s gcc) # Work around a bug in gcc
set_target_properties(common PROPERTIES PREFIX "keyleds_"
                                        VERSION ${PROJECT_VERSION}
                                        SOVERSION ${PROJECT_VERSION_MAJOR})

add_library(core STATIC ${core_SRCS})
target_compile_definitions(core PRIVATE KEYLEDSD_INTERNAL)
target_include_directories(core PUBLIC "include")
target_include_directories(core SYSTEM PUBLIC ${X11_Xlib_INCLUDE_PATH} ${X11_Xinput_INCLUDE_PATH})
target_link_libraries(core common
    ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${LIBYAML} ${X11_X11_LIB} ${X11_Xinput_LIB}
)

add_executable(keyledsd ${service_SRCS})
target_compile_definitions(keyledsd PRIVATE KEYLEDSD_INTERNAL)
target_compile_definitions(keyledsd PRIVATE KEYLEDSD_MODULES_STATIC=1 $<$<BOOL:${NO_DBUS}>:NO_DBUS>)
target_include_directories(keyledsd PRIVATE include ${LIBUV_INCLUDE_DIRS})
target_link_libraries(keyledsd common core libkeyleds ${LIBUDEV} ${LIBUV_LIBRARIES})
IF(NOT NO_DBUS)
    target_include_directories(keyledsd SYSTEM PRIVATE ${LIBSYSTEMD_INCLUDE_DIRS})
    target_link_libraries(keyledsd ${LIBSYSTEMD_LIBRARIES})
ENDIF()

##############################################################################
# Tests

IF(WITH_TESTS)
    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)

    add_executable(test-common ${test-common_SRCS})
    target_compile_options(test-common PRIVATE "-Wno-unevaluated-expression")
    target_include_directories(test-common SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
    target_link_libraries(test-common common ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

    add_test(NAME common COMMAND test-common)

    find_package(benchmark)
    IF(benchmark_FOUND)
        add_executable(bench-rendertarget tests/RenderTarget_bench.cxx)
        target_include_directories(bench-rendertarget SYSTEM PRIVATE ${benchmark_INCLUDE_DIRS})
        target_link_libraries(bench-rendertarget common ${benchmark_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    ENDIF(benchmark_FOUND)
ENDIF(WITH_TESTS)

##############################################################################
# Subtargets and data files

add_subdirectory(plugins)

install(TARGETS common LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS keyledsd DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY effects/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/effects
        FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY layouts/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/layouts
        FILES_MATCHING PATTERN "*.yaml")
install(FILES keyledsd.conf.sample keyledsd.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME})
