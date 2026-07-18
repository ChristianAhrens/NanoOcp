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
)

target_include_directories(NanoOcp1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(NanoOcp1 PUBLIC cxx_std_17)

if(WIN32)
    target_link_libraries(NanoOcp1 PUBLIC ws2_32)
endif()
