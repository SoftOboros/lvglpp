# lvglpp::playit per-module tests — host-side parity fixtures against
# the rlvgl wire-protocol.

add_executable(lvglpp_playit_parser_test parser_test.cpp)
target_link_libraries(lvglpp_playit_parser_test
    PRIVATE
        lvglpp::playit
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_parser COMMAND lvglpp_playit_parser_test)

# CORE-02 acceptance: round-trip every spec → core variant.
add_executable(lvglpp_playit_conversion_test conversion_test.cpp)
target_link_libraries(lvglpp_playit_conversion_test
    PRIVATE
        lvglpp::playit
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_conversion COMMAND lvglpp_playit_conversion_test)

# PLAYIT-02 acceptance: NullPipeline passthrough + secondary-slot fan-out.
add_executable(lvglpp_playit_event_pipeline_test event_pipeline_test.cpp)
target_link_libraries(lvglpp_playit_event_pipeline_test
    PRIVATE
        lvglpp::playit
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_event_pipeline COMMAND lvglpp_playit_event_pipeline_test)

# PLAYIT-04 acceptance: end-to-end wire-bytes → Dispatcher → widget tree.
add_executable(lvglpp_playit_dispatcher_test dispatcher_test.cpp)
target_link_libraries(lvglpp_playit_dispatcher_test
    PRIVATE
        lvglpp::playit
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_dispatcher COMMAND lvglpp_playit_dispatcher_test)

# LVGLPP-WRAP-0N: the same wire bytes drive an lv_obj tree (ObjDispatcher).
add_executable(lvglpp_playit_obj_dispatcher_test obj_dispatcher_test.cpp)
target_link_libraries(lvglpp_playit_obj_dispatcher_test
    PRIVATE
        lvglpp::playit
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_obj_dispatcher COMMAND lvglpp_playit_obj_dispatcher_test)

# PLAYIT-04b acceptance: format_response wire output.
add_executable(lvglpp_playit_format_test format_test.cpp)
target_link_libraries(lvglpp_playit_format_test
    PRIVATE
        lvglpp::playit
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_format COMMAND lvglpp_playit_format_test)

# PLAYIT-07 acceptance: end-to-end Transport + Executor round-trip.
add_executable(lvglpp_playit_executor_test executor_test.cpp)
target_link_libraries(lvglpp_playit_executor_test
    PRIVATE
        lvglpp::playit
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_executor COMMAND lvglpp_playit_executor_test)

# PLAYIT-06 acceptance: EventRecorder + Executor RS/RE/RD wiring.
add_executable(lvglpp_playit_recorder_test recorder_test.cpp)
target_link_libraries(lvglpp_playit_recorder_test
    PRIVATE
        lvglpp::playit
        lvglpp::widgets
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_recorder COMMAND lvglpp_playit_recorder_test)

# PLAYIT-04a acceptance: gesture recognizers (Tap + DoubleTap + GesturePipeline).
add_executable(lvglpp_playit_gesture_test gesture_test.cpp)
target_link_libraries(lvglpp_playit_gesture_test
    PRIVATE
        lvglpp::playit
        lvglpp_warnings
)
add_test(NAME lvglpp_playit_gesture COMMAND lvglpp_playit_gesture_test)
