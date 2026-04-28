# examples/host_sdl_label — minimal demo: a Label rendering in an
# SDL2 window. Quits on window close, Escape, or Q.
#
# Gated on LVGLPP_PLATFORM_HOST_SDL — added only when that option is
# enabled by the parent examples/CMakeLists.txt.

add_executable(lvglpp_example_host_sdl_label main.cpp)

target_link_libraries(lvglpp_example_host_sdl_label
    PRIVATE
        lvglpp::widgets
        lvglpp::platform
        lvglpp::playit
        lvglpp_warnings
)
