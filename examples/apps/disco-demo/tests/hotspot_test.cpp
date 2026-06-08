// hotspot_test.cpp — DEMO-05 acceptance for ActionHotspot.
//
// Mirrors the rlvgl hotspot unit behavior: visibility-gated bounds (zero
// when the predicate is false), PressRelease fires the activation closure
// and is consumed, and a hidden hotspot ignores input.

#include "lvglpp/app/disco_demo/hotspot.hpp"

#include <cassert>
#include <cstdint>

namespace lc = lvglpp::core;
namespace ad = lvglpp::app::disco_demo;

namespace {

const lc::Rect kBounds{100, 100, 40, 40};

void test_default_visible_bounds_and_tap() {
    int taps = 0;
    ad::ActionHotspot h{kBounds, [&taps] { ++taps; }};
    assert(h.bounds() == kBounds);

    const lc::Event ev{lc::event::PressRelease{110, 110}};
    assert(h.handle_event(ev));
    assert(taps == 1);
}

void test_visibility_gated_bounds() {
    bool visible = false;
    ad::ActionHotspot h{kBounds, [] {}};
    h.with_visibility([&visible] { return visible; });

    assert(h.bounds() == (lc::Rect{0, 0, 0, 0}));  // predicate false -> zero
    visible = true;
    assert(h.bounds() == kBounds);
}

void test_hidden_ignores_tap() {
    int taps = 0;
    ad::ActionHotspot h{kBounds, [&taps] { ++taps; }};
    h.with_visibility([] { return false; });

    const lc::Event ev{lc::event::PressRelease{110, 110}};
    assert(!h.handle_event(ev));
    assert(taps == 0);
}

void test_non_pressrelease_ignored() {
    int taps = 0;
    ad::ActionHotspot h{kBounds, [&taps] { ++taps; }};
    const lc::Event ev{lc::event::PointerDown{110, 110}};
    assert(!h.handle_event(ev));
    assert(taps == 0);
}

}  // namespace

int main() {
    test_default_visible_bounds_and_tap();
    test_visibility_gated_bounds();
    test_hidden_ignores_tap();
    test_non_pressrelease_ignored();
    return 0;
}
