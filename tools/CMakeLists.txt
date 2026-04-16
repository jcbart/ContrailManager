# readbin library

add_library(readbin STATIC
    readbin.cpp
)

target_include_directories(readbin
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(readbin
    PUBLIC
        # Public to avoid copying for executables that link to readbin
        compile_options
        Arrow::arrow_static
        Parquet::parquet_static
    PRIVATE
        contrailmanager
        cereal::cereal
)

if(WITH_COCIP)
    target_link_libraries(readbin PUBLIC cocip)
endif()

# Executables

add_executable(bintocsv
    bintocsv.cpp
)

add_executable(bintopq
    bintopq.cpp
)

target_link_libraries(bintocsv PRIVATE readbin)
target_link_libraries(bintopq PRIVATE readbin)