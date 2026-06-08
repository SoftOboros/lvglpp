# lvglpp::ui — higher-level UI compositions on top of lvglpp::widgets.
#
# PARITY: rlvgl/ui (v0.2.0). Adds theming, layout helpers, draw helpers,
# event-window utilities — everything that sits between widgets and
# applications.
#
# DEMO-02 moved this module from an INTERFACE-only stub to a compiled
# STATIC library (first compiled unit: src/draw_helpers.cpp).

add_library(lvglpp_ui)
add_library(lvglpp::ui ALIAS lvglpp_ui)

target_sources(lvglpp_ui
    PRIVATE
        src/draw_helpers.cpp
)

target_include_directories(lvglpp_ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(lvglpp_ui
    PUBLIC
        lvglpp::core
        lvglpp::widgets
    PRIVATE
        lvglpp_warnings
)

target_compile_features(lvglpp_ui PUBLIC cxx_std_20)

if(LVGLPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
