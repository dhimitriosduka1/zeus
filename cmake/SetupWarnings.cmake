include(CheckCXXCompilerFlag)

option(LW_DISABLE_WARNINGS "Disable additional warnings [Not recommended]" OFF)

if(NOT LW_DISABLE_WARNINGS)
    # Add warnings by default
    # But keep warnings about unused parameters & functions at bay,
    # as the nature of the framework is to be implemented step by step
    set(WARNING_FLAGS -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-function -Wno-psabi)
    check_cxx_compiler_flag("${WARNING_FLAGS}" COMPILER_SUPPORTS_WARNINGS_GNU)

    if(NOT COMPILER_SUPPORTS_WARNINGS_GNU)
        set(WARNING_FLAGS /W4 /wd4100 /wd4505)
        check_cxx_compiler_flag("${WARNING_FLAGS}" COMPILER_SUPPORTS_WARNINGS_MSVC)

        if(NOT COMPILER_SUPPORTS_WARNINGS_MSVC)
            set(WARNING_FLAGS)
        endif()
    endif()
endif()

function(add_warnings TARGET)
    if(NOT LW_DISABLE_WARNINGS)
        target_compile_options(${TARGET} PRIVATE ${WARNING_FLAGS})
    endif()
endfunction()
