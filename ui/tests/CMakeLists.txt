# lvglpp::ui per-module tests.

# DEMO-02 acceptance.
add_executable(lvglpp_ui_draw_helpers_test draw_helpers_test.cpp)
target_link_libraries(lvglpp_ui_draw_helpers_test
    PRIVATE
        lvglpp::ui
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_ui_draw_helpers COMMAND lvglpp_ui_draw_helpers_test)

# DEMO-03 acceptance.
add_executable(lvglpp_ui_event_window_test event_window_test.cpp)
target_link_libraries(lvglpp_ui_event_window_test
    PRIVATE
        lvglpp::ui
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_ui_event_window COMMAND lvglpp_ui_event_window_test)
