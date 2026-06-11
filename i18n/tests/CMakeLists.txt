# lvglpp::i18n per-module tests.
#
# The generated surface under test (test_i18n*.{hpp,inc,bin}) is
# produced by tools/gen_i18n.py from i18n/tests/locales/ — rlvgl's own
# en.json/fr.json, so the golden cross-language check compares like
# with like (fixtures/translations_golden.bin is the Rust-built blob
# from the same inputs).

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(_i18n_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(_i18n_gen_script "${CMAKE_CURRENT_SOURCE_DIR}/../tools/gen_i18n.py")
set(_i18n_locales "${CMAKE_CURRENT_SOURCE_DIR}/locales")

add_custom_command(
    OUTPUT
        "${_i18n_gen_dir}/test_i18n.hpp"
        "${_i18n_gen_dir}/test_i18n_rltn.inc"
        "${_i18n_gen_dir}/test_i18n.bin"
        "${_i18n_gen_dir}/test_i18n_lv.hpp"
    COMMAND Python3::Interpreter "${_i18n_gen_script}"
        --locales "${_i18n_locales}"
        --out "${_i18n_gen_dir}"
        --name test_i18n
        --emit keys,rltn,lv
    DEPENDS
        "${_i18n_gen_script}"
        "${_i18n_locales}/en.json"
        "${_i18n_locales}/fr.json"
    COMMENT "gen_i18n.py: test_i18n (keys,rltn,lv)"
)

add_custom_target(lvglpp_i18n_testgen
    DEPENDS
        "${_i18n_gen_dir}/test_i18n.hpp"
        "${_i18n_gen_dir}/test_i18n_rltn.inc"
        "${_i18n_gen_dir}/test_i18n.bin"
        "${_i18n_gen_dir}/test_i18n_lv.hpp"
)

# I18N-01 acceptance: format()/Param.
add_executable(lvglpp_i18n_format_test format_test.cpp)
target_link_libraries(lvglpp_i18n_format_test
    PRIVATE lvglpp::i18n lvglpp_warnings)
add_test(NAME lvglpp_i18n_format COMMAND lvglpp_i18n_format_test)

# I18N-01 acceptance: RLTN runtime + codegen + golden cross-check.
add_executable(lvglpp_i18n_rltn_test rltn_test.cpp)
add_dependencies(lvglpp_i18n_rltn_test lvglpp_i18n_testgen)
target_include_directories(lvglpp_i18n_rltn_test PRIVATE "${_i18n_gen_dir}")
target_compile_definitions(lvglpp_i18n_rltn_test PRIVATE
    LVGLPP_I18N_GOLDEN_BLOB="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/translations_golden.bin")
target_link_libraries(lvglpp_i18n_rltn_test
    PRIVATE lvglpp::i18n lvglpp_warnings)
add_test(NAME lvglpp_i18n_rltn COMMAND lvglpp_i18n_rltn_test)

# I18N-02 acceptance: lv_translation bridge against the in-repo lvgl
# build (header-only bridge; this harness is its compile + behavior
# gate — docs/i18n/01 §5.4).
option(LVGLPP_I18N_LV_BRIDGE_TESTS
    "Build the lv_translation bridge tests against the lvgl submodule" ON)
if(LVGLPP_I18N_LV_BRIDGE_TESTS AND TARGET lvgl)
    add_executable(lvglpp_i18n_lv_bridge_test lv_bridge_test.cpp)
    add_dependencies(lvglpp_i18n_lv_bridge_test lvglpp_i18n_testgen)
    target_include_directories(lvglpp_i18n_lv_bridge_test
        PRIVATE "${_i18n_gen_dir}")
    target_link_libraries(lvglpp_i18n_lv_bridge_test
        PRIVATE lvglpp::i18n lvgl lvglpp_warnings)
    add_test(NAME lvglpp_i18n_lv_bridge COMMAND lvglpp_i18n_lv_bridge_test)
endif()
