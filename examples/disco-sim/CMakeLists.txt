# examples/disco-sim — runnable host-SDL simulator for the disco demo.
#
# PARITY: rlvgl examples/disco-sim (the rlvgl-disco-sim host binary).
# Wires DEMO-06 DiscoController into an SDL2 window + playit stdin driver.
#
# Gated on LVGLPP_PLATFORM_HOST_SDL — added only when that option is
# enabled by the parent examples/CMakeLists.txt (same gate as
# examples/host_sdl_label/).
#
# docs/disco-demo/06-controller-and-host-target.md (DEMO-06) §6.

add_executable(lvglpp_example_disco_sim main.cpp)

target_link_libraries(lvglpp_example_disco_sim
    PRIVATE
        lvglpp::app_disco_demo
        lvglpp::platform
        lvglpp::playit
        lvglpp_warnings
)
