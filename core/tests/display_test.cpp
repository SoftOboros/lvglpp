// display_test.cpp — LPAR-03 acceptance: RAII Display over lv_display_t
// (flush-cb + draw-buffer + render-mode), active-screen view, and the
// invalidation entry point.
//
// See docs/core-object/01-invalidation-display.md §12.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::Display;
using lvglpp::core::Object;
using lvglpp::core::RenderMode;

namespace {

// dma/external: caller-owned render buffer handed to LVGL. Static storage
// guarantees it outlives the Display (CLAUDE.md ownership rule 10).
std::uint8_t g_draw_buf[100 * 20 * 4];

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// Empty-safe: a moved-from Display answers every accessor with a safe
// default and every setter is a no-op.
void test_empty_display_safe() {
    Display d = Display::make(100, 100);
    d.set_flush_cb(&noop_flush);
    d.set_buffers(g_draw_buf, nullptr, static_cast<std::uint32_t>(sizeof(g_draw_buf)),
                  RenderMode::Partial);

    Display moved = std::move(d);
    (void)moved;
    assert(d.empty());
    assert(d.borrow_raw() == nullptr);
    assert(d.active_screen().empty());
    d.set_flush_cb(&noop_flush);                 // no-op, must not crash
    d.set_buffers(g_draw_buf, nullptr, 0U, RenderMode::Full);
    d.set_render_mode(RenderMode::Direct);
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    Display disp = Display::make(100, 100);
    assert(!disp.empty());
    assert(disp.borrow_raw() != nullptr);

    disp.set_flush_cb(&noop_flush);
    disp.set_buffers(g_draw_buf, nullptr, static_cast<std::uint32_t>(sizeof(g_draw_buf)),
                     RenderMode::Partial);
    disp.set_render_mode(RenderMode::Partial);

    // The display owns an active screen, surfaced as an ObjectView.
    ObjectView screen = disp.active_screen();
    assert(!screen.empty());
    assert(screen.borrow_raw() == lv_screen_active());

    // A child under the active screen, and the invalidation entry point.
    // This confirms the lv_obj_invalidate primitive links — the future
    // Object::invalidate() (LPAR-03) wraps exactly this call.
    Object child = Object::make(screen);
    assert(!child.empty());
    lv_obj_invalidate(child.borrow_raw());

    test_empty_display_safe();

    return 0;
}
