# lvglpp top-level tests — host-side smoke tests that exercise the
# aggregated lvglpp::lvglpp surface. Per-module unit tests live under
# <module>/tests/.

add_executable(lvglpp_smoke smoke_test.cpp)
target_link_libraries(lvglpp_smoke PRIVATE lvglpp::lvglpp lvglpp_warnings)
add_test(NAME lvglpp_smoke COMMAND lvglpp_smoke)

# Build a downstream project to prove config selection reaches both C and
# C++ consumers and library-only builds need no rlvgl checkout.
foreach(_config_mode PATH DIR)
    add_test(NAME lvglpp_cmake_consumer_${_config_mode}
        COMMAND "${CMAKE_COMMAND}"
            "-DLVGLPP_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "-DLVGLPP_LVGL_DIR=${LVGLPP_LVGL_DIR}"
            "-DPROBE_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer_${_config_mode}"
            "-DPROBE_GENERATOR=${CMAKE_GENERATOR}"
            "-DPROBE_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DPROBE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DPROBE_EXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
            "-DCONFIG_MODE=${_config_mode}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/consumer_build.cmake")
endforeach()
