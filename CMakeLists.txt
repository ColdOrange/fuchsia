cmake_minimum_required(VERSION 3.22)

project(fuchsia)

set(CMAKE_CXX_STANDARD 20)

# Add conan generated cmake files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_BINARY_DIR})

find_package(spdlog REQUIRED)

# A simple way to add stdexec as dependency
include_directories(thirdparty/stdexec/include)

file(GLOB_RECURSE FUCHSIA_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    )

add_library(fuchsia STATIC ${FUCHSIA_SRCS})
add_library(fuchsia::fuchsia ALIAS fuchsia)

target_include_directories(fuchsia PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(fuchsia PUBLIC spdlog::spdlog)

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
