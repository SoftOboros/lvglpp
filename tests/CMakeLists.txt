# lvglpp top-level tests — host-side smoke tests that exercise the
# aggregated lvglpp::lvglpp surface. Per-module unit tests live under
# <module>/tests/.

add_executable(lvglpp_smoke smoke_test.cpp)
target_link_libraries(lvglpp_smoke PRIVATE lvglpp::lvglpp lvglpp_warnings)
add_test(NAME lvglpp_smoke COMMAND lvglpp_smoke)
