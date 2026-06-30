# lvglpp::core per-module tests — host-side unit tests for the value
# types frozen by CORE-02 / CORE-03 / CORE-04 / CORE-05.

add_executable(lvglpp_core_widget_test widget_test.cpp)
target_link_libraries(lvglpp_core_widget_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_widget COMMAND lvglpp_core_widget_test)

add_executable(lvglpp_core_renderer_test renderer_test.cpp)
target_link_libraries(lvglpp_core_renderer_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_renderer COMMAND lvglpp_core_renderer_test)

add_executable(lvglpp_core_style_test style_test.cpp)
target_link_libraries(lvglpp_core_style_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_style COMMAND lvglpp_core_style_test)

# CORE-06 acceptance.
add_executable(lvglpp_core_font_test font_test.cpp)
target_link_libraries(lvglpp_core_font_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_font COMMAND lvglpp_core_font_test)

# CORE-03a acceptance.
add_executable(lvglpp_core_widget_node_test widget_node_test.cpp)
target_link_libraries(lvglpp_core_widget_node_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_widget_node COMMAND lvglpp_core_widget_node_test)

# LPAR-CPP-02 acceptance.
add_executable(lvglpp_core_object_test object_test.cpp)
target_link_libraries(lvglpp_core_object_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_object COMMAND lvglpp_core_object_test)

# LPAR-CPP-03 acceptance.
add_executable(lvglpp_core_display_test display_test.cpp)
target_link_libraries(lvglpp_core_display_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_display COMMAND lvglpp_core_display_test)

# LPAR-CPP-04 acceptance.
add_executable(lvglpp_core_input_test input_test.cpp)
target_link_libraries(lvglpp_core_input_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_input COMMAND lvglpp_core_input_test)

# LPAR-CPP-05 acceptance.
add_executable(lvglpp_core_scroll_test scroll_test.cpp)
target_link_libraries(lvglpp_core_scroll_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_scroll COMMAND lvglpp_core_scroll_test)

# LPAR-CPP-06 acceptance.
add_executable(lvglpp_core_timer_test timer_test.cpp)
target_link_libraries(lvglpp_core_timer_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_timer COMMAND lvglpp_core_timer_test)

# LPAR-CPP-07 acceptance.
add_executable(lvglpp_core_style_lvgl_test style_lvgl_test.cpp)
target_link_libraries(lvglpp_core_style_lvgl_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_style_lvgl COMMAND lvglpp_core_style_lvgl_test)

# LPAR-CPP-08 acceptance.
add_executable(lvglpp_core_draw_lvgl_test draw_lvgl_test.cpp)
target_link_libraries(lvglpp_core_draw_lvgl_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_draw_lvgl COMMAND lvglpp_core_draw_lvgl_test)

# LPAR-CPP-09 acceptance.
add_executable(lvglpp_core_asset_lvgl_test asset_lvgl_test.cpp)
target_link_libraries(lvglpp_core_asset_lvgl_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_asset_lvgl COMMAND lvglpp_core_asset_lvgl_test)

# DEMO-04 acceptance (consume-only RLEC decoder).
add_executable(lvglpp_core_rle_test rle_test.cpp)
target_link_libraries(lvglpp_core_rle_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
# Point the test at a real lvglpp-owned icon asset; the decoder itself is
# fully covered by the hand-built fixtures.
set(LVGLPP_RLE_ASSET
    "${CMAKE_SOURCE_DIR}/examples/apps/disco-demo/assets/icons/file.rle")
if(EXISTS "${LVGLPP_RLE_ASSET}")
    target_compile_definitions(lvglpp_core_rle_test
        PRIVATE LVGLPP_RLE_ASSET_PATH="${LVGLPP_RLE_ASSET}")
endif()
add_test(NAME lvglpp_core_rle COMMAND lvglpp_core_rle_test)
