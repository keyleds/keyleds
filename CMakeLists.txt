cmake_minimum_required (VERSION 2.8)
project (keyleds C)

##############################################################################
# Settings

option(WITHOUT_PYTHON "do not build python bindings" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -std=c99 -D_POSIX_C_SOURCE=200112L")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(libkeyleds)
add_subdirectory(keyledsctl)
IF (NOT WITHOUT_PYTHON)
    add_subdirectory(python)
ENDIF()
