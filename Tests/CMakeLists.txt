include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

add_executable(NanoOcp1Tests
    Ocp1DataTypesTest.cpp
    VariantTest.cpp
    Ocp1MessageTest.cpp
    ObjectDefinitionsTest.cpp
)

target_link_libraries(NanoOcp1Tests PRIVATE
    NanoOcp1
    GTest::gtest_main
)

target_compile_features(NanoOcp1Tests PRIVATE cxx_std_17)

include(GoogleTest)
gtest_discover_tests(NanoOcp1Tests)
