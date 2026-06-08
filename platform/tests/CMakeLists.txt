# lvglpp::platform per-module tests.
#
# These tests exercise the SDL-independent surface of lvglpp::platform
# (the Screen display descriptor and any future value types). They build
# on a plain host configure with no backend selected — `lvglpp::platform`
# is then an INTERFACE umbrella that still exposes `include/`, which is
# all the header-only Screen needs. Do NOT gate this on
# LVGLPP_PLATFORM_HOST_SDL.

# DEMO-0S acceptance — Screen display descriptor.
add_executable(lvglpp_platform_screen_test screen_test.cpp)
target_link_libraries(lvglpp_platform_screen_test
    PRIVATE
        lvglpp::platform
        lvglpp_warnings
)
add_test(NAME lvglpp_platform_screen COMMAND lvglpp_platform_screen_test)
