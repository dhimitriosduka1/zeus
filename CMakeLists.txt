cmake_minimum_required(VERSION 3.9...3.20)

set(FEATURES "" CACHE STRING "Features to enable")
set(EXTRA_DEFINES "" CACHE STRING "Additional defines")

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Prevent in source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    if(NOT DEFINED WITH_IN_SOURCE_BUILD)
        message(FATAL_ERROR "CMake generation for Lightwave is not allowed within the source directory! Define WITH_IN_SOURCE_BUILD if absolutely necessary!")
    endif()
endif()

# Omit superfluous "Up-to-date" messages.
if(NOT DEFINED CMAKE_INSTALL_MESSAGE)
    set(CMAKE_INSTALL_MESSAGE "LAZY")
endif()

project(lightwave LANGUAGES CXX)

# For whatever reason Lightwave might be used as a subproject...
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(LW_SUBPROJECT OFF)
else()
    set(LW_SUBPROJECT ON)
endif()

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

# Warn students if some obscure systems are used
if(MINGW)
    message(WARNING "Lightwave with MinGW is not supported. Please use Clang or MSVC on Windows")
endif()

if(MSYS)
    message(WARNING "Lightwave with MSYS is not supported. Please use Clang or MSVC on Windows")
endif()

if(CYGWIN)
    message(WARNING "Lightwave with CygWin is not supported. Please use Clang or MSVC on Windows")
endif()

# Setup stuff
include(cmake/SetupWarnings.cmake)
include(cmake/SetupFlags.cmake)
include(cmake/SetupLTO.cmake)
include(cmake/SetupSanitizers.cmake)
include(cmake/SetupChecks.cmake)

# Setup dependencies
add_subdirectory(deps/miniz)
include_directories(deps/tinyexr)
include_directories(deps/stb)
include_directories(deps/tinyformat)
find_package(Threads REQUIRED)

function(add_extra_options TARGET)
    add_warnings(${TARGET}) # Defined in cmake/SetupWarnings.cmake
    add_fastmath(${TARGET}) # Defined in cmake/SetupFlags.cmake
    add_lto(${TARGET}) # Defined in cmake/SetupLTO.cmake
    add_checks(${TARGET}) # Defined in cmake/SetupChecks.cmake
    add_sanitizers(${TARGET}) # Defined in cmake/SetupSanitizers.cmake
endfunction()

# Extract all source files inside the directory src/
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/**/*.cpp" "src/**/*.hpp" "include/*.hpp")

# ######################
set(MY_TARGET_NAME zeus)
# ######################

find_package(OpenImageDenoise)

add_executable(${MY_TARGET_NAME} ${SOURCE_FILES})
target_compile_definitions(${MY_TARGET_NAME} PUBLIC "${FEATURES};${EXTRA_DEFINES}")
target_link_libraries(${MY_TARGET_NAME} PRIVATE miniz Threads::Threads)
target_include_directories(${MY_TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${MY_TARGET_NAME} PRIVATE ${stb_SOURCE_DIR} ${tinyexr_SOURCE_DIR} PUBLIC ${tinyformat_SOURCE_DIR})
target_compile_definitions(${MY_TARGET_NAME} PUBLIC "$<$<CONFIG:Debug>:LW_DEBUG>")
target_compile_features(${MY_TARGET_NAME} PUBLIC cxx_std_20)

# Ensure the target is directly in the build directory. This is the default on Linux/Mac, but not Windows.
# Pro Tip: You can commit this out to have Debug and Release builds at the same time on Windows. This will prevent the default parameters to work though.
set_target_properties(${MY_TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}"
)

if(OpenImageDenoise_FOUND)
    target_link_libraries(${MY_TARGET_NAME} PRIVATE OpenImageDenoise)
    target_compile_definitions(${MY_TARGET_NAME} PRIVATE "LW_WITH_OIDN")
else()
    message(WARNING "No denoising support")
endif()

if(WIN32)
  target_link_libraries(${MY_TARGET_NAME} PRIVATE wsock32 ws2_32)
endif()

add_extra_options(${MY_TARGET_NAME})

add_subdirectory(blender_exporter)
