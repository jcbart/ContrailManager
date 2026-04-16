cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add vcpkg to toolchain
set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_SOURCE_DIR}/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING ""
)

include(cmake/ESMF.cmake)

# Set CMake compiler
set(CMAKE_C_COMPILER ${ESMF_CCOMPILER})
set(CMAKE_CXX_COMPILER ${ESMF_CXXCOMPILER})

# Begin project
project(ContrailManager LANGUAGES CXX)

# Build options

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

option(WITH_COCIP "Build with CoCiP" ON)
option(BUILD_TOOLS "Build tools" ON)

message(STATUS "--------------------------------------------")
message(STATUS "Contrail Manager")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build with CoCiP: ${WITH_COCIP}")
message(STATUS "Build tools: ${BUILD_TOOLS}")
message(STATUS "--------------------------------------------")

# Add compile options interface target
include(cmake/compile_options.cmake)

# Find libraries
find_package(OpenMP REQUIRED)
find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

if(WITH_COCIP)
    set(COCIP_RELATIVE_PATH submodules/CoCiPPlusPlus)
    set(COCIP_BUILD_EXECUTABLE OFF CACHE BOOL "Build a standalone executable" FORCE)
    set(COCIP_USE_EXTERNAL_LIBRARIES ON CACHE BOOL "Use external libraries instead of bundled" FORCE)
    add_subdirectory(${COCIP_RELATIVE_PATH} EXCLUDE_FROM_ALL)
    add_compile_definitions(WITH_COCIP)
endif()

add_subdirectory(src)

if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

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

# ------------------------ Library installation ------------------------

# Set install directory if user did not do -DCMAKE_PREFIX_PATH=
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path prefix" FORCE)
endif()

include(GNUInstallDirs)

# Set install targets
set(INSTALL_TARGETS
    contrailmanager
    compile_options # Necessary since contrailmanager links to it
)

if(WITH_COCIP)
    list(APPEND INSTALL_TARGETS cocip build_config)
    list(APPEND INSTALL_INPUT_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${COCIP_RELATIVE_PATH}/CoCiP-params.yaml"
    )
endif()

if(BUILD_TOOLS)
    list(APPEND INSTALL_TARGETS bintocsv bintopq)
endif()

set(INSTALL_INPUT_FILES ${CMAKE_CURRENT_BINARY_DIR}/CM-config.yaml)

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