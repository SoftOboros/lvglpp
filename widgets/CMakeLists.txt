# lvglpp::widgets — concrete widget set on top of lvglpp::core.
#
# PARITY: rlvgl/widgets (v0.2.0). Widgets land per-chapter; WID-01
# (Label) is the first concrete widget.

add_library(lvglpp_widgets)
add_library(lvglpp::widgets ALIAS lvglpp_widgets)

target_sources(lvglpp_widgets
    PRIVATE
        src/legacy/button.cpp
        src/legacy/checkbox.cpp
        src/legacy/container.cpp
        src/legacy/image.cpp
        src/legacy/label.cpp
        src/legacy/list.cpp
        src/legacy/slider.cpp
        src/legacy/switch.cpp
)

target_include_directories(lvglpp_widgets
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(lvglpp_widgets
    PUBLIC
        lvglpp::core
    PRIVATE
        lvglpp_warnings
)

target_compile_features(lvglpp_widgets PUBLIC cxx_std_20)

if(LVGLPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
