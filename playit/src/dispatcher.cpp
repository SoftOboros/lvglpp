// dispatcher.cpp — Dispatcher::dispatch implementation.
//
// PARITY: rlvgl/playit/src/executor.rs (v0.2.0 @ 79f730d) — per-
//         command behaviour. docs/playit-tagged/00-tagged-queries.md
//         §5.3 freezes the per-command mapping under Standards
//         Action.

#include "lvglpp/playit/dispatcher.hpp"

#include <cstdint>
#include <variant>

namespace lvglpp::playit {

namespace {

// String-literal error reasons; their lifetime is static, so they
// satisfy Response::Error's `borrows` requirement automatically.
inline constexpr std::string_view kErrTagNotFound      = "tag not found";
inline constexpr std::string_view kErrNotImplemented   = "not implemented";
inline constexpr std::string_view kErrUnknownExtension = "unhandled extension";

}  // namespace

Response Dispatcher::dispatch(const Command& cmd) noexcept {
    namespace cc = ::lvglpp::core;

    return std::visit([this](const auto& payload) -> Response {
        using T = std::decay_t<decltype(payload)>;

        // ---- §5.3 ROW: Status -----------------------------------
        if constexpr (std::is_same_v<T, command::Status>) {
            return Response{response::Status{status_}};
        }

        // ---- §5.3 ROW: Inject -----------------------------------
        else if constexpr (std::is_same_v<T, command::Inject>) {
            cc::Event ev = to_event(payload.event);
            (void)root_->dispatch_event(ev);
            return Response{response::Ok{}};
        }

        // ---- §5.3 ROW: InjectTagged ----------------------------
        else if constexpr (std::is_same_v<T, command::InjectTagged>) {
            cc::WidgetNode* node = cc::find_by_tag(*root_, payload.tag);
            if (node == nullptr || !node->widget) {
                return Response{response::Error{kErrTagNotFound}};
            }
            cc::Event ev = to_event(payload.event);
            (void)node->widget->handle_event(ev);
            return Response{response::Ok{}};
        }

        // ---- §5.3 ROWS: QueryBounds / QueryExists / QueryChildCount
        else if constexpr (std::is_same_v<T, command::QueryBounds>) {
            const cc::WidgetNode* node = cc::find_by_tag(*root_, payload.tag);
            if (node == nullptr || !node->widget) {
                return Response{response::Error{kErrTagNotFound}};
            }
            const cc::Rect b = node->widget->bounds();
            return Response{response::Bounds{b.x, b.y, b.width, b.height}};
        }
        else if constexpr (std::is_same_v<T, command::QueryExists>) {
            const cc::WidgetNode* node = cc::find_by_tag(*root_, payload.tag);
            return Response{response::Exists{node != nullptr}};
        }
        else if constexpr (std::is_same_v<T, command::QueryChildCount>) {
            const cc::WidgetNode* node = cc::find_by_tag(*root_, payload.tag);
            if (node == nullptr) {
                return Response{response::Error{kErrTagNotFound}};
            }
            return Response{response::ChildCount{
                static_cast<std::uint16_t>(node->children.size())}};
        }

        // ---- §5.3 ROWS: deferred sub-phases ---------------------
        else if constexpr (std::is_same_v<T, command::DumpPixels> ||
                           std::is_same_v<T, command::RecordStart> ||
                           std::is_same_v<T, command::RecordStop>  ||
                           std::is_same_v<T, command::RecordDump>) {
            return Response{response::Error{kErrNotImplemented}};
        }

        // ---- §5.3 ROW: Extension --------------------------------
        else if constexpr (std::is_same_v<T, command::Extension>) {
            return Response{response::Error{kErrUnknownExtension}};
        }

        // Unreachable — static_assert guards future Command variants.
        else {
            static_assert(sizeof(T) == 0,
                "Dispatcher::dispatch: unhandled Command variant — "
                "concepts doc §5.3 added a command without updating "
                "this seam");
        }
    }, cmd);
}

}  // namespace lvglpp::playit
