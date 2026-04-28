// conversion_test.cpp — round-trip every wire-format spec variant
// through to_event() / to_key() / to_core() into the lvglpp::core
// runtime types.
//
// This is the load-bearing acceptance test for CORE-02 §12 — every
// EventSpec variant must produce the corresponding Event variant
// with payload preserved. Drift here is a CORE-02 conformance bug.

#include "lvglpp/playit/playit.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <variant>

using namespace lvglpp::playit;
namespace lc = lvglpp::core;

namespace {

template <class Alt, class Variant>
const Alt* as(const Variant& v) noexcept {
    return std::get_if<Alt>(&v);
}

// -------------------------------------------------------------------
// TouchState round-trip
// -------------------------------------------------------------------

void test_touch_state_to_core() {
    assert(to_core(TouchStateSpec::Down)    == lc::TouchState::Down);
    assert(to_core(TouchStateSpec::Up)      == lc::TouchState::Up);
    assert(to_core(TouchStateSpec::Contact) == lc::TouchState::Contact);
}

// -------------------------------------------------------------------
// TouchPoint round-trip
// -------------------------------------------------------------------

void test_touch_point_to_core() {
    TouchPointSpec spec{
        /*id=*/3,
        TouchStateSpec::Contact,
        /*x=*/100,
        /*y=*/200,
    };
    lc::TouchPoint core = to_core(spec);
    assert(core.id    == 3);
    assert(core.x     == 100);
    assert(core.y     == 200);
    assert(core.state == lc::TouchState::Contact);
}

// -------------------------------------------------------------------
// Key round-trip — every KeySpec::Kind must produce the matching
// lvglpp::core::Key variant.
// -------------------------------------------------------------------

void test_key_named_variants() {
    assert(as<lc::key::Escape>     (to_key(KeySpec{KeySpec::Kind::Escape,     0})));
    assert(as<lc::key::Enter>      (to_key(KeySpec{KeySpec::Kind::Enter,      0})));
    assert(as<lc::key::Space>      (to_key(KeySpec{KeySpec::Kind::Space,      0})));
    assert(as<lc::key::ArrowUp>    (to_key(KeySpec{KeySpec::Kind::ArrowUp,    0})));
    assert(as<lc::key::ArrowDown>  (to_key(KeySpec{KeySpec::Kind::ArrowDown,  0})));
    assert(as<lc::key::ArrowLeft>  (to_key(KeySpec{KeySpec::Kind::ArrowLeft,  0})));
    assert(as<lc::key::ArrowRight> (to_key(KeySpec{KeySpec::Kind::ArrowRight, 0})));
}

void test_key_function() {
    auto core = to_key(KeySpec{KeySpec::Kind::Function, 7});
    auto* fn  = as<lc::key::Function>(core);
    assert(fn);
    assert(fn->n == 7);
}

void test_key_character() {
    auto core = to_key(KeySpec{KeySpec::Kind::Character, 0x41});  // 'A'
    auto* ch  = as<lc::key::Character>(core);
    assert(ch);
    assert(ch->codepoint == 0x41U);
}

void test_key_other() {
    auto core = to_key(KeySpec{KeySpec::Kind::Other, 0xDEADBEEF});
    auto* o   = as<lc::key::Other>(core);
    assert(o);
    assert(o->code == 0xDEADBEEFU);
}

// -------------------------------------------------------------------
// Event round-trip — every variant in concepts doc §5.1.
// -------------------------------------------------------------------

void test_event_tick() {
    auto core = to_event(EventSpec{event_spec::Tick{}});
    assert(as<lc::event::Tick>(core) != nullptr);
}

void test_event_pointer_family() {
    {
        auto core = to_event(EventSpec{event_spec::PointerDown{1, 2}});
        auto* p   = as<lc::event::PointerDown>(core);
        assert(p && p->x == 1 && p->y == 2);
    }
    {
        auto core = to_event(EventSpec{event_spec::PointerUp{3, 4}});
        auto* p   = as<lc::event::PointerUp>(core);
        assert(p && p->x == 3 && p->y == 4);
    }
    {
        auto core = to_event(EventSpec{event_spec::PointerMove{5, 6}});
        auto* p   = as<lc::event::PointerMove>(core);
        assert(p && p->x == 5 && p->y == 6);
    }
}

void test_event_press_family() {
    {
        auto core = to_event(EventSpec{event_spec::PressDown{10, 20}});
        auto* p   = as<lc::event::PressDown>(core);
        assert(p && p->x == 10 && p->y == 20);
    }
    {
        auto core = to_event(EventSpec{event_spec::PressRelease{30, 40}});
        auto* p   = as<lc::event::PressRelease>(core);
        assert(p && p->x == 30 && p->y == 40);
    }
    {
        auto core = to_event(EventSpec{event_spec::DoubleTap{50, 60}});
        auto* p   = as<lc::event::DoubleTap>(core);
        assert(p && p->x == 50 && p->y == 60);
    }
}

void test_event_key_family() {
    {
        EventSpec spec{event_spec::KeyDown{KeySpec{KeySpec::Kind::Enter, 0}}};
        auto core = to_event(spec);
        auto* kd  = as<lc::event::KeyDown>(core);
        assert(kd);
        assert(as<lc::key::Enter>(kd->key) != nullptr);
    }
    {
        EventSpec spec{event_spec::KeyUp{KeySpec{KeySpec::Kind::Function, 4}}};
        auto core = to_event(spec);
        auto* ku  = as<lc::event::KeyUp>(core);
        assert(ku);
        auto* fn  = as<lc::key::Function>(ku->key);
        assert(fn && fn->n == 4);
    }
}

void test_event_touch() {
    event_spec::Touch frame{};
    frame.count     = 3;
    frame.points[0] = TouchPointSpec{0, TouchStateSpec::Down,    10, 20};
    frame.points[1] = TouchPointSpec{1, TouchStateSpec::Contact, 30, 40};
    frame.points[2] = TouchPointSpec{2, TouchStateSpec::Up,      50, 60};
    // points[3..MAX_TOUCH_POINTS) are default (state=Up, all zero) and
    // must propagate without modification.

    auto core = to_event(EventSpec{frame});
    auto* t   = as<lc::event::Touch>(core);
    assert(t);
    assert(t->count == 3);

    assert(t->points[0].id    == 0);
    assert(t->points[0].x     == 10);
    assert(t->points[0].y     == 20);
    assert(t->points[0].state == lc::TouchState::Down);

    assert(t->points[1].id    == 1);
    assert(t->points[1].state == lc::TouchState::Contact);

    assert(t->points[2].state == lc::TouchState::Up);

    // Trailing slots: zero-initialised TouchPoints, state==Up.
    assert(t->points[3].id    == 0);
    assert(t->points[3].x     == 0);
    assert(t->points[3].y     == 0);
    assert(t->points[3].state == lc::TouchState::Up);
    assert(t->points[4].state == lc::TouchState::Up);
}

// -------------------------------------------------------------------
// Static / structural sanity
// -------------------------------------------------------------------

void test_static_invariants() {
    static_assert(lc::MAX_TOUCH_POINTS == 5,
        "concepts doc §5.4 freezes MAX_TOUCH_POINTS at 5");
    static_assert(std::variant_size_v<lc::Event> == 10,
        "concepts doc §5.1 freezes the Event variant set at 10");
    static_assert(std::variant_size_v<lc::Key> == 10,
        "concepts doc §5.3 freezes the Key variant set at 10");
}

}  // namespace

int main() {
    test_static_invariants();
    test_touch_state_to_core();
    test_touch_point_to_core();
    test_key_named_variants();
    test_key_function();
    test_key_character();
    test_key_other();
    test_event_tick();
    test_event_pointer_family();
    test_event_press_family();
    test_event_key_family();
    test_event_touch();
    return 0;
}
