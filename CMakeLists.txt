cmake_minimum_required(VERSION 3.22)

project(fuchsia)

set(CMAKE_CXX_STANDARD 20)

set(DEFAULT_CMAKE_BUILD_TYPE "Debug")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to ${DEFAULT_CMAKE_BUILD_TYPE}")
    set(CMAKE_BUILD_TYPE "${DEFAULT_CMAKE_BUILD_TYPE}" CACHE STRING "Build type (default ${DEFAULT_CMAKE_BUILD_TYPE})" FORCE)
endif ()

# Add conan generated cmake files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_BINARY_DIR})

find_package(spdlog REQUIRED)
find_package(llhttp REQUIRED)

# A simple way to add stdexec as dependency
#include_directories(thirdparty/stdexec/include)

file(GLOB_RECURSE FUCHSIA_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    )

add_library(fuchsia SHARED ${FUCHSIA_SRCS}) # STATIC won't compile because of a possible linker bug reported in stdexec: https://github.com/NVIDIA/stdexec/pull/993
add_library(fuchsia::fuchsia ALIAS fuchsia)

target_include_directories(fuchsia PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stdexec/include> # add stdexec as dependency
    $<INSTALL_INTERFACE:include>)
target_link_libraries(fuchsia PUBLIC spdlog::spdlog)
target_link_libraries(fuchsia PUBLIC llhttp::llhttp)

if (PROJECT_IS_TOP_LEVEL)
    option(FUCHSIA_BUILD_EXAMPLES "Build examples" ON)
else()
    option(FUCHSIA_BUILD_EXAMPLES "Build examples" OFF)
endif()

if (FUCHSIA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(FUCHSIA_BUILD_TESTS "Build tests" ON)

if (FUCHSIA_BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()
