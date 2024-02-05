if(CMAKE_VERSION VERSION_GREATER 3.6)
    # Add clang-tidy if available
    option(LW_USE_CLANG_TIDY "Use Clang-Tidy" OFF)
    if(LW_USE_CLANG_TIDY)
        find_program(
            CLANG_TIDY_EXECUTABLE
            NAMES "clang-tidy"
            DOC "Path to clang-tidy executable"
        )

        set(CLANG_TIDY_STYLE -*)
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},clang-analyzer-cplusplus*)
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},performance-*)
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},portability-*)
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},modernize-*)
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},cppcoreguidelines-*)
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},bugprone-*)
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},concurrency-*)

        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},-cppcoreguidelines-owning-memory)                     # smart pointers are nice, but deps not always use them
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},-modernize-use-trailing-return-type)                  # never argue about style
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},-cppcoreguidelines-pro-bounds-pointer-arithmetic)     # we are close to hardware and need pointer magic
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},-cppcoreguidelines-pro-type-reinterpret-cast)         # reinterpret_cast is necessary
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},-cppcoreguidelines-pro-type-const-cast)               # const_cast is useful
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},-cppcoreguidelines-pro-bounds-array-to-pointer-decay) # this is just annoying
        set(CLANG_TIDY_STYLE ${CLANG_TIDY_STYLE},-cppcoreguidelines-avoid-magic-numbers)               # we take care of magic numbers in our way

        if(CLANG_TIDY_EXECUTABLE)
            message(STATUS "Using Clang-Tidy ${CLANG_TIDY_EXECUTABLE}")
            set(CLANG_TIDY_ARGS 
                "--checks=${CLANG_TIDY_STYLE}"
                "--quiet")
        endif()
    endif()
endif()

if(CMAKE_VERSION VERSION_GREATER 3.8)
    # Add cpplint if available
    option(LW_USE_CPPLINT "Do style check with cpplint." OFF)
    if(LW_USE_CPPLINT)
        find_program(
            CPPLINT_EXECUTABLE
            NAMES "cpplint"
            DOC "Path to cpplint executable"
        )

        set(LW_CPPLINT_STYLE)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-whitespace/braces,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-whitespace/tab,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-whitespace/line_length,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-whitespace/comments,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-whitespace/indent,)
        #set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-build/include_order,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-build/namespaces,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-build/include_what_you_use,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-build/include,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-legal/copyright,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-readability/namespace,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-readability/todo,)
        set(LW_CPPLINT_STYLE ${LW_CPPLINT_STYLE}-runtime/references,)

        if(CPPLINT_EXECUTABLE)
            message(STATUS "Using cpplint ${CPPLINT_EXECUTABLE}")
            set(CPPLINT_ARGS 
                "--filter=${LW_CPPLINT_STYLE}"
                "--counting=detailed"
                "--extensions=cpp,h,inl"
                "--headers=h,inl"
                "--quiet")
        endif()
    endif()
endif()

if(CMAKE_VERSION VERSION_GREATER 3.10)
    # Add cppcheck if available
    option(LW_USE_CPPCHECK "Do checks with cppcheck." OFF)
    if(LW_USE_CPPCHECK)
        find_program(
            CPPCHECK_EXECUTABLE
            NAMES "cppcheck"
            DOC "Path to cppcheck executable"
        )

        if(CPPCHECK_EXECUTABLE)
            message(STATUS "Using cppcheck ${CPPCHECK_EXECUTABLE}")
            if(WIN32)
                set(ARCH_ARGS "-DWIN32" "-D_MSC_VER")
            else()
                set(ARCH_ARGS "-Dlinux" "-D__GNUC__")
            endif()
            set(CPPCHECK_ARGS ${ARCH_ARGS}
                "--quiet"
                "--enable=warning,style,performance,portability"
                "--suppress=preprocessorErrorDirective"
                "--library=std"
                "--std=c++17")
        endif()
    endif()
endif()

function(add_checks TARGET)
    if(LW_USE_CLANG_TIDY AND CLANG_TIDY_EXECUTABLE)
        set_property(TARGET ${TARGET} PROPERTY CXX_CLANG_TIDY ${CLANG_TIDY_EXECUTABLE} ${CLANG_TIDY_ARGS})
    endif()
    if(LW_USE_CPPLINT AND CPPLINT_EXECUTABLE)
        set_property(TARGET ${TARGET} PROPERTY CXX_CPPLINT ${CPPLINT_EXECUTABLE} ${CPPLINT_ARGS})
    endif()
    if(LW_USE_CPPCHECK AND CPPCHECK_EXECUTABLE)
        set_property(TARGET ${TARGET} PROPERTY CXX_CPPCHECK ${CPPCHECK_EXECUTABLE} ${CPPCHECK_ARGS})
    endif()
endfunction()