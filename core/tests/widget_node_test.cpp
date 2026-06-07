// widget_node_test.cpp — CORE-03a acceptance: dispatch_event order,
// draw order, find_by_tag (const + non-const), missing-tag returns
// nullptr.
//
// Mirrors the rlvgl tag tests at rlvgl/playit/src/tag.rs.

#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace lvglpp::core;

namespace {

// Trace which widget consumed which event, in order.
struct Tracer {
    std::vector<std::string> events;
    std::vector<std::string> draws;
};

// Stub widget that records its label on draw / handle_event.
struct StubWidget : Widget {
    std::string  label;
    Tracer*      tracer;
    bool         consume_events = false;

    StubWidget(std::string l, Tracer* t) : label{std::move(l)}, tracer{t} {}

    [[nodiscard]] Rect bounds() const override { return Rect{}; }

    void draw(Renderer& /*r*/) const override {
        if (tracer) tracer->draws.push_back(label);
    }

    [[nodiscard]] bool handle_event(const Event& /*e*/) override {
        if (tracer) tracer->events.push_back(label);
        return consume_events;
    }
};

struct NullRenderer : Renderer {
    void fill_rect(Rect, Color) override {}
    void draw_text(std::int32_t, std::int32_t, std::string_view, Color) override {}
};

void test_dispatch_event_dfs_order() {
    Tracer t;
    auto root_w = std::make_unique<StubWidget>("root", &t);
    auto a_w    = std::make_unique<StubWidget>("a", &t);
    auto b_w    = std::make_unique<StubWidget>("b", &t);
    auto deep_w = std::make_unique<StubWidget>("deep", &t);

    WidgetNode root{std::move(root_w)};
    auto& a = root.add_child(WidgetNode{std::move(a_w)});
    auto& b = root.add_child(WidgetNode{std::move(b_w)});
    (void)a;
    b.add_child(WidgetNode{std::move(deep_w)});

    Event tick{event::Tick{}};
    bool consumed = root.dispatch_event(tick);
    assert(!consumed);
    // DFS order: root → a → b → deep.
    assert(t.events.size() == 4);
    assert(t.events[0] == "root");
    assert(t.events[1] == "a");
    assert(t.events[2] == "b");
    assert(t.events[3] == "deep");
}

void test_dispatch_event_first_consume_short_circuits() {
    Tracer t;
    auto root_w = std::make_unique<StubWidget>("root", &t);
    auto a_w    = std::make_unique<StubWidget>("a", &t);
    auto b_w    = std::make_unique<StubWidget>("b", &t);
    a_w->consume_events = true;  // a consumes; b should never be called.

    WidgetNode root{std::move(root_w)};
    root.add_child(WidgetNode{std::move(a_w)});
    root.add_child(WidgetNode{std::move(b_w)});

    Event tick{event::Tick{}};
    bool consumed = root.dispatch_event(tick);
    assert(consumed);
    assert(t.events.size() == 2);  // root, a — b never reached.
    assert(t.events[0] == "root");
    assert(t.events[1] == "a");
}

void test_draw_dfs_order() {
    Tracer t;
    auto root_w = std::make_unique<StubWidget>("root", &t);
    auto a_w    = std::make_unique<StubWidget>("a", &t);
    auto b_w    = std::make_unique<StubWidget>("b", &t);

    WidgetNode root{std::move(root_w)};
    root.add_child(WidgetNode{std::move(a_w)});
    root.add_child(WidgetNode{std::move(b_w)});

    NullRenderer r;
    root.draw(r);

    assert(t.draws.size() == 3);
    assert(t.draws[0] == "root");  // parent before children.
    assert(t.draws[1] == "a");
    assert(t.draws[2] == "b");
}

void test_find_by_tag_root() {
    auto w = std::make_unique<StubWidget>("only", nullptr);
    WidgetNode root{std::move(w), "root"};

    assert(find_by_tag(root, "root") == &root);
    assert(find_by_tag(root, "other") == nullptr);
}

void test_find_by_tag_nested() {
    auto root_w = std::make_unique<StubWidget>("root", nullptr);
    auto a_w    = std::make_unique<StubWidget>("a", nullptr);
    auto b_w    = std::make_unique<StubWidget>("b", nullptr);
    auto deep_w = std::make_unique<StubWidget>("deep", nullptr);

    WidgetNode root{std::move(root_w)};
    root.add_child(WidgetNode{std::move(a_w), "a"});
    auto& b = root.add_child(WidgetNode{std::move(b_w), "b"});
    b.add_child(WidgetNode{std::move(deep_w), "deep"});

    assert(find_by_tag(root, "a") != nullptr);
    assert(find_by_tag(root, "b") != nullptr);
    assert(find_by_tag(root, "deep") != nullptr);
    assert(find_by_tag(root, "missing") == nullptr);
}

void test_find_by_tag_untagged_tree() {
    auto root_w  = std::make_unique<StubWidget>("root", nullptr);
    auto child_w = std::make_unique<StubWidget>("child", nullptr);
    WidgetNode root{std::move(root_w)};
    root.add_child(WidgetNode{std::move(child_w)});
    assert(find_by_tag(root, "anything") == nullptr);
}

void test_find_by_tag_mut_returns_mutable() {
    auto root_w = std::make_unique<StubWidget>("root", nullptr);
    auto kid_w  = std::make_unique<StubWidget>("kid", nullptr);
    WidgetNode root{std::move(root_w), "root"};
    root.add_child(WidgetNode{std::move(kid_w), "kid"});

    WidgetNode* node = find_by_tag(root, "kid");
    assert(node != nullptr);
    // Prove the non-const overload is in play (we can move-assign to it).
    node->tag = std::string_view{"renamed"};
    assert(find_by_tag(root, "kid") == nullptr);
    assert(find_by_tag(root, "renamed") != nullptr);
}

}  // namespace

int main() {
    test_dispatch_event_dfs_order();
    test_dispatch_event_first_consume_short_circuits();
    test_draw_dfs_order();
    test_find_by_tag_root();
    test_find_by_tag_nested();
    test_find_by_tag_untagged_tree();
    test_find_by_tag_mut_returns_mutable();
    return 0;
}
