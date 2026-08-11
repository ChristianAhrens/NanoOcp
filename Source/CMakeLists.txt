configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/NanoOcp1Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/NanoOcp1Version.h
    @ONLY
)

add_library(NanoOcp1 STATIC
    NanoOcp1.cpp
    NanoOcp1.h
    AmpController.cpp
    AmpController.h
    SoundscapeController.cpp
    SoundscapeController.h
    Ocp1Connection.cpp
    Ocp1Connection.h
    Ocp1Controller.cpp
    Ocp1Controller.h
    Ocp1ConnectionServer.cpp
    Ocp1ConnectionServer.h
    Ocp1DataTypes.cpp
    Ocp1DataTypes.h
    Ocp1DS100ObjectDefinitions.h
    Ocp1Message.cpp
    Ocp1Message.h
    Ocp1ObjectDefinitions.h
    Variant.cpp
    Variant.h
    internal/NanoSocket.cpp
    internal/NanoSocket.h
    internal/NanoThread.h
    internal/NanoTimer.cpp
    internal/NanoTimer.h
    internal/NanoAsyncDispatcher.cpp
    internal/NanoAsyncDispatcher.h
    ${CMAKE_CURRENT_BINARY_DIR}/generated/NanoOcp1Version.h
)

target_include_directories(NanoOcp1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_compile_features(NanoOcp1 PUBLIC cxx_std_17)

# Mostly relevant if NanoOcp1 is ever built SHARED instead of STATIC; harmless
# on a static lib. Ties the on-disk artifact's version metadata to the same
# single source of truth as NanoOcp1Version.h (the root project() call).
set_target_properties(NanoOcp1 PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(WIN32)
    target_link_libraries(NanoOcp1 PUBLIC ws2_32)
endif()

# ── Install (also used by the release packaging CI workflow) ──────────────────
# Mirrors the flat, Source/-as-include-root layout consumers already build
# against: headers land at include/*.h and include/internal/*.h, unprefixed,
# exactly like `#include "Ocp1Connection.h"` resolves against Source/ today.
install(TARGETS NanoOcp1
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/NanoOcp1Version.h
    DESTINATION include
)
