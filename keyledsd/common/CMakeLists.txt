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
include(CheckCSourceCompiles)

##############################################################################
# Options

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64 OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL i686)
    set(KEYLEDSD_USE_MMX 1)
    set(KEYLEDSD_USE_SSE2 1)
    set(KEYLEDSD_USE_AVX2 1)
endif()

##############################################################################
# Sources

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(common_SRCS
    src/KeyDatabase.cxx
    src/RenderTarget.cxx
    src/accelerated.c
    src/accelerated_plain.c
    src/colors.cxx
    src/utils.cxx
)
if(KEYLEDSD_USE_MMX)
    set(common_SRCS ${common_SRCS} src/accelerated_mmx.c)
    set_source_files_properties("src/accelerated_mmx.c"
                                PROPERTIES COMPILE_FLAGS "-mmmx")
endif()
if(KEYLEDSD_USE_SSE2)
    set(common_SRCS ${common_SRCS} src/accelerated_sse2.c)
    set_source_files_properties("src/accelerated_sse2.c"
                                PROPERTIES COMPILE_FLAGS "-msse2")
endif()
if(KEYLEDSD_USE_AVX2)
    set(common_SRCS ${common_SRCS} src/accelerated_avx2.c)
    set_source_files_properties("src/accelerated_avx2.c"
                                PROPERTIES COMPILE_FLAGS "-mavx2")
endif()

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

configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

add_library(common SHARED ${common_SRCS})
target_include_directories(common PUBLIC "include")
target_link_libraries(common PRIVATE gcc_s gcc) # Work around a bug in gcc

set_target_properties(common PROPERTIES PREFIX "keyleds_")
set_target_properties(common PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(common PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS common LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
