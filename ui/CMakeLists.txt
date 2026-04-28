# lvglpp::ui — higher-level UI compositions on top of lvglpp::widgets.
#
# PARITY: rlvgl/ui (v0.2.0). Adds theming, layout helpers, draw helpers,
# event-window utilities — everything that sits between widgets and
# applications.

add_library(lvglpp_ui INTERFACE)
add_library(lvglpp::ui ALIAS lvglpp_ui)

target_include_directories(lvglpp_ui
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(lvglpp_ui
    INTERFACE
        lvglpp::core
        lvglpp::widgets
)

target_compile_features(lvglpp_ui INTERFACE cxx_std_20)
