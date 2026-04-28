# lvglpp examples — board- or host-specific demos.
#
# Each example is opt-in via its own option flag and declares its
# ownership story for any HAL/display driver it pulls in.

# PLAT-01 host SDL Label demo — first runnable on-screen target.
if(LVGLPP_PLATFORM_HOST_SDL)
    add_subdirectory(host_sdl_label)
endif()

# PLAT-02 STM32H747I-DISCO. Gated on LVGLPP_PLATFORM_DISCO; requires
# the arm-none-eabi cross-toolchain. The example's own CMakeLists
# raises a FATAL_ERROR if invoked under a host toolchain.
if(LVGLPP_PLATFORM_DISCO)
    add_subdirectory(stm32h747i-disco)
endif()
