cmake_minimum_required(VERSION 3.15)

project(NanoOcp1
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NANOOCP1_BUILD_DEMO "Build the NanoOcp1Demo CLI application" ON)
option(NANOOCP1_BUILD_TESTS "Build the NanoOcp1Tests unit test suite" ON)

add_subdirectory(Source)

if(NANOOCP1_BUILD_DEMO)
    add_subdirectory(NanoOcp1Demo)
endif()

if(NANOOCP1_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
