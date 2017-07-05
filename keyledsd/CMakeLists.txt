cmake_minimum_required (VERSION 2.8)
project(keyledsd CXX)
include(CheckCXXSourceCompiles)

##############################################################################
# Options

option(NO_DBUS "Do not compile DBus support" OFF)

if(NOT KEYLEDSD_DATA_PREFIX)
    set(KEYLEDSD_DATA_PREFIX ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++14 -fno-rtti -fstrict-enums")

##############################################################################
# Sources

# Look for header files in build directory (for config.h) and include dir
include_directories("${PROJECT_BINARY_DIR}" "include")

# List of sources
set(keyledsd_SRCS
    src/keyledsd/common.cxx
    src/keyledsd/Configuration.cxx
    src/keyledsd/Context.cxx
    src/keyledsd/ContextWatcher.cxx
    src/keyledsd/Device.cxx
    src/keyledsd/DeviceManager.cxx
    src/keyledsd/Layout.cxx
    src/keyledsd/PluginManager.cxx
    src/keyledsd/RenderLoop.cxx
    src/keyledsd/Service.cxx
    src/plugins/fill.cxx
    src/tools/accelerated.c
    src/tools/accelerated_mmx.c
    src/tools/accelerated_plain.c
    src/tools/accelerated_sse2.c
    src/tools/AnimationLoop.cxx
    src/tools/DeviceWatcher.cxx
    src/tools/XWindow.cxx
    src/tools/YAMLParser.cxx
    src/logging.cxx
    src/main.cxx
)
IF(NOT NO_DBUS)
    set(keyledsd_SRCS ${keyledsd_SRCS}
        src/dbus/DeviceManagerAdaptor.cxx
        src/dbus/ServiceAdaptor.cxx)
ENDIF()

set_source_files_properties(src/tools/accelerated_mmx.cxx PROPERTIES COMPILE_FLAGS -mmmx)
set_source_files_properties(src/tools/accelerated_sse2.cxx PROPERTIES COMPILE_FLAGS -msse2)

##############################################################################
# Dependencies

set(QT_MODULES QtCore)
IF(NOT NO_DBUS)
    set(QT_MODULES ${QT_MODULES} QtDBus)
ENDIF()

find_package(Qt4 REQUIRED ${QT_MODULES})
set(keyledsd_DEPS ${keyledsd_DEPS} Qt4::QtCore)
qt4_wrap_cpp(moc_sources
    include/keyledsd/ContextWatcher.h
    include/keyledsd/Device.h
    include/keyledsd/DeviceManager.h
    include/keyledsd/Service.h
    include/tools/DeviceWatcher.h
)
set(keyledsd_SRCS ${keyledsd_SRCS} ${moc_sources})

IF(NOT NO_DBUS)
    set(keyledsd_DEPS ${keyledsd_DEPS} Qt4::QtDBus)
    qt4_wrap_cpp(moc_sources
        include/dbus/DeviceManagerAdaptor.h
        include/dbus/ServiceAdaptor.h
    )
    set(keyledsd_SRCS ${keyledsd_SRCS} ${moc_sources})
ENDIF()

find_package(Threads REQUIRED)

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

find_package(X11)
set(keyledsd_DEPS ${keyledsd_DEPS} ${X11_LIBRARIES} ${X11_Xinput_LIB})

set(keyledsd_DEPS "${keyledsd_DEPS}"
    ${CMAKE_THREAD_LIBS_INIT} ${LIBUDEV} ${LIBRT} ${LIBYAML}
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
