cmake_minimum_required (VERSION 3.0)
project(keyledsctl VERSION 1.0 LANGUAGES C)

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories("${PROJECT_BINARY_DIR}" "include")

# List of sources
set(keyledsctl_SRCS
    src/dev_enum.c
    src/logging.c
    src/keyledsctl.c
    src/utils.c
)

##############################################################################
# Dependencies

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

configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

# CLI tool
add_executable(keyledsctl ${keyledsctl_SRCS})
target_link_libraries(keyledsctl libkeyleds ${keyledsctl_DEPS})

# Installing stuff (stub)
install(TARGETS keyledsctl DESTINATION ${CMAKE_INSTALL_BINDIR})
