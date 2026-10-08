cmake_minimum_required(VERSION 3.20)
project(lvglpp_consumer_probe LANGUAGES C CXX)

# Exercise parent-project normal variables, a header path containing spaces,
# and transitive C/C++ configuration without a Rust reference checkout.
file(READ "${LVGLPP_SOURCE_DIR}/include/lvglpp/lv_conf.h" _config)
string(REPLACE "LV_COLOR_FORMAT_ARGB8888" "LV_COLOR_FORMAT_RGB565" _config "${_config}")
string(APPEND _config "\n#define LVGLPP_CONSUMER_CONFIG_MARKER 1234\n")
set(_config_dir "${CMAKE_CURRENT_BINARY_DIR}/consumer config")
file(MAKE_DIRECTORY "${_config_dir}")
file(WRITE "${_config_dir}/lv_conf.h" "${_config}")

if(CONFIG_MODE STREQUAL "PATH")
    set(LV_BUILD_CONF_PATH "${_config_dir}/lv_conf.h")
elseif(CONFIG_MODE STREQUAL "DIR")
    set(LV_BUILD_CONF_DIR "${_config_dir}")
elseif(CONFIG_MODE STREQUAL "FALLBACK")
    # Configure once with the bundled header before selecting the consumer's
    # directory in the same build tree: the fallback must not stick in cache.
else()
    message(FATAL_ERROR "CONFIG_MODE must be PATH, DIR, or FALLBACK")
endif()

set(LVGLPP_BUILD_TESTS OFF CACHE BOOL "")
set(LVGLPP_BUILD_EXAMPLES OFF CACHE BOOL "")
set(LVGLPP_USE_RLVGL OFF CACHE BOOL "")
set(LVGLPP_RLVGL_DIR "${CMAKE_CURRENT_BINARY_DIR}/absent-rlvgl" CACHE PATH "")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "")
add_subdirectory("${LVGLPP_SOURCE_DIR}" lvglpp)

if(NOT TARGET lvglpp::core OR NOT TARGET lvglpp::i18n)
    message(FATAL_ERROR "Consumer wrapper libraries were not created")
endif()
foreach(_unexpected lvglpp_smoke lvglpp_app_disco_demo lvgl_examples lvgl_demos)
    if(TARGET ${_unexpected})
        message(FATAL_ERROR "Library-only build unexpectedly created ${_unexpected}")
    endif()
endforeach()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/probe.c" [=[
#include <lvgl.h>
#if LVGLPP_CONSUMER_CONFIG_MARKER != 1234
#error Consumer C configuration was not propagated
#endif
_Static_assert(LV_COLOR_FORMAT_DEFAULT == LV_COLOR_FORMAT_RGB565, "Consumer C color format");
int consumer_c_probe(void) { return LV_COLOR_FORMAT_DEFAULT; }
]=])
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/probe.cpp" [=[
#include <lvglpp/core/runtime.hpp>
#include <lvgl.h>
#if LVGLPP_CONSUMER_CONFIG_MARKER != 1234
#error Consumer C++ configuration was not propagated
#endif
static_assert(LV_COLOR_FORMAT_DEFAULT == LV_COLOR_FORMAT_RGB565, "Consumer C++ color format");
extern "C" int consumer_c_probe(void);
int main() {
    lvglpp::Runtime runtime;
    return consumer_c_probe() == LV_COLOR_FORMAT_RGB565 ? 0 : 1;
}
]=])
add_executable(consumer_probe
    "${CMAKE_CURRENT_BINARY_DIR}/probe.c"
    "${CMAKE_CURRENT_BINARY_DIR}/probe.cpp")
target_link_libraries(consumer_probe PRIVATE lvglpp::core)
if(MSVC)
    target_compile_options(consumer_probe PRIVATE /WX)
else()
    target_compile_options(consumer_probe PRIVATE -Werror)
endif()
