# lvglpp::widgets per-module tests.

add_executable(lvglpp_widgets_label_test label_test.cpp)
target_link_libraries(lvglpp_widgets_label_test
    PRIVATE
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_widgets_label COMMAND lvglpp_widgets_label_test)

add_executable(lvglpp_widgets_button_test button_test.cpp)
target_link_libraries(lvglpp_widgets_button_test
    PRIVATE
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_widgets_button COMMAND lvglpp_widgets_button_test)

# WID-03 acceptance.
add_executable(lvglpp_widgets_checkbox_test checkbox_test.cpp)
target_link_libraries(lvglpp_widgets_checkbox_test
    PRIVATE lvglpp::widgets lvglpp_warnings)
add_test(NAME lvglpp_widgets_checkbox COMMAND lvglpp_widgets_checkbox_test)

add_executable(lvglpp_widgets_switch_test switch_test.cpp)
target_link_libraries(lvglpp_widgets_switch_test
    PRIVATE lvglpp::widgets lvglpp_warnings)
add_test(NAME lvglpp_widgets_switch COMMAND lvglpp_widgets_switch_test)

# WID-04 acceptance.
add_executable(lvglpp_widgets_slider_test slider_test.cpp)
target_link_libraries(lvglpp_widgets_slider_test
    PRIVATE lvglpp::widgets lvglpp_warnings)
add_test(NAME lvglpp_widgets_slider COMMAND lvglpp_widgets_slider_test)
