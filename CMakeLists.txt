cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED ENV{ESMFMKFILE})
    message(FATAL_ERROR "Environment variable ESMFMKFILE was not set.")
endif()

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
string(REGEX REPLACE "-O[0-3|g|s|z|fast]*" "" ESMF_CXXCOMPILEOPTS_CLEAN "${ESMF_CXXCOMPILEOPTS}")
separate_arguments(ESMF_CXXCOMPILEOPTS_LIST NATIVE_COMMAND "${ESMF_CXXCOMPILEOPTS_CLEAN}")

# Set CMake compiler
set(CMAKE_C_COMPILER ${ESMF_CCOMPILER})
set(CMAKE_CXX_COMPILER ${ESMF_CXXCOMPILER})

# Begin project
project(ContrailManager LANGUAGES CXX)

# Submodules
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
add_subdirectory(submodules/yaml-cpp EXCLUDE_FROM_ALL)

# Source files
set(SOURCES
    "src/BasicPlume.cpp"
    "src/ContrailManager.cpp"
    "src/domain.cpp"
    "src/extern.cpp"
    "src/mapUtils.cpp"
    "src/mathUtils.cpp"
    "src/projection.cpp"
    "src/thermo.cpp"
    "src/timekeeping.cpp"
)

# Target definition
add_library(contrailmanager STATIC ${SOURCES})

# Apply ESMF options and flags
target_compile_options(contrailmanager PRIVATE ${ESMF_CXXCOMPILEOPTS_LIST} -O2)
target_compile_definitions(contrailmanager PRIVATE ${ESMF_CXXCOMPILECPPFLAGS})

target_link_libraries(contrailmanager PRIVATE yaml-cpp ${ESMF_CXXLINKLIBS})