// draw_test.cpp — LPAR-08 acceptance: Font glyph metrics over lv_font_*.
//
// See docs/core-draw/00-text-draw-image-mask.md §12.
//
// Scope: this test exercises the non-owning Font handle, which needs no
// draw context. The draw helpers (draw::fill_rect / draw::label) require a
// live lv_layer_t that only exists inside an LVGL draw event, so their
// runtime coverage is DEFERRED to the first custom-draw widget (Canvas /
// Scale / Chart, concepts §14). Here we only take their addresses to prove
// they compile and link.

#include "lvglpp/core/draw.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>

using lvglpp::Runtime;
using lvglpp::core::Font;

namespace {

void test_default_font() {
    const Font f = Font::default_font();
    assert(!f.empty());
    assert(f.borrow_raw() != nullptr);
}

void test_glyph_metrics() {
    const Font f = Font::default_font();
    // 'A' is present in every default font; advance and line height > 0.
    assert(f.glyph_advance(static_cast<std::uint32_t>('A')) > 0U);
    assert(f.line_height() > 0);
}

void test_empty_font_safe() {
    const Font empty{};  // observes nothing
    assert(empty.empty());
    assert(empty.borrow_raw() == nullptr);
    assert(empty.glyph_advance(static_cast<std::uint32_t>('A')) == 0U);
    assert(empty.line_height() == 0);

    // A Font built from an explicit nullptr is equivalent to empty.
    const Font null_font{static_cast<const lv_font_t*>(nullptr)};
    assert(null_font.empty());
    assert(null_font.glyph_advance(static_cast<std::uint32_t>('A')) == 0U);
    assert(null_font.line_height() == 0);
}

// Compile-time linkage proof for the draw helpers: taking the function
// addresses forces them to be emitted without needing a live layer.
void test_draw_helpers_link() {
    using lvglpp::core::draw::fill_rect;
    using lvglpp::core::draw::label;
    auto fill_ptr  = &fill_rect;
    auto label_ptr = &label;
    assert(fill_ptr != nullptr);
    assert(label_ptr != nullptr);
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    test_default_font();
    test_glyph_metrics();
    test_empty_font_safe();
    test_draw_helpers_link();
    return 0;
}
