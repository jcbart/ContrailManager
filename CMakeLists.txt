cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{ESMFMKFILE})
    message(FATAL_ERROR "Environment variable ESMFMKFILE was not set.")
endif()

# Add vcpkg to toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

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
get_esmf_var("ESMF_CXXESMFLINKLIBS" ESMF_CXXESMFLINKLIBS)
get_esmf_var("ESMF_CXXESMFLINKPATHS" ESMF_CXXESMFLINKPATHS)

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

# Find libraries
find_package(OpenMP REQUIRED)
find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

# Source files
set(SOURCES
    "src/CMConfig.cpp"
    "src/CMLog.cpp"
    "src/ContrailManager.cpp"
    "src/Domain.cpp"
    "src/extern.cpp"
    "src/FlightContainer_general.cpp"
    "src/FlightContainer_parquet.cpp"
    "src/mapFunctions.cpp"
    "src/SegmentCoCiP.cpp"
)

# Target definitions
add_library(contrailmanager STATIC ${SOURCES})
add_library(readbin STATIC "src/readbin.cpp")
add_executable(bintocsv "src/bintocsv.cpp")
add_executable(bintopq "src/bintopq.cpp")

# Submodules

if(WITH_COCIP)
    set(COCIP_RELATIVE_PATH submodules/CoCiPPlusPlus)
    set(COCIP_BUILD_EXECUTABLE OFF CACHE BOOL "Build a standalone executable" FORCE)
    set(COCIP_USE_EXTERNAL_LIBRARIES ON CACHE BOOL "Use external libraries instead of bundled" FORCE)
    add_subdirectory(${COCIP_RELATIVE_PATH} EXCLUDE_FROM_ALL)
    add_compile_definitions(WITH_COCIP)
    target_link_libraries(contrailmanager PRIVATE cocip)
    target_link_libraries(readbin PUBLIC cocip)
endif()

# Apply ESMF options and flags

target_compile_options(contrailmanager PRIVATE ${ESMF_CXXCOMPILEOPTS_LIST})
target_compile_definitions(contrailmanager PRIVATE ${ESMF_CXXCOMPILECPPFLAGS})

target_compile_options(readbin PUBLIC ${ESMF_CXXCOMPILEOPTS_LIST})
target_compile_definitions(readbin PUBLIC ${ESMF_CXXCOMPILECPPFLAGS})

target_include_directories(contrailmanager
    PUBLIC
        #$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        #$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(contrailmanager
    PRIVATE
        OpenMP::OpenMP_CXX
        Arrow::arrow_static
        Parquet::parquet_static
        cereal::cereal
        yaml-cpp::yaml-cpp
)

target_link_libraries(readbin
    PRIVATE
        contrailmanager
        cereal::cereal
    # Make public libraries which are needed by any executable which depends on readbin
    PUBLIC
        Arrow::arrow_static
        Parquet::parquet_static
)

target_link_libraries(contrailmanager PUBLIC ${ESMF_CXXESMFLINKLIBS} ${ESMF_CXXESMFLINKPATHS})
target_link_libraries(readbin PUBLIC ${ESMF_CXXESMFLINKLIBS} ${ESMF_CXXESMFLINKPATHS})

target_link_libraries(bintocsv PRIVATE readbin)
target_link_libraries(bintopq PRIVATE readbin)

# Copy default input files if they do not already exist in build directory
set(CM_INPUT_FILES "CM-config.yaml")

foreach(FILE_NAME ${CM_INPUT_FILES})
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/inputs/${FILE_NAME}.default"
            "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}"
            COPYONLY
        )
    endif()
endforeach()

# Library installation

set(INSTALL_TARGETS
    contrailmanager
    bintocsv
    bintopq
)

set(INSTALL_INPUT_FILES ${CMAKE_CURRENT_BINARY_DIR}/CM-config.yaml)

if(WITH_COCIP)
    list(APPEND INSTALL_TARGETS cocip build_config)
    list(APPEND INSTALL_INPUT_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${COCIP_RELATIVE_PATH}/CoCiP-params.yaml"
    )
endif()

# Set install directory if user did not do -DCMAKE_PREFIX_PATH=
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path prefix" FORCE)
endif()

include(GNUInstallDirs)

install(TARGETS ${INSTALL_TARGETS}
    EXPORT contrailmanagerTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# If needed later
#install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(WITH_COCIP)
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${COCIP_RELATIVE_PATH}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

install(EXPORT contrailmanagerTargets
    NAMESPACE contrailmanager::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/contrailmanager
)

include(CMakePackageConfigHelpers)

# Tells consuming projects where to find Contrail Manager's dependencies
set(CM_DEP_DIR "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/contrailmanagerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/contrailmanagerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/contrailmanager
)

#write_basic_package_version_file(
#    ${CMAKE_CURRENT_BINARY_DIR}/contrailmanagerConfigVersion.cmake
#    VERSION "1.0.0"
#    COMPATIBILITY SameMajorVersion
#)

# Install CMake files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/contrailmanagerConfig.cmake
    #${CMAKE_CURRENT_BINARY_DIR}/contrailmanagerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/contrailmanager
)

# Install required files
install(FILES ${INSTALL_INPUT_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/contrailmanager
)