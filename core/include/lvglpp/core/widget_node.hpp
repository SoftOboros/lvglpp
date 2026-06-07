// widget_node.hpp — tree wrapper for the widget hierarchy.
//
// PARITY: rlvgl/core/src/lib.rs:104 (WidgetNode), :138 (dispatch_event),
//         :154 (draw); rlvgl/playit/src/tag.rs:5 (find_by_tag).
//         v0.2.0 @ d99f793.
// LVGL:   N/A — composition above the Renderer/Widget seam, no LVGL
//         analog.
// DELTA:  rlvgl uses Rc<RefCell<dyn Widget>> for shared, interior-
//         mutable widgets. lvglpp uses std::unique_ptr<Widget> — C++
//         has no borrow-checker constraint that needs RefCell, and
//         single-owner is a more honest expression of typical usage.
//         If a real call site needs sharing, swap to shared_ptr per
//         CLAUDE.md ownership rule 3, with a per-call-site comment.
//
// This file implements docs/core-widget/01-widget-node.md (CORE-03a).
// Field set, dispatch order, draw order, and find_by_tag semantics
// are FROZEN under Standards Action.

#ifndef LVGLPP_CORE_WIDGET_NODE_HPP
#define LVGLPP_CORE_WIDGET_NODE_HPP

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::core {

// Node in the widget hierarchy.
//
// Ownership tags per concepts doc §5.1:
//   widget   — owns Widget via unique_ptr.
//   children — owns the recursive subtree.
//   tag      — borrows the tag string; the underlying storage MUST
//              outlive the node (string literals satisfy this).
//
// Move-only because of the contained unique_ptr.
struct WidgetNode {
    std::unique_ptr<Widget>         widget{};
    std::vector<WidgetNode>         children{};
    std::optional<std::string_view> tag{};

    WidgetNode() = default;
    explicit WidgetNode(std::unique_ptr<Widget> w) noexcept
        : widget{std::move(w)} {}
    WidgetNode(std::unique_ptr<Widget> w, std::string_view t) noexcept
        : widget{std::move(w)}, tag{t} {}

    WidgetNode(const WidgetNode&)            = delete;
    WidgetNode& operator=(const WidgetNode&) = delete;
    WidgetNode(WidgetNode&&) noexcept            = default;
    WidgetNode& operator=(WidgetNode&&) noexcept = default;
    ~WidgetNode() = default;

    // Append a child node by move. Returns a reference to the
    // inserted child for chaining.
    WidgetNode& add_child(WidgetNode child) {
        children.push_back(std::move(child));
        return children.back();
    }

    // Depth-first event propagation per concepts doc §5.2.
    // Returns true as soon as any widget consumes the event.
    [[nodiscard]] bool dispatch_event(const Event& event) {
        if (widget && widget->handle_event(event)) {
            return true;
        }
        for (auto& child : children) {
            if (child.dispatch_event(event)) {
                return true;
            }
        }
        return false;
    }

    // Recursive draw per concepts doc §5.3 — parent first, then
    // children in vector order.
    void draw(Renderer& renderer) const {
        if (widget) {
            widget->draw(renderer);
        }
        for (const auto& child : children) {
            child.draw(renderer);
        }
    }
};

// Depth-first tag lookup. Const overload returns const pointer; the
// non-const overload returns mutable. Returns nullptr on miss.
// Mirrors rlvgl/playit/src/tag.rs:5.
[[nodiscard]] inline const WidgetNode*
find_by_tag(const WidgetNode& root, std::string_view tag) noexcept {
    if (root.tag && *root.tag == tag) {
        return &root;
    }
    for (const auto& child : root.children) {
        if (const auto* hit = find_by_tag(child, tag)) {
            return hit;
        }
    }
    return nullptr;
}

[[nodiscard]] inline WidgetNode*
find_by_tag(WidgetNode& root, std::string_view tag) noexcept {
    if (root.tag && *root.tag == tag) {
        return &root;
    }
    for (auto& child : root.children) {
        if (auto* hit = find_by_tag(child, tag)) {
            return hit;
        }
    }
    return nullptr;
}

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_WIDGET_NODE_HPP
