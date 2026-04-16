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
get_esmf_var("ESMF_CXXESMFLINKLIBS" ESMF_CXXESMFLINKLIBS)
get_esmf_var("ESMF_CXXESMFLINKPATHS" ESMF_CXXESMFLINKPATHS)

# Remove optimisation flags from ESMF_CXXCOMPILEOPTS
string(REGEX REPLACE
    "-O[0-3|g|s|z|fast]*|-std=c\\+\\+[0-9][0-9]"
    ""
    ESMF_CXXCOMPILEOPTS_CLEAN
    "${ESMF_CXXCOMPILEOPTS}")

separate_arguments(ESMF_CXXCOMPILEOPTS_LIST NATIVE_COMMAND "${ESMF_CXXCOMPILEOPTS_CLEAN}")