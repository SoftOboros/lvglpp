// obj_dispatcher_test.cpp — LVGLPP-WRAP-0N acceptance: the same playit wire
// bytes (T@tag / T / QB / QE / QC / ? / D / RS..RD) drive a real lv_obj tree
// of lv_obj-backed widgets, with byte-identical Responses to the hand-rolled
// WidgetNode Dispatcher.
//
// See docs/wrap/00-concepts.md §6 (WRAP-0N playit).

#include "lvglpp/playit/obj_dispatcher.hpp"

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/playit/parser.hpp"
#include "lvglpp/playit/response.hpp"
#include "lvglpp/widgets/button.hpp"
#include "lvglpp/widgets/label.hpp"

#include <cassert>
#include <cstdint>
#include <string_view>
#include <variant>

namespace lc = lvglpp::core;
namespace lp = lvglpp::playit;
namespace lw = lvglpp::widgets;

namespace {

template <class Alt, class Variant>
const Alt* as(const Variant& v) noexcept {
    return std::get_if<Alt>(&v);
}

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// Build the lv_obj equivalent of the WidgetNode dispatcher fixture:
//   screen [tag=root]
//   ├── status label [tag=status] at {0,0,200,50}
//   └── ok button     [tag=ok]     at {40,80,120,40}, clickable, leaf
struct Fixture {
    int            clicks = 0;
    lc::Screen     screen = lc::Screen::make();
    lw::Label      status = lw::Label::make(screen.view());
    lw::Button     ok     = lw::Button::make(screen.view());
};

void build(Fixture& fx) {
    fx.screen.set_tag("root");

    fx.status.set_tag("status");
    fx.status.set_text("status");
    lv_obj_set_pos(fx.status.borrow_raw(), 0, 0);
    fx.status.set_size(200, 50);

    fx.ok.set_tag("ok");
    lv_obj_set_pos(fx.ok.borrow_raw(), 40, 80);
    fx.ok.set_size(120, 40);  // leaf: no text label child
    fx.ok.set_on_click([&fx] { fx.clicks += 1; });

    lv_obj_update_layout(fx.screen.borrow_raw());
}

void run(lv_display_t* /*disp*/) {
    {  // tagged inject delivers a click to the tagged object
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        auto cmd = lp::parse_command("T@ok:80,90");
        assert(cmd.has_value());
        lp::Response r = d.dispatch(*cmd);
        assert(as<lp::response::Ok>(r) != nullptr);
        assert(fx.clicks == 1);
    }
    {  // tagged inject, missing tag -> "tag not found"
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        auto cmd = lp::parse_command("T@nonexistent:0,0");
        lp::Response r = d.dispatch(*cmd);
        const auto* err = as<lp::response::Error>(r);
        assert(err && err->reason == "tag not found");
    }
    {  // untagged inject hit-tests the tree and clicks the button
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        auto cmd = lp::parse_command("T80,90");
        assert(cmd.has_value());
        lp::Response r = d.dispatch(*cmd);
        assert(as<lp::response::Ok>(r) != nullptr);
        assert(fx.clicks == 1);
    }
    {  // QB:ok -> bounds {40,80,120,40}
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        const auto* b = as<lp::response::Bounds>(d.dispatch(*lp::parse_command("QB:ok")));
        assert(b);
        assert(b->x == 40 && b->y == 80 && b->width == 120 && b->height == 40);
    }
    {  // QB:missing -> "tag not found"
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        const auto* err = as<lp::response::Error>(d.dispatch(*lp::parse_command("QB:missing")));
        assert(err && err->reason == "tag not found");
    }
    {  // QE yes/no
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        const auto* y = as<lp::response::Exists>(d.dispatch(*lp::parse_command("QE:status")));
        assert(y && y->value);
        const auto* n = as<lp::response::Exists>(d.dispatch(*lp::parse_command("QE:gone")));
        assert(n && !n->value);
    }
    {  // QC:root -> 2 (status + ok); QC:ok -> 0 (leaf)
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        const auto* root_cc = as<lp::response::ChildCount>(d.dispatch(*lp::parse_command("QC:root")));
        assert(root_cc && root_cc->value == 2);
        const auto* ok_cc = as<lp::response::ChildCount>(d.dispatch(*lp::parse_command("QC:ok")));
        assert(ok_cc && ok_cc->value == 0);
    }
    {  // Status snapshot
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        d.set_status_snapshot(lp::StatusData{1234, 567});
        const auto* s = as<lp::response::Status>(d.dispatch(*lp::parse_command("?")));
        assert(s && s->snapshot.tick_count == 1234 && s->snapshot.present_count == 567);
    }
    {  // deferred commands -> "not implemented"
        Fixture fx;
        build(fx);
        lp::ObjDispatcher d{fx.screen.view()};
        const auto* dp = as<lp::response::Error>(d.dispatch(*lp::parse_command("D0,0,10,10")));
        assert(dp && dp->reason == "not implemented");
        for (std::string_view line : {"RS", "RE", "RD"}) {
            const auto* e = as<lp::response::Error>(d.dispatch(*lp::parse_command(line)));
            assert(e && e->reason == "not implemented");
        }
    }
}

}  // namespace

int main() {
    auto runtime = lvglpp::Runtime::try_make();
    assert(runtime.has_value());

    static std::uint8_t draw_buf[200 * 20 * 4];
    lv_display_t* disp = lv_display_create(200, 200);
    assert(disp != nullptr);
    lv_display_set_flush_cb(disp, noop_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr,
                           static_cast<std::uint32_t>(sizeof(draw_buf)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    run(disp);

    lv_display_delete(disp);
    return 0;
}
