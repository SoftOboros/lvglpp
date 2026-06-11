# lvglpp::i18n — RLTN translation runtime + codegen (I18N-01) and the
# lv_translation bridge header (I18N-02).
#
# PARITY: rlvgl/i18n (v0.2.0). The runtime is key-agnostic; per-app
# Locale/Key enums + blobs come from tools/gen_i18n.py at consumer
# configure/build time. The lv bridge is header-only (no lv_conf ABI
# baked into a prebuilt object) — consumers compile it against their
# own lvgl tree; LVGLPP_I18N_LV_BRIDGE_TESTS exercises it here against
# the lvgl submodule.

add_library(lvglpp_i18n)
add_library(lvglpp::i18n ALIAS lvglpp_i18n)

target_sources(lvglpp_i18n
    PRIVATE
        src/format.cpp
        src/rltn.cpp
)

target_include_directories(lvglpp_i18n
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(lvglpp_i18n
    PUBLIC
        lvglpp::expected
    PRIVATE
        lvglpp_warnings
)

target_compile_features(lvglpp_i18n PUBLIC cxx_std_20)

if(LVGLPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
