// obj_dispatcher.cpp — ObjDispatcher::dispatch implementation (LVGLPP-WRAP-0N).
//
// PARITY: rlvgl/playit/src/executor.rs (v0.2.4 @ 343f596) — per-command
//         behaviour. docs/playit-tagged/00-tagged-queries.md §5.3 freezes the
//         per-command mapping (Standards Action).
// LVGL:   lvgl/src/core/lv_obj_tree.c, lv_obj_pos.c, lv_obj_event.c.
// DELTA:  see obj_dispatcher.hpp.

#include "lvglpp/playit/obj_dispatcher.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <variant>

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::playit {

namespace {

inline constexpr std::string_view kErrTagNotFound      = "tag not found";
inline constexpr std::string_view kErrNotImplemented   = "not implemented";
inline constexpr std::string_view kErrUnknownExtension = "unhandled extension";

// Deepest clickable lv_obj whose absolute coords contain (x, y), searched
// topmost-first (later children draw on top). nullptr if none.
lv_obj_t* hit_at(lv_obj_t* obj, std::int32_t x, std::int32_t y) noexcept {
    if (obj == nullptr) {
        return nullptr;
    }
    const std::uint32_t n = lv_obj_get_child_count(obj);
    for (std::uint32_t i = n; i-- > 0;) {
        lv_obj_t* hit = hit_at(lv_obj_get_child(obj, static_cast<std::int32_t>(i)), x, y);
        if (hit != nullptr) {
            return hit;
        }
    }
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    if (x >= area.x1 && x <= area.x2 && y >= area.y1 && y <= area.y2 &&
        lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        return obj;
    }
    return nullptr;
}

bool event_point(const EventSpec& spec, std::int32_t& x, std::int32_t& y) noexcept {
    return std::visit([&](const auto& payload) -> bool {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, event_spec::PressRelease> ||
                      std::is_same_v<T, event_spec::PressDown> ||
                      std::is_same_v<T, event_spec::PointerDown> ||
                      std::is_same_v<T, event_spec::PointerUp> ||
                      std::is_same_v<T, event_spec::PointerMove> ||
                      std::is_same_v<T, event_spec::DoubleTap>) {
            x = payload.x;
            y = payload.y;
            return true;
        } else if constexpr (std::is_same_v<T, event_spec::Touch>) {
            if (payload.count == 0U) {
                return false;
            }
            x = payload.points[0].x;
            y = payload.points[0].y;
            return true;
        } else {
            return false;
        }
    }, spec);
}

}  // namespace

lv_obj_t* ObjDispatcher::resolve(std::string_view tag) const noexcept {
    lv_obj_t* root = root_.borrow_raw();
    if (root == nullptr) {
        return nullptr;
    }
    // lv_obj_find_by_name needs a NUL-terminated string; copy the view.
    char buf[128];
    if (tag.size() >= sizeof(buf)) {
        return nullptr;  // tag longer than any real name -> not found
    }
    std::memcpy(buf, tag.data(), tag.size());
    buf[tag.size()] = '\0';

    // find_by_name searches descendants only; the root may carry the tag.
    const char* root_name = lv_obj_get_name(root);
    if (root_name != nullptr && std::strcmp(root_name, buf) == 0) {
        return root;
    }
    return lv_obj_find_by_name(root, buf);
}

Response ObjDispatcher::dispatch(const Command& cmd) noexcept {
    return std::visit([this](const auto& payload) -> Response {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, command::Status>) {
            return Response{response::Status{status_}};
        }

        else if constexpr (std::is_same_v<T, command::Inject>) {
            // Untagged inject: hit-test the tree for the topmost clickable
            // object at the event point and click it (best-effort; always Ok,
            // matching rlvgl).
            std::int32_t x = 0;
            std::int32_t y = 0;
            if (event_point(payload.event, x, y)) {
                lv_obj_t* target = hit_at(root_.borrow_raw(), x, y);
                if (target != nullptr) {
                    static_cast<void>(lv_obj_send_event(target, LV_EVENT_CLICKED, nullptr));
                }
            }
            return Response{response::Ok{}};
        }

        else if constexpr (std::is_same_v<T, command::InjectTagged>) {
            lv_obj_t* obj = resolve(payload.tag);
            if (obj == nullptr) {
                return Response{response::Error{kErrTagNotFound}};
            }
            // The tag is the target: deliver the click to it directly.
            static_cast<void>(lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr));
            return Response{response::Ok{}};
        }

        else if constexpr (std::is_same_v<T, command::QueryBounds>) {
            lv_obj_t* obj = resolve(payload.tag);
            if (obj == nullptr) {
                return Response{response::Error{kErrTagNotFound}};
            }
            return Response{response::Bounds{lv_obj_get_x(obj), lv_obj_get_y(obj),
                                             lv_obj_get_width(obj),
                                             lv_obj_get_height(obj)}};
        }
        else if constexpr (std::is_same_v<T, command::QueryExists>) {
            return Response{response::Exists{resolve(payload.tag) != nullptr}};
        }
        else if constexpr (std::is_same_v<T, command::QueryChildCount>) {
            lv_obj_t* obj = resolve(payload.tag);
            if (obj == nullptr) {
                return Response{response::Error{kErrTagNotFound}};
            }
            return Response{response::ChildCount{
                static_cast<std::uint16_t>(lv_obj_get_child_count(obj))}};
        }

        else if constexpr (std::is_same_v<T, command::DumpPixels> ||
                           std::is_same_v<T, command::RecordStart> ||
                           std::is_same_v<T, command::RecordStop> ||
                           std::is_same_v<T, command::RecordDump>) {
            return Response{response::Error{kErrNotImplemented}};
        }

        else if constexpr (std::is_same_v<T, command::Extension>) {
            return Response{response::Error{kErrUnknownExtension}};
        }

        else {
            static_assert(sizeof(T) == 0,
                "ObjDispatcher::dispatch: unhandled Command variant — concepts "
                "doc §5.3 added a command without updating this seam");
        }
    }, cmd);
}

}  // namespace lvglpp::playit
