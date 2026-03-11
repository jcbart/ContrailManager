cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{ESMFMKFILE})
    message(FATAL_ERROR "Environment variable ESMFMKFILE was not set.")
endif()

# Build options

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

option(WITH_COCIP "Build with CoCiP" ON)

message(STATUS "--------------------------------------------")
message(STATUS "Contrail Manager")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build with CoCiP: ${WITH_COCIP}")
message(STATUS "--------------------------------------------")

set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Function to parse esmf.mk
function(get_esmf_var VAR_NAME OUTPUT_VAR)
    execute_process(
        COMMAND grep "^${VAR_NAME}=" $ENV{ESMFMKFILE}
        COMMAND cut -d= -f2-
        OUTPUT_VARIABLE RESULT
    )
    string(STRIP "${RESULT}" STRIPPED_RESULT)
    set(${OUTPUT_VAR} "${STRIPPED_RESULT}" PARENT_SCOPE)
endfunction()

# Get the compiler and flags
get_esmf_var("ESMF_CXXCOMPILER" ESMF_CXXCOMPILER)
get_esmf_var("ESMF_CCOMPILER" ESMF_CCOMPILER)
get_esmf_var("ESMF_CXXCOMPILEOPTS" ESMF_CXXCOMPILEOPTS)
get_esmf_var("ESMF_CXXCOMPILECPPFLAGS" ESMF_CXXCOMPILECPPFLAGS)
get_esmf_var("ESMF_CXXLINKLIBS" ESMF_CXXLINKLIBS)

# Remove optimisation flags from ESMF_CXXCOMPILEOPTS
string(REGEX REPLACE
    "-O[0-3|g|s|z|fast]*|-std=c\\+\\+[0-9][0-9]"
    ""
    ESMF_CXXCOMPILEOPTS_CLEAN
    "${ESMF_CXXCOMPILEOPTS}")
separate_arguments(ESMF_CXXCOMPILEOPTS_LIST NATIVE_COMMAND "${ESMF_CXXCOMPILEOPTS_CLEAN}")

# Set CMake compiler
set(CMAKE_C_COMPILER ${ESMF_CCOMPILER})
set(CMAKE_CXX_COMPILER ${ESMF_CXXCOMPILER})

# Begin project
project(ContrailManager LANGUAGES CXX)

# Source files
set(SOURCES
    "src/CMLog.cpp"
    "src/ContrailManager.cpp"
    "src/Domain.cpp"
    "src/extern.cpp"
    "src/mapUtils.cpp"
    "src/SegmentCoCiP.cpp"
)

# Target definition
add_library(contrailmanager STATIC ${SOURCES})

# Submodules

if(NOT TARGET yaml-cpp)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    add_subdirectory(submodules/yaml-cpp EXCLUDE_FROM_ALL)
endif()

if(WITH_COCIP)
    set(COCIP_BUILD_EXECUTABLE OFF CACHE BOOL "Build a standalone executable" FORCE)
    set(COCIP_USE_EXTERNAL_LIBRARIES ON CACHE BOOL "Use external libraries instead of bundled" FORCE)
    add_subdirectory(submodules/CoCiPPlusPlus EXCLUDE_FROM_ALL)
    add_compile_definitions(WITH_COCIP)
    target_link_libraries(contrailmanager PRIVATE cocip)
    # Force library to be bundled inside
    target_sources(contrailmanager PRIVATE $<TARGET_OBJECTS:cocip>)
endif()

# Apply ESMF options and flags
target_compile_options(contrailmanager PRIVATE ${ESMF_CXXCOMPILEOPTS_LIST})
target_compile_definitions(contrailmanager PRIVATE ${ESMF_CXXCOMPILECPPFLAGS})

target_link_libraries(contrailmanager PRIVATE yaml-cpp)
# Force library to be bundled inside
target_sources(contrailmanager PRIVATE $<TARGET_OBJECTS:yaml-cpp>)

target_link_libraries(contrailmanager PUBLIC ${ESMF_CXXLINKLIBS})