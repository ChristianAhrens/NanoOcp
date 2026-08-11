add_executable(NanoOcp1Demo
    main.cpp
    Terminal.h
    Ansi.h
    AppState.h
    Format.h
    FocusParams.h
    Panels.h
    Demo.h
)

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/NanoOcp1Demo.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/NanoOcp1Demo.rc
        @ONLY
    )
    target_sources(NanoOcp1Demo PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/NanoOcp1Demo.rc)
endif()

target_link_libraries(NanoOcp1Demo PRIVATE NanoOcp1)

target_compile_features(NanoOcp1Demo PRIVATE cxx_std_17)

# Always place the built executable under NanoOcp1Demo/<Config>/, e.g.
# build/NanoOcp1Demo/Release/NanoOcp1Demo. Multi-config generators (Visual Studio,
# Xcode) do this by default already; $<CONFIG> makes single-config generators
# (Unix Makefiles, Ninja — macOS/Linux) resolve to the same layout instead of
# dropping straight into build/NanoOcp1Demo/.
set_target_properties(NanoOcp1Demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/NanoOcp1Demo/$<CONFIG>"
)

install(TARGETS NanoOcp1Demo RUNTIME DESTINATION bin)
