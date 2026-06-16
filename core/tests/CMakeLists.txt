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

# LVGLPP-WRAP-00 acceptance (RAII lv_obj Object/Screen).
add_executable(lvglpp_core_object_test object_test.cpp)
target_link_libraries(lvglpp_core_object_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_object COMMAND lvglpp_core_object_test)

# LPAR-02 acceptance (object flags/state/hit-test/tree over lv_obj_*).
add_executable(lvglpp_core_object_substrate_test object_substrate_test.cpp)
target_link_libraries(lvglpp_core_object_substrate_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_object_substrate COMMAND lvglpp_core_object_substrate_test)

# LPAR-05 acceptance (scroll over lv_obj_scroll_*).
add_executable(lvglpp_core_object_scroll_test object_scroll_test.cpp)
target_link_libraries(lvglpp_core_object_scroll_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_object_scroll COMMAND lvglpp_core_object_scroll_test)

# LPAR-10 acceptance (flex/grid layout + sizing over lv_obj_set_*).
add_executable(lvglpp_core_object_layout_test object_layout_test.cpp)
target_link_libraries(lvglpp_core_object_layout_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_object_layout COMMAND lvglpp_core_object_layout_test)

# LPAR-03 acceptance (RAII Display over lv_display_t + Object::invalidate).
add_executable(lvglpp_core_display_test display_test.cpp)
target_link_libraries(lvglpp_core_display_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_display COMMAND lvglpp_core_display_test)

# LPAR-06 acceptance (RAII Timer over lv_timer_t + Animation over lv_anim_t).
add_executable(lvglpp_core_timer_test timer_test.cpp)
target_link_libraries(lvglpp_core_timer_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_timer COMMAND lvglpp_core_timer_test)

# LPAR-08 acceptance (Font handle over lv_font_t + draw helpers).
add_executable(lvglpp_core_draw_test draw_test.cpp)
target_link_libraries(lvglpp_core_draw_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_draw COMMAND lvglpp_core_draw_test)

# FONT-00 acceptance (Font builtins/metrics/AA + cascade font selection).
add_executable(lvglpp_core_font_select_test font_select_test.cpp)
target_link_libraries(lvglpp_core_font_select_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_font_select COMMAND lvglpp_core_font_select_test)

# FONT-05 acceptance (FontId + heap-free FontRegistry register/lookup/apply).
add_executable(lvglpp_core_font_registry_test font_registry_test.cpp)
target_link_libraries(lvglpp_core_font_registry_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_font_registry COMMAND lvglpp_core_font_registry_test)

# LPAR-04 acceptance (Object::on event cbs + FocusGroup/InputDevice + seam).
add_executable(lvglpp_core_input_test input_test.cpp)
target_link_libraries(lvglpp_core_input_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_input COMMAND lvglpp_core_input_test)

# LPAR-07 acceptance (RAII Style over lv_style_t + selectors + Theme handle).
add_executable(lvglpp_core_style_cascade_test style_cascade_test.cpp)
target_link_libraries(lvglpp_core_style_cascade_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_style_cascade COMMAND lvglpp_core_style_cascade_test)

# LPAR-09 acceptance (ImageDecoder / FsDriver RAII over lv_image_decoder/lv_fs_drv).
add_executable(lvglpp_core_asset_test asset_test.cpp)
target_link_libraries(lvglpp_core_asset_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_asset COMMAND lvglpp_core_asset_test)

# CORE-03a acceptance.
add_executable(lvglpp_core_widget_node_test widget_node_test.cpp)
target_link_libraries(lvglpp_core_widget_node_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
add_test(NAME lvglpp_core_widget_node COMMAND lvglpp_core_widget_node_test)

# DEMO-04 acceptance (consume-only RLEC decoder).
add_executable(lvglpp_core_rle_test rle_test.cpp)
target_link_libraries(lvglpp_core_rle_test
    PRIVATE
        lvglpp::core
        lvglpp_warnings
)
# Point the test at a real rlvgl icon asset when the (non-recursive) rlvgl
# submodule is checked out; otherwise the test skips that sub-case. The
# decoder itself is fully covered by the hand-built fixtures.
set(LVGLPP_RLE_ASSET
    "${CMAKE_SOURCE_DIR}/rlvgl/examples/stm32h747i-disco/assets/icons/file.rle")
if(EXISTS "${LVGLPP_RLE_ASSET}")
    target_compile_definitions(lvglpp_core_rle_test
        PRIVATE LVGLPP_RLE_ASSET_PATH="${LVGLPP_RLE_ASSET}")
endif()
add_test(NAME lvglpp_core_rle COMMAND lvglpp_core_rle_test)
