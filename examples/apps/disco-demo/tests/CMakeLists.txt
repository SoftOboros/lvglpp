# lvglpp::app_disco_demo per-widget tests (DEMO-05).
#
# Each test mirrors the rlvgl unit behavior for its composite widget and
# links only the app module (which transitively pulls core/ui/widgets).
# Icon-byte fixtures are hand-built RLEC blobs so the tests do not depend on
# the rlvgl submodule being present.

add_executable(lvglpp_app_disco_demo_icon_strip_test icon_strip_test.cpp)
target_link_libraries(lvglpp_app_disco_demo_icon_strip_test
    PRIVATE lvglpp::app_disco_demo lvglpp_warnings)
add_test(NAME lvglpp_app_disco_demo_icon_strip
         COMMAND lvglpp_app_disco_demo_icon_strip_test)

add_executable(lvglpp_app_disco_demo_wing_test wing_test.cpp)
target_link_libraries(lvglpp_app_disco_demo_wing_test
    PRIVATE lvglpp::app_disco_demo lvglpp_warnings)
add_test(NAME lvglpp_app_disco_demo_wing
         COMMAND lvglpp_app_disco_demo_wing_test)

add_executable(lvglpp_app_disco_demo_dashboard_panel_test
               dashboard_panel_test.cpp)
target_link_libraries(lvglpp_app_disco_demo_dashboard_panel_test
    PRIVATE lvglpp::app_disco_demo lvglpp_warnings)
add_test(NAME lvglpp_app_disco_demo_dashboard_panel
         COMMAND lvglpp_app_disco_demo_dashboard_panel_test)

add_executable(lvglpp_app_disco_demo_hotspot_test hotspot_test.cpp)
target_link_libraries(lvglpp_app_disco_demo_hotspot_test
    PRIVATE lvglpp::app_disco_demo lvglpp_warnings)
add_test(NAME lvglpp_app_disco_demo_hotspot
         COMMAND lvglpp_app_disco_demo_hotspot_test)

# DEMO-06 — controller + FSM parity suite (SDL-free; mirrors the rlvgl
# #[cfg(test)] suite in lib.rs).
add_executable(lvglpp_app_disco_demo_controller_test disco_controller_test.cpp)
target_link_libraries(lvglpp_app_disco_demo_controller_test
    PRIVATE lvglpp::app_disco_demo lvglpp_warnings)
add_test(NAME lvglpp_app_disco_demo_controller
         COMMAND lvglpp_app_disco_demo_controller_test)
