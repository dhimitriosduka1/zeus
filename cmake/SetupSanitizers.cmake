if(${CMAKE_VERSION} VERSION_LESS "3.19.0") 
    message("To enable sanitizers, please consider switching to CMake 3.19.0+")

    function(add_sanitizers TARGET)
        # noop
    endfunction()

    return()
endif()

include(CheckCompilerFlag)

# ###################################
# Add address sanitizer
option(LW_USE_ADDRESS_SANITIZER "Use Address Sanitizer" OFF)

if(LW_USE_ADDRESS_SANITIZER)
    if(MSVC)
        set(LW_ADDRESS_SANITIZER_FLAGS "/fsanitize=address")
        check_compiler_flag(CXX "${LW_ADDRESS_SANITIZER_FLAGS}" HAS_ADDRESS_SANITIZER)
    else()
        set(LW_ADDRESS_SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_REQUIRED_LIBRARIES "asan")
        check_compiler_flag(CXX "${LW_ADDRESS_SANITIZER_FLAGS}" HAS_ADDRESS_SANITIZER)
    endif()

    if(HAS_ADDRESS_SANITIZER)
        set(LW_HAS_ADDRESS_SANITIZER ON)
        message(STATUS "Using address sanitizer")
    else()
        message(WARNING "Requested address sanitizer not available")
    endif()
endif()

# ###################################
# Add memory sanitizer
if(NOT MSVC)
    option(LW_USE_MEMORY_SANITIZER "Use Memory Sanitizer" OFF)
endif()

if(LW_USE_MEMORY_SANITIZER)
    set(LW_MEMORY_SANITIZER_FLAGS "-fsanitize=memory -fno-omit-frame-pointer")
    check_compiler_flag(CXX "${LW_MEMORY_SANITIZER_FLAGS}" HAS_MEMORY_SANITIZER)

    if(HAS_MEMORY_SANITIZER)
        set(LW_HAS_MEMORY_SANITIZER ON)
        message(STATUS "Using memory sanitizer")
    else()
        message(WARNING "Requested memory sanitizer not available")
    endif()
endif()

# ###################################
# Add leak sanitizer
if(NOT MSVC)
    option(LW_USE_LEAK_SANITIZER "Use Leak Sanitizer" OFF)
endif()

if(LW_USE_LEAK_SANITIZER)
    set(LW_LEAK_SANITIZER_FLAGS "-fsanitize=leak")
    check_compiler_flag(CXX "${LW_LEAK_SANITIZER_FLAGS}" HAS_LEAK_SANITIZER)
        
    if(HAS_LEAK_SANITIZER)
        set(LW_HAS_LEAK_SANITIZER ON)
        message(STATUS "Using leak sanitizer")
    else()
        message(WARNING "Requested leak sanitizer not available")
    endif()
endif()

# ###################################
# Add thread sanitizer
if(NOT MSVC)
    option(LW_USE_THREAD_SANITIZER "Use Thread Sanitizer" OFF)
endif()

if(LW_USE_THREAD_SANITIZER)
    set(LW_THREAD_SANITIZER_FLAGS "-fsanitize=thread")
    set(CMAKE_REQUIRED_LIBRARIES "tsan")
    check_compiler_flag(CXX "${LW_THREAD_SANITIZER_FLAGS}" HAS_THREAD_SANITIZER)

    if(HAS_THREAD_SANITIZER)
        set(LW_HAS_THREAD_SANITIZER ON)
        message(STATUS "Using thread sanitizer")
    else()
        message(WARNING "Requested thread sanitizer not available")
    endif()
endif()

# ###################################
# Add undefined behavior sanitizer
if(NOT MSVC)
    option(LW_USE_UNDEFINED_BEHAVIOR_SANITIZER "Use Undefined Behavior Sanitizer" OFF)
endif()

if(LW_USE_UNDEFINED_BEHAVIOR_SANITIZER)
    set(LW_UNDEFINED_BEHAVIOR_SANITIZER_FLAGS "-fsanitize=undefined")
    check_compiler_flag(CXX "${LW_UNDEFINED_BEHAVIOR_SANITIZER_FLAGS}" HAS_UNDEFINED_SANITIZER)
    
    if(HAS_UNDEFINED_SANITIZER)
        set(LW_HAS_UNDEFINED_BEHAVIOR_SANITIZER ON)
        message(STATUS "Using undefined behavior sanitizer")
    else()
        message(WARNING "Requested undefined behavior sanitizer not available")
    endif()
endif()

# ###################################
# Combine all together
function(add_sanitizer_flag TARGET FLAG)
    separate_arguments(cmd_options NATIVE_COMMAND ${${FLAG}})
    target_compile_options(${TARGET} PUBLIC "$<$<CONFIG:Debug>:${cmd_options}>" "$<$<CONFIG:RelWithDebInfo>:${cmd_options}>")
    target_link_options(${TARGET} PUBLIC "$<$<CONFIG:Debug>:${cmd_options}>" "$<$<CONFIG:RelWithDebInfo>:${cmd_options}>")
endfunction()

function(add_sanitizers TARGET)
    if(LW_USE_ADDRESS_SANITIZER AND LW_HAS_ADDRESS_SANITIZER)
        add_sanitizer_flag(${TARGET} LW_ADDRESS_SANITIZER_FLAGS)
    endif()

    if(LW_USE_MEMORY_SANITIZER AND LW_HAS_MEMORY_SANITIZER)
        add_sanitizer_flag(${TARGET} LW_MEMORY_SANITIZER_FLAGS)
    endif()

    if(LW_USE_LEAK_SANITIZER AND LW_HAS_LEAK_SANITIZER)
        add_sanitizer_flag(${TARGET} LW_LEAK_SANITIZER_FLAGS)
    endif()

    if(LW_USE_THREAD_SANITIZER AND LW_HAS_THREAD_SANITIZER)
        add_sanitizer_flag(${TARGET} LW_THREAD_SANITIZER_FLAGS)
    endif()

    if(LW_USE_UNDEFINED_BEHAVIOR_SANITIZER AND LW_HAS_UNDEFINED_BEHAVIOR_SANITIZER)
        add_sanitizer_flag(${TARGET} LW_UNDEFINED_BEHAVIOR_SANITIZER_FLAGS)
    endif()
endfunction()