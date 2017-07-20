cmake_minimum_required (VERSION 2.8)
project(libkeyleds C)
include(CheckCSourceCompiles)
include(CheckIncludeFile)

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories(BEFORE "${PROJECT_BINARY_DIR}" "include")

# List of sources
set(libkeyleds_SRCS
    src/device.c
    src/error.c
    src/feature_core.c
    src/feature_gamemode.c
    src/feature_layout.c
    src/feature_leds.c
    src/feature_reportrate.c
    src/feature_version.c
    src/hid_parser.c
    src/keys.c
    src/logging.c
    src/strings.c
)

##############################################################################
# Dependencies

# Required dependency on Linux' hidraw device
check_include_file(linux/hidraw.h HIDRAW_FOUND)
IF(NOT HIDRAW_FOUND)
    MESSAGE(SEND_ERROR "linux/hidraw.h not found -- is the target system a Linux box")
ENDIF()

# Optional Thread-local storage for error reporting
check_c_source_compiles("__thread int tls; int main() { return 0; }" GCC_THREAD_LOCAL_FOUND)
check_c_source_compiles("_Thread_local int tls; int main() { return 0; }" C11_THREAD_LOCAL_FOUND)

# Optional enhanced strerrror if available
check_c_source_compiles("#include <string.h>\nint main() { char buf[1]; return strerror_r(0, buf, 1); }"
                        POSIX_STRERROR_R_FOUND)

# Optional automatic translation of key names to/from Linux input keycodes
check_include_file(linux/input-event-codes.h INPUT_EVENT_CODES_FOUND)
IF(NOT INPUT_EVENT_CODES_FOUND)
    MESSAGE(WARNING "linux/input-event-codes.h not found, key names will not be available")
ENDIF()

configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

# Main library
add_library(libkeyleds STATIC ${libkeyleds_SRCS})
target_include_directories(libkeyleds PUBLIC "include")
set_target_properties(libkeyleds PROPERTIES POSITION_INDEPENDENT_CODE on)
set_target_properties(libkeyleds PROPERTIES PREFIX "")

install(TARGETS libkeyleds ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/keyleds.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
