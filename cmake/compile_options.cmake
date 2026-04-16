add_library(compile_options INTERFACE)

target_compile_features(compile_options INTERFACE cxx_std_20)

target_compile_options(compile_options INTERFACE
    ${ESMF_CXXCOMPILEOPTS_LIST}
)
target_compile_definitions(compile_options INTERFACE
    ${ESMF_CXXCOMPILECPPFLAGS}
)

# Per-config optimisation flags
target_compile_options(compile_options INTERFACE
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Debug>:-g;-O0>
)