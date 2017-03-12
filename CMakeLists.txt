cmake_minimum_required (VERSION 2.8)
project (keyleds C)
include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(FindPythonLibs)

##############################################################################
# Settings

option(WITHOUT_PYTHON "do not build python bindings" OFF)

# Configure compiler flags here

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -std=c99 -D_POSIX_C_SOURCE=200112L")

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories("${PROJECT_BINARY_DIR}" "include")

# List of sources
set(keyleds_SRCS
    src/keyleds/command.c
    src/keyleds/device.c
    src/keyleds/error.c
    src/keyleds/feature_core.c
    src/keyleds/feature_gamemode.c
    src/keyleds/feature_leds.c
    src/keyleds/feature_reportrate.c
    src/keyleds/feature_version.c
    src/keyleds/hid_parser.c
    src/keyleds/keys.c
    src/keyleds/logging.c
    src/keyleds/strings.c
)
set(pykeyleds_SRCS
    pykeyleds.c
)
set(keyledsctl_SRCS
    src/dev_enum.c
    src/logging.c
    src/keyledsctl.c
    src/utils.c
)

##############################################################################
# Dependencies

# Required dependency on Linux' hidraw device
check_include_file(linux/hidraw.h HIDRAW_FOUND)
IF(NOT HIDRAW_FOUND)
    MESSAGE(SEND_ERROR "linux/hidraw.h not found -- is the target system a Linux box")
ENDIF()

# Choose device enumeration method depending on available packages
find_library(LIBUDEV_FOUND udev)
IF(LIBUDEV_FOUND)
    MESSAGE(STATUS "Using libudev for device enumeration")
    set(keyledsctl_SRCS ${keyledsctl_SRCS} src/dev_enum_udev.c)
    set(keyledsctl_DEPS ${keyledsctl_DEPS} udev)
ELSE(LIBUDEV_FOUND)
    MESSAGE(WARNING "libudev not found, falling back to hardcoded enumeration")
    set(keyledsctl_SRCS ${keyledsctl_SRCS} src/dev_enum_hard.c)
ENDIF(LIBUDEV_FOUND)

# Optional Thread-local storage for error reporting
check_c_source_compiles("__thread int tls; int main() { return 0; }" GCC_THREAD_LOCAL_FOUND)
IF(NOT GCC_THREAD_LOCAL_FOUND)
    check_c_source_compiles("_Thread_local int tls; int main() { return 0; }" C11_THREAD_LOCAL_FOUND)
ENDIF()

# Optional enhanced strerrror if available
check_c_source_compiles("#include <string.h>\nint main() { char buf[1]; return strerror_r(0, buf, 1); }"
                        POSIX_STRERROR_R_FOUND)

# Optional automatic translation of key names to/from Linux input keycodes
check_include_file(linux/input-event-codes.h INPUT_EVENT_CODES_FOUND)
IF(NOT INPUT_EVENT_CODES_FOUND)
    MESSAGE(WARNING "linux/input-event-codes.h not found, key names will not be available")
ENDIF()

# Build python bindings unless disabled or dependincies are missing
IF (NOT WITHOUT_PYTHON)
    find_package(PythonLibs)
    find_program(CYTHON cython3 cython)
    IF (PYTHONLIBS_FOUND AND NOT CYTHON-NOTFOUND)
        set(pykeyleds_INCLUDES ${pykeyleds_INCLUDES} ${PYTHON_INCLUDE_DIRS})
        set(pykeyleds_DEPS ${pykeyleds_DEPS} ${PYTHON_LIBRARIES})
    ELSE()
        set(WITHOUT_PYTHON on)
        MESSAGE(WARNING "Either python libs or cython is missing, not building python bindings")
    ENDIF()
ENDIF (NOT WITHOUT_PYTHON)

configure_file("include/config.h.in" "config.h")

##############################################################################
# Commands

add_custom_command(OUTPUT pykeyleds.c
                   COMMAND ${CYTHON}
                   ARGS $(C_INCLUDES) -o pykeyleds.c ${CMAKE_CURRENT_SOURCE_DIR}/src/pykeyleds.pyx
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/pykeyleds.pyx ${CMAKE_CURRENT_SOURCE_DIR}/include/pykeyleds.pxd
                   COMMENT "Cython source")

##############################################################################
# Targets

# Main library
add_library(keyleds STATIC ${keyleds_SRCS})
set_target_properties(keyleds PROPERTIES POSITION_INDEPENDENT_CODE on)

# CLI tool
add_executable(keyledsctl ${keyledsctl_SRCS})
target_link_libraries(keyledsctl keyleds ${keyledsctl_DEPS})

# Python bindings
IF(NOT WITHOUT_PYTHON)
    add_library(pykeyleds SHARED ${pykeyleds_SRCS})
    target_include_directories(pykeyleds PRIVATE ${pykeyleds_INCLUDES})
    set_target_properties(pykeyleds PROPERTIES PREFIX "")
    target_link_libraries(pykeyleds keyleds ${pykeyleds_DEPS})
ENDIF()

# Installing stuff (stub)
install(TARGETS keyledsctl DESTINATION bin)

##############################################################################
# DEBUGGING - uncomment to show all cmake vars
# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
