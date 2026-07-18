add_executable(NanoOcp1Demo
    main.cpp
)

target_link_libraries(NanoOcp1Demo PRIVATE NanoOcp1)

target_compile_features(NanoOcp1Demo PRIVATE cxx_std_17)
