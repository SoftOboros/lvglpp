// dispatcher_test.cpp — PLAYIT-04 acceptance: end-to-end
// "wire-bytes → Command → Dispatcher → WidgetNode → widget callback"
// path. This is the cross-language closure: the same wire bytes
// rlvgl playit fixtures send drive a real lvglpp widget tree.

#include "lvglpp/playit/playit.hpp"

#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/widgets/legacy/button.hpp"
#include "lvglpp/widgets/legacy/label.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace lc = lvglpp::core;
namespace lp = lvglpp::playit;
namespace lw = lvglpp::widgets::legacy;

namespace {

template <class Alt, class Variant>
const Alt* as(const Variant& v) noexcept {
    return std::get_if<Alt>(&v);
}

// Build a small tree:
//   root [tag=root, no widget shell — uses a no-op Label as a stand-in]
//   ├── status_label  [tag=status]
//   └── ok_button     [tag=ok]
//        └── (leaf — no children)
struct Fixture {
    int                button_clicks = 0;
    lc::WidgetNode     root;
    lw::Button*        button_ptr  = nullptr;
    lw::Label*         label_ptr   = nullptr;
};

Fixture make_fixture() {
    Fixture fx{};

    auto root_label = std::make_unique<lw::Label>(
        std::string{""}, lc::Rect{0, 0, 200, 200});
    fx.root = lc::WidgetNode{std::move(root_label), "root"};

    auto status_label = std::make_unique<lw::Label>(
        std::string{"status"}, lc::Rect{0, 0, 200, 50});
    fx.label_ptr = status_label.get();
    fx.root.add_child(lc::WidgetNode{std::move(status_label), "status"});

    auto button = std::make_unique<lw::Button>(
        std::string{"OK"}, lc::Rect{40, 80, 120, 40});
    button->set_on_click([&fx](lw::Button&) { fx.button_clicks += 1; });
    fx.button_ptr = button.get();
    fx.root.add_child(lc::WidgetNode{std::move(button), "ok"});

    return fx;
}

// ---- Cross-language closure: wire-bytes → click ---------------------

void test_tagged_press_release_drives_button() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    // The exact bytes a rlvgl playit fixture would send. The button
    // is at bounds {40, 80, 120, 40}; we tap inside.
    auto cmd = lp::parse_command("T@ok:80,90");
    assert(cmd.has_value());

    lp::Response resp = dispatcher.dispatch(*cmd);
    assert(as<lp::response::Ok>(resp) != nullptr);
    assert(fx.button_clicks == 1);
}

void test_tagged_press_release_outside_does_not_fire() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    // Tap is OUTSIDE the button's bounds — Dispatcher still calls
    // handle_event on the tagged node, but Button's inside_bounds
    // check rejects so on_click does NOT fire.
    auto cmd = lp::parse_command("T@ok:5,5");
    assert(cmd.has_value());

    lp::Response resp = dispatcher.dispatch(*cmd);
    // Dispatch itself succeeded — the button just didn't fire.
    assert(as<lp::response::Ok>(resp) != nullptr);
    assert(fx.button_clicks == 0);
}

void test_tagged_inject_missing_tag_returns_error() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    auto cmd = lp::parse_command("T@nonexistent:0,0");
    assert(cmd.has_value());
    lp::Response resp = dispatcher.dispatch(*cmd);
    const auto* err = as<lp::response::Error>(resp);
    assert(err != nullptr);
    assert(err->reason == "tag not found");
}

// ---- Untagged inject: tree DFS dispatch ----------------------------

void test_untagged_press_release_walks_tree() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    // Untagged tap inside the button's region — DFS dispatch reaches
    // the button (root no-op label first ignores, then status label
    // ignores, then button consumes).
    auto cmd = lp::parse_command("T80,90");
    assert(cmd.has_value());

    lp::Response resp = dispatcher.dispatch(*cmd);
    assert(as<lp::response::Ok>(resp) != nullptr);
    assert(fx.button_clicks == 1);
}

// ---- Query commands ------------------------------------------------

void test_query_bounds_ok() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    auto cmd = lp::parse_command("QB:ok");
    assert(cmd.has_value());
    lp::Response resp = dispatcher.dispatch(*cmd);
    const auto* b = as<lp::response::Bounds>(resp);
    assert(b);
    assert(b->x == 40 && b->y == 80 && b->width == 120 && b->height == 40);
}

void test_query_bounds_missing_tag_errors() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    auto cmd = lp::parse_command("QB:missing");
    lp::Response resp = dispatcher.dispatch(*cmd);
    const auto* err = as<lp::response::Error>(resp);
    assert(err && err->reason == "tag not found");
}

void test_query_exists_yes_no() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    auto cmd_yes = lp::parse_command("QE:status");
    const auto* y = as<lp::response::Exists>(dispatcher.dispatch(*cmd_yes));
    assert(y && y->value);

    auto cmd_no = lp::parse_command("QE:gone");
    const auto* n = as<lp::response::Exists>(dispatcher.dispatch(*cmd_no));
    assert(n && !n->value);
}

void test_query_child_count_root_has_two() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    auto cmd = lp::parse_command("QC:root");
    const auto* cc = as<lp::response::ChildCount>(dispatcher.dispatch(*cmd));
    assert(cc);
    assert(cc->value == 2);  // status + ok
}

void test_query_child_count_leaf_is_zero() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    auto cmd = lp::parse_command("QC:ok");
    const auto* cc = as<lp::response::ChildCount>(dispatcher.dispatch(*cmd));
    assert(cc && cc->value == 0);
}

// ---- Status command ------------------------------------------------

void test_status_returns_snapshot() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};
    dispatcher.set_status_snapshot(lp::StatusData{1234, 567});

    auto cmd = lp::parse_command("?");
    lp::Response resp = dispatcher.dispatch(*cmd);
    const auto* s = as<lp::response::Status>(resp);
    assert(s);
    assert(s->snapshot.tick_count    == 1234);
    assert(s->snapshot.present_count == 567);
}

// ---- Deferred commands return Error{not implemented} ---------------

void test_dump_pixels_not_implemented() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    auto cmd = lp::parse_command("D0,0,10,10");
    const auto* err = as<lp::response::Error>(dispatcher.dispatch(*cmd));
    assert(err && err->reason == "not implemented");
}

void test_record_commands_not_implemented() {
    Fixture fx = make_fixture();
    lp::Dispatcher dispatcher{fx.root};

    for (std::string_view line : {"RS", "RE", "RD"}) {
        auto cmd = lp::parse_command(line);
        const auto* err = as<lp::response::Error>(dispatcher.dispatch(*cmd));
        assert(err && err->reason == "not implemented");
    }
}

}  // namespace

int main() {
    test_tagged_press_release_drives_button();
    test_tagged_press_release_outside_does_not_fire();
    test_tagged_inject_missing_tag_returns_error();
    test_untagged_press_release_walks_tree();
    test_query_bounds_ok();
    test_query_bounds_missing_tag_errors();
    test_query_exists_yes_no();
    test_query_child_count_root_has_two();
    test_query_child_count_leaf_is_zero();
    test_status_returns_snapshot();
    test_dump_pixels_not_implemented();
    test_record_commands_not_implemented();
    return 0;
}
