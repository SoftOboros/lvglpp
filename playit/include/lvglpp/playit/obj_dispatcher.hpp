// obj_dispatcher.hpp — playit Dispatcher over the lv_obj tree (LVGLPP-WRAP-0N).
//
// PARITY: rlvgl/playit/src/executor.rs (v0.2.4 @ 343f596) — per-command
//         dispatch. Byte-identical Commands/Responses to the hand-rolled
//         lvglpp::playit::Dispatcher; only the tree it resolves against
//         differs (lv_obj instead of core::WidgetNode).
// LVGL:   lvgl/src/core/lv_obj_tree.h (lv_obj_set_name / get_name /
//         find_by_name), lv_obj_pos.h (coords), lv_obj_event.h (send_event).
// DELTA:  Tags resolve to lv_obj names (the WRAP-00 §5.4 channel). Tagged
//         inject sends LV_EVENT_CLICKED to the tagged object (the tag IS the
//         target); untagged inject hit-tests the tree for the topmost
//         clickable object at the point (reusing the LPAR-04 event_to_indev
//         seam to extract the point). Bounds come from lv_obj position/size.

#ifndef LVGLPP_PLAYIT_OBJ_DISPATCHER_HPP
#define LVGLPP_PLAYIT_OBJ_DISPATCHER_HPP

#include "lvglpp/core/object.hpp"  // ObjectView
#include "lvglpp/playit/command.hpp"
#include "lvglpp/playit/conversion.hpp"
#include "lvglpp/playit/response.hpp"

namespace lvglpp::playit {

// Routes parsed playit Commands into an lv_obj tree. The lv_obj re-target of
// the hand-rolled WidgetNode Dispatcher; the Command/Response wire contract is
// unchanged (Standards Action, docs/playit-tagged/00-tagged-queries.md §5.3).
class ObjDispatcher {
public:
    // Args:
    //   root: borrows the lv_obj subtree (a Screen or panel) the dispatcher
    //         resolves tags against; MUST outlive this dispatcher.
    explicit ObjDispatcher(::lvglpp::ObjectView root) noexcept : root_{root} {}

    // Replace the StatusData snapshot returned for `Status` commands.
    void set_status_snapshot(StatusData snapshot) noexcept { status_ = snapshot; }

    // Process one parsed command. noexcept: internal failure yields a
    // Response::Error rather than throwing.
    [[nodiscard]] Response dispatch(const Command& cmd) noexcept;

private:
    // Resolve `tag` to an lv_obj within root_ (root itself may carry the tag;
    // otherwise lv_obj_find_by_name searches descendants). nullptr if absent.
    [[nodiscard]] lv_obj_t* resolve(std::string_view tag) const noexcept;

    // borrows: the caller-owned lv_obj root subtree.
    ::lvglpp::ObjectView root_;
    StatusData           status_{};
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_OBJ_DISPATCHER_HPP
