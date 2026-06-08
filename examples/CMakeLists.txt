# lvglpp examples — board- or host-specific demos.
#
# Each example is opt-in via its own option flag and declares its
# ownership story for any HAL/display driver it pulls in.

# DEMO-05 disco-demo app module — platform-INDEPENDENT composite widgets +
# icon-asset pipeline (lvglpp::app_disco_demo). Deliberately NOT gated on
# LVGLPP_PLATFORM_HOST_SDL / LVGLPP_PLATFORM_DISCO so it builds on a plain
# host configure. Guarded only on LVGLPP_HOST_BUILD because it links the
# lvglpp::core/ui/widgets module libraries, which themselves only exist in
# host configures (cross builds link those on-target in later phases).
if(LVGLPP_HOST_BUILD)
    add_subdirectory(apps/disco-demo)
endif()

# PLAT-01 host SDL Label demo — first runnable on-screen target.
if(LVGLPP_PLATFORM_HOST_SDL)
    add_subdirectory(host_sdl_label)
endif()

# DEMO-06 host-SDL disco simulator — wires DiscoController into an SDL
# window + playit stdin. Same gate as host_sdl_label.
if(LVGLPP_PLATFORM_HOST_SDL)
    add_subdirectory(disco-sim)
endif()

# PLAT-02 STM32H747I-DISCO. Gated on LVGLPP_PLATFORM_DISCO; requires
# the arm-none-eabi cross-toolchain. The example's own CMakeLists
# raises a FATAL_ERROR if invoked under a host toolchain.
if(LVGLPP_PLATFORM_DISCO)
    add_subdirectory(stm32h747i-disco)
endif()
