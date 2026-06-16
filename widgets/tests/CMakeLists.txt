# lvglpp::widgets per-module tests.

# LVGLPP-WRAP-01: lv_obj-backed Label (core::Object over lv_label).
add_executable(lvglpp_widgets_label_obj_test label_obj_test.cpp)
target_link_libraries(lvglpp_widgets_label_obj_test
    PRIVATE
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_widgets_label_obj COMMAND lvglpp_widgets_label_obj_test)

# LVGLPP-WRAP-02..06: lv_obj-backed Button/Checkbox/Switch/Slider/Container/
# List/Image.
add_executable(lvglpp_widgets_obj_test widgets_obj_test.cpp)
target_link_libraries(lvglpp_widgets_obj_test
    PRIVATE
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_widgets_obj COMMAND lvglpp_widgets_obj_test)

# WID-01 (legacy hand-rolled Label) acceptance.
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

# DEMO-01 acceptance (disco-demo prerequisite).
add_executable(lvglpp_widgets_container_test container_test.cpp)
target_link_libraries(lvglpp_widgets_container_test
    PRIVATE lvglpp::widgets lvglpp_warnings)
add_test(NAME lvglpp_widgets_container COMMAND lvglpp_widgets_container_test)

# WID-05 acceptance (List half; Container landed via DEMO-01).
add_executable(lvglpp_widgets_list_test list_test.cpp)
target_link_libraries(lvglpp_widgets_list_test
    PRIVATE lvglpp::widgets lvglpp_warnings)
add_test(NAME lvglpp_widgets_list COMMAND lvglpp_widgets_list_test)

# WID-06 acceptance.
add_executable(lvglpp_widgets_image_test image_test.cpp)
target_link_libraries(lvglpp_widgets_image_test
    PRIVATE lvglpp::widgets lvglpp_warnings)
add_test(NAME lvglpp_widgets_image COMMAND lvglpp_widgets_image_test)
