# lvglpp::playit — serial test driver, C++ port of rlvgl-playit.
#
# PARITY: rlvgl/playit (v0.2.0). The wire protocol is the source of
# truth — see rlvgl/playit/README.md and the top-level CLAUDE.md
# § "Runtime Command Protocol (rlvgl-playit)".
#
# As of PLAYIT-01 this is a compiled library carrying parse_command()
# and the wire-format event/command types. Future phases (PLAYIT-02
# dispatcher, PLAYIT-03+ richer commands) extend this target.

add_library(lvglpp_playit)
add_library(lvglpp::playit ALIAS lvglpp_playit)

target_sources(lvglpp_playit
    PRIVATE
        src/dispatcher.cpp
        src/obj_dispatcher.cpp   # LVGLPP-WRAP-0N: lv_obj-tree re-target
        src/executor.cpp
        src/format.cpp
        src/parser.cpp
)

# StdioTransport / TcpServerTransport are host-only — they pull in
# POSIX <fcntl.h> / <sys/socket.h>. Excluded under embedded posture
# so the freestanding subset rule in docs/std-mapping.md is
# preserved. (TCP: DEMO-07 §5.4, mirrors rlvgl playit/src/tcp.rs.)
if(NOT LVGLPP_EMBEDDED_POSTURE)
    target_sources(lvglpp_playit PRIVATE
        src/stdio_transport.cpp
        src/tcp_transport.cpp)
endif()

target_include_directories(lvglpp_playit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(lvglpp_playit
    PUBLIC
        lvglpp::core
    PRIVATE
        lvglpp_warnings
)

target_compile_features(lvglpp_playit PUBLIC cxx_std_20)

if(LVGLPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
