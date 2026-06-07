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
