add_library(contrailmanager STATIC
    FlightContainer/general.cpp
    FlightContainer/parquet.cpp
    map/functions.cpp
    segments/SegmentCaCE.cpp
    segments/SegmentCoCiP.cpp
    CMConfig.cpp
    CMLog.cpp
    ContrailManager.cpp
    Domain.cpp
    extern.cpp
    SegmentContainer.cpp
)

target_link_libraries(contrailmanager
    PUBLIC
        ${ESMF_CXXESMFLINKLIBS}
        ${ESMF_CXXESMFLINKPATHS}
    PRIVATE
        compile_options
        OpenMP::OpenMP_CXX
        Arrow::arrow_static
        Parquet::parquet_static
        cereal::cereal
        yaml-cpp::yaml-cpp
)

target_include_directories(contrailmanager
    PUBLIC
        #$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        #$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

if(WITH_COCIP)
    target_link_libraries(contrailmanager PRIVATE cocip)
    target_compile_definitions(contrailmanager PUBLIC WITH_COCIP)
endif()