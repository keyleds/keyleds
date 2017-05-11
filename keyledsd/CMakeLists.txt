cmake_minimum_required (VERSION 2.8)
project(keyledsd CXX)
include(CheckCXXSourceCompiles)

##############################################################################
# Options

if(NOT KEYLEDSD_DATA_PREFIX)
    set(KEYLEDSD_DATA_PREFIX ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
endif()

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories("${PROJECT_BINARY_DIR}" "include")

# List of sources
set(keyledsd_SRCS
    src/keyledsd/Configuration.cxx
    src/keyledsd/Device.cxx
    src/keyledsd/DeviceManager.cxx
    src/keyledsd/DeviceManagerAdaptor.cxx
    src/keyledsd/Layout.cxx
    src/keyledsd/Service.cxx
    src/keyledsd/ServiceAdaptor.cxx
    src/tools/accelerated.c
    src/tools/accelerated_mmx.c
    src/tools/accelerated_plain.c
    src/tools/accelerated_sse2.c
    src/tools/AnimationLoop.cxx
    src/tools/DeviceWatcher.cxx
    src/tools/SessionWatcher.cxx
    src/tools/YAMLParser.cxx
    src/logging.cxx
    src/main.cxx
)
set_source_files_properties(src/tools/accelerated_mmx.cxx PROPERTIES COMPILE_FLAGS -mmmx)
set_source_files_properties(src/tools/accelerated_sse2.cxx PROPERTIES COMPILE_FLAGS -msse2)

##############################################################################
# Dependencies

find_package(Qt4 REQUIRED QtCore)
qt4_wrap_cpp(moc_sources
    include/keyledsd/Device.h
    include/keyledsd/DeviceManager.h
    include/keyledsd/DeviceManagerAdaptor.h
    include/keyledsd/Service.h
    include/keyledsd/ServiceAdaptor.h
    include/tools/AnimationLoop.h
    include/tools/DeviceWatcher.h
    include/tools/SessionWatcher.h
)
set(keyledsd_SRCS ${keyledsd_SRCS} ${moc_sources})

find_library(LIBUDEV udev)
IF(NOT LIBUDEV)
    MESSAGE(SEND_ERROR "libudev is required for keyledsd")
ENDIF(NOT LIBUDEV)

find_library(LIBRT rt)
IF(NOT LIBRT)
    MESSAGE(SEND_ERROR "librt is required for keyledsd")
ENDIF(NOT LIBRT)

find_library(LIBYAML yaml)
IF(NOT LIBYAML)
    MESSAGE(SEND_ERROR "libyaml is required for keyledsd")
ENDIF(NOT LIBYAML)

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
set(keyledsd_DEPS ${keyledsd_DEPS} ${LIBXML2_LIBRARIES})

set(keyledsd_DEPS "${keyledsd_DEPS}"
    Qt4::QtCore Qt4::QtDBus
    ${LIBUDEV} ${LIBRT} ${LIBYAML}
)
configure_file("include/config.h.in" "config.h")

##############################################################################
# Targets

# CLI tool
add_executable(${PROJECT_NAME} ${keyledsd_SRCS})
target_link_libraries(${PROJECT_NAME} libkeyleds ${keyledsd_DEPS})

# Installing stuff (stub)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY layouts/ DESTINATION ${KEYLEDSD_DATA_PREFIX}
        FILES_MATCHING PATTERN "*.xml")
