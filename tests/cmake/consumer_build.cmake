function(run_checked)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE _result
        OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Consumer check failed: ${ARGV}\n${_stdout}\n${_stderr}")
    endif()
endfunction()

if(CONFIG_MODE STREQUAL "DIR" AND NOT EXISTS "${PROBE_BINARY_DIR}/CMakeCache.txt")
    run_checked("${CMAKE_COMMAND}"
        -S "${LVGLPP_SOURCE_DIR}/tests/cmake/consumer"
        -B "${PROBE_BINARY_DIR}"
        -G "${PROBE_GENERATOR}"
        "-DCMAKE_C_COMPILER=${PROBE_C_COMPILER}"
        "-DCMAKE_CXX_COMPILER=${PROBE_CXX_COMPILER}"
        "-DLVGLPP_SOURCE_DIR=${LVGLPP_SOURCE_DIR}"
        "-DLVGLPP_LVGL_DIR=${LVGLPP_LVGL_DIR}"
        -DCONFIG_MODE=FALLBACK)
endif()

run_checked("${CMAKE_COMMAND}"
    -S "${LVGLPP_SOURCE_DIR}/tests/cmake/consumer"
    -B "${PROBE_BINARY_DIR}"
    -G "${PROBE_GENERATOR}"
    "-DCMAKE_C_COMPILER=${PROBE_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${PROBE_CXX_COMPILER}"
    "-DLVGLPP_SOURCE_DIR=${LVGLPP_SOURCE_DIR}"
    "-DLVGLPP_LVGL_DIR=${LVGLPP_LVGL_DIR}"
    "-DCONFIG_MODE=${CONFIG_MODE}"
    -DCMAKE_BUILD_TYPE=Debug)
run_checked("${CMAKE_COMMAND}" --build "${PROBE_BINARY_DIR}"
    --target consumer_probe --config Debug --parallel 4)
if(EXISTS "${PROBE_BINARY_DIR}/Debug/consumer_probe${PROBE_EXECUTABLE_SUFFIX}")
    run_checked("${PROBE_BINARY_DIR}/Debug/consumer_probe${PROBE_EXECUTABLE_SUFFIX}")
else()
    run_checked("${PROBE_BINARY_DIR}/consumer_probe${PROBE_EXECUTABLE_SUFFIX}")
endif()
