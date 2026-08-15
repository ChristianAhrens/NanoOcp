cmake_minimum_required(VERSION 3.15)

project(NanoOcp1
    VERSION 0.6.3
    LANGUAGES CXX
)
# The project version continues the numbering previously tracked in
# NanoOcp1Demo.jucer's version="..." attribute (last bumped there to 0.5.1,
# never tagged) -- this is now the single source of truth for both the
# NanoOcp1 library and NanoOcp1Demo, propagated into build artifacts via the
# generated NanoOcp1Version.h (see Source/CMakeLists.txt) and, on Windows,
# NanoOcp1Demo's .exe VERSIONINFO resource.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Single-config generators (Unix Makefiles, Ninja — macOS/Linux) have no build type
# unless one is specified, which leaves $<CONFIG> empty and drops the per-config
# output subdirectory that multi-config generators (Visual Studio, Xcode) always
# have. Default to Debug so NanoOcp1Demo's output layout (see its CMakeLists.txt)
# is consistent across platforms even when -DCMAKE_BUILD_TYPE isn't passed.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

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
