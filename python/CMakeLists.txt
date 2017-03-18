cmake_minimum_required (VERSION 2.8)
project (python-keyleds C)

##############################################################################
# Sources

# List of sources
set(keyleds_SRCS
    keyleds.c
)

##############################################################################
# Dependencies

# Build python bindings unless disabled or dependincies are missing
find_package(PythonLibs)
IF (NOT PYTHONLIBS_FOUND)
    MESSAGE(WARNING "Python libs not found, not building python bindings")
    RETURN()
ENDIF()
set(keyleds_INCLUDES ${keyleds_INCLUDES} ${PYTHON_INCLUDE_DIRS})
set(keyleds_DEPS ${keyleds_DEPS} ${PYTHON_LIBRARIES})

find_program(CYTHON cython3 cython)
IF (NOT CYTHON)
    MESSAGE(WARNING "cython is missing, not building python bindings")
    RETURN()
ENDIF()


##############################################################################
# Commands

add_custom_command(OUTPUT keyleds.c
                   COMMAND ${CYTHON}
                   ARGS $(C_INCLUDES) -o keyleds.c ${CMAKE_CURRENT_SOURCE_DIR}/keyleds.pyx
                   MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/keyleds.pyx
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/keyleds.pxd
                   COMMENT "Generating C source keyleds.c")

##############################################################################
# Targets

# Python bindings
add_library(keyleds MODULE ${keyleds_SRCS})
set_target_properties(keyleds PROPERTIES PREFIX "")
target_include_directories(keyleds PRIVATE ${keyleds_INCLUDES})
target_link_libraries(keyleds libkeyleds ${keyleds_DEPS})
