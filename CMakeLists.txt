cmake_minimum_required (VERSION 3.0)
project (keyleds LANGUAGES C CXX)

##############################################################################
# Settings

include(GNUInstallDirs)

option(WITH_PYTHON "build python bindings" OFF)
option(WITH_KEYLEDSD "build keyledsd daemon" ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -std=c99 -D_POSIX_C_SOURCE=200112L")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++14")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

string(REPLACE "-O3" "-Os" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

add_subdirectory(libkeyleds)
add_subdirectory(keyledsctl)
IF (WITH_KEYLEDSD)
    add_subdirectory(keyledsd)
ENDIF()
IF (WITH_PYTHON)
    add_subdirectory(python)
ENDIF()
