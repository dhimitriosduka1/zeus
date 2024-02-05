include(CheckCXXCompilerFlag)

option(LW_DISABLE_FASTMATH "Disable math optimizations [Not recommended]" OFF)

if(NOT LW_DISABLE_FASTMATH)
	if((CMAKE_CXX_COMPILER_ID MATCHES "MSVC") OR (CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC"))
		set(FF_FLAGS /fp:fast)
    elseif((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
		set(FF_FLAGS -funsafe-math-optimizations -fno-rounding-math -fno-math-errno)
	endif()
endif()

if((CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
	set(CMAKE_CXX_FLAGS_DEBUG "-g -Og" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE STRING "" FORCE)
endif()

function(add_fastmath TARGET)
    target_compile_options(${TARGET} PRIVATE ${FF_FLAGS})
endfunction()
