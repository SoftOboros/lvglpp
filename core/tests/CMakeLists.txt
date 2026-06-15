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
