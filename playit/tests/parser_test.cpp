// parser_test.cpp — parity fixtures against rlvgl/playit/src/protocol.rs.
//
// Every assertion below has a corresponding rlvgl behaviour at
// rlvgl/playit/src/protocol.rs (v0.2.0 @ 79f730d). Drift between
// these implementations is a bug — playit is the cross-language test
// harness and the wire format is the contract.

#include "lvglpp/playit/playit.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>

using namespace lvglpp::playit;

namespace {

// Alt-first so call sites have a single template argument and don't
// trip the assert() macro on a comma between template args.
template <class Alt, class Variant>
const Alt* as(const Variant& v) noexcept {
    return std::get_if<Alt>(&v);
}

// -------------------------------------------------------------------
// Status / record commands
// -------------------------------------------------------------------

void test_status() {
    auto cmd = parse_command("?");
    assert(cmd.has_value());
    assert(as<command::Status>(*cmd) != nullptr);
}

void test_record_commands() {
    {
        auto cmd = parse_command("RS");
        assert(cmd && as<command::RecordStart>(*cmd));
    }
    {
        auto cmd = parse_command("RE");
        assert(cmd && as<command::RecordStop>(*cmd));
    }
    {
        auto cmd = parse_command("RD");
        assert(cmd && as<command::RecordDump>(*cmd));
    }
    {
        // Lowercase aliases are accepted in rlvgl.
        auto cmd = parse_command("rs");
        assert(cmd && as<command::RecordStart>(*cmd));
    }
    {
        auto cmd = parse_command("RX");
        // Unknown second byte for R: rlvgl returns None. Per
        // rlvgl/playit/src/protocol.rs:340 this falls through the
        // "RS/RE/RD" match arm and returns None.
        assert(!cmd.has_value());
    }
}

// -------------------------------------------------------------------
// Tap / press injection (T family)
// -------------------------------------------------------------------

void test_press_release_bare() {
    auto cmd = parse_command("T100,200");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    assert(inject != nullptr);
    auto* pr = std::get_if<event_spec::PressRelease>(&inject->event);
    assert(pr != nullptr);
    assert(pr->x == 100);
    assert(pr->y == 200);
}

void test_press_release_negative() {
    auto cmd = parse_command("T-5,-7");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    assert(inject);
    auto* pr = std::get_if<event_spec::PressRelease>(&inject->event);
    assert(pr && pr->x == -5 && pr->y == -7);
}

void test_tick() {
    auto cmd = parse_command("TK");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    assert(inject);
    assert(std::holds_alternative<event_spec::Tick>(inject->event));
}

void test_press_down() {
    auto cmd = parse_command("TD42,84");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    auto* pd = std::get_if<event_spec::PressDown>(&inject->event);
    assert(pd && pd->x == 42 && pd->y == 84);
}

void test_double_tap() {
    auto cmd = parse_command("TT70,80");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    auto* dt = std::get_if<event_spec::DoubleTap>(&inject->event);
    assert(dt && dt->x == 70 && dt->y == 80);
}

void test_tagged_press() {
    auto cmd = parse_command("T@SettingsButton:100,200");
    assert(cmd.has_value());
    auto* tagged = as<command::InjectTagged>(*cmd);
    assert(tagged);
    assert(tagged->tag == "SettingsButton");
    auto* pr = std::get_if<event_spec::PressRelease>(&tagged->event);
    assert(pr && pr->x == 100 && pr->y == 200);
}

// -------------------------------------------------------------------
// Pointer family
// -------------------------------------------------------------------

void test_pointer_down_up_move() {
    {
        auto cmd = parse_command("PD10,20");
        auto* inject = as<command::Inject>(*cmd);
        auto* p = std::get_if<event_spec::PointerDown>(&inject->event);
        assert(p && p->x == 10 && p->y == 20);
    }
    {
        auto cmd = parse_command("PU30,40");
        auto* inject = as<command::Inject>(*cmd);
        auto* p = std::get_if<event_spec::PointerUp>(&inject->event);
        assert(p && p->x == 30 && p->y == 40);
    }
    {
        auto cmd = parse_command("PM15,25");
        auto* inject = as<command::Inject>(*cmd);
        auto* p = std::get_if<event_spec::PointerMove>(&inject->event);
        assert(p && p->x == 15 && p->y == 25);
    }
}

// -------------------------------------------------------------------
// Keyboard family
// -------------------------------------------------------------------

void test_key_named() {
    auto cmd = parse_command("KD:Enter");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    auto* kd = std::get_if<event_spec::KeyDown>(&inject->event);
    assert(kd);
    assert(kd->key.kind == KeySpec::Kind::Enter);
}

void test_key_alias() {
    auto cmd = parse_command("KU:Esc");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    auto* ku = std::get_if<event_spec::KeyUp>(&inject->event);
    assert(ku);
    assert(ku->key.kind == KeySpec::Kind::Escape);
}

void test_key_function() {
    auto cmd = parse_command("KD:F5");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    auto* kd = std::get_if<event_spec::KeyDown>(&inject->event);
    assert(kd);
    assert(kd->key.kind == KeySpec::Kind::Function);
    assert(kd->key.value == 5);
}

void test_key_character() {
    auto cmd = parse_command("KD:a");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    auto* kd = std::get_if<event_spec::KeyDown>(&inject->event);
    assert(kd);
    assert(kd->key.kind == KeySpec::Kind::Character);
    assert(kd->key.value == static_cast<std::uint32_t>('a'));
}

// -------------------------------------------------------------------
// Query family
// -------------------------------------------------------------------

void test_query_family() {
    {
        auto cmd = parse_command("QB:foo");
        auto* q = as<command::QueryBounds>(*cmd);
        assert(q && q->tag == "foo");
    }
    {
        auto cmd = parse_command("QE:foo");
        auto* q = as<command::QueryExists>(*cmd);
        assert(q && q->tag == "foo");
    }
    {
        auto cmd = parse_command("QC:foo");
        auto* q = as<command::QueryChildCount>(*cmd);
        assert(q && q->tag == "foo");
    }
}

// -------------------------------------------------------------------
// Dump
// -------------------------------------------------------------------

void test_dump_default_frames() {
    auto cmd = parse_command("D10,20,30,40");
    assert(cmd.has_value());
    auto* d = as<command::DumpPixels>(*cmd);
    assert(d);
    assert(d->spec.x == 10);
    assert(d->spec.y == 20);
    assert(d->spec.width == 30);
    assert(d->spec.height == 40);
    assert(d->spec.frames == 1);
}

void test_dump_explicit_frames() {
    auto cmd = parse_command("D10,20,30,40,2");
    auto* d = as<command::DumpPixels>(*cmd);
    assert(d && d->spec.frames == 2);
}

void test_dump_clamps() {
    // width/height clamp to 1..=40, frames clamp to 1..=4 per
    // rlvgl/playit/src/protocol.rs:240.
    auto cmd = parse_command("D0,0,100,100,99");
    auto* d = as<command::DumpPixels>(*cmd);
    assert(d);
    assert(d->spec.width == 40);
    assert(d->spec.height == 40);
    assert(d->spec.frames == 4);
}

// -------------------------------------------------------------------
// Multi-touch
// -------------------------------------------------------------------

void test_mt_two_points() {
    auto cmd = parse_command("MT2:0,D,10,20;1,U,30,40");
    assert(cmd.has_value());
    auto* inject = as<command::Inject>(*cmd);
    assert(inject);
    auto* touch = std::get_if<event_spec::Touch>(&inject->event);
    assert(touch);
    assert(touch->count == 2);

    assert(touch->points[0].id    == 0);
    assert(touch->points[0].state == TouchStateSpec::Down);
    assert(touch->points[0].x     == 10);
    assert(touch->points[0].y     == 20);

    assert(touch->points[1].id    == 1);
    assert(touch->points[1].state == TouchStateSpec::Up);
    assert(touch->points[1].x     == 30);
    assert(touch->points[1].y     == 40);
}

void test_mt_count_out_of_range() {
    // count > MAX_TOUCH_POINTS rejected.
    auto cmd = parse_command("MT9:0,D,0,0");
    assert(!cmd.has_value());
}

// -------------------------------------------------------------------
// Extension / unknown prefix
// -------------------------------------------------------------------

void test_extension_explicit() {
    auto cmd = parse_command("Xpayload-bytes");
    auto* ext = as<command::Extension>(*cmd);
    assert(ext);
    assert(ext->payload == "payload-bytes");
}

void test_extension_unknown_prefix() {
    // rlvgl passes unknown-prefix lines through as Extension(line).
    auto cmd = parse_command("Zhello");
    auto* ext = as<command::Extension>(*cmd);
    assert(ext);
    assert(ext->payload == "Zhello");
}

void test_empty_input() {
    auto cmd = parse_command("");
    assert(!cmd.has_value());
}

// -------------------------------------------------------------------
// Malformed
// -------------------------------------------------------------------

void test_malformed() {
    // T followed by a non-coordinate-starting byte.
    assert(!parse_command("Tx").has_value());
    // PD with no digits.
    assert(!parse_command("PD").has_value());
    // KD without colon.
    assert(!parse_command("KDEnter").has_value());
    // QC without tag.
    assert(parse_command("QC:") == Command{command::QueryChildCount{""}});
}

}  // namespace

int main() {
    test_status();
    test_record_commands();
    test_press_release_bare();
    test_press_release_negative();
    test_tick();
    test_press_down();
    test_double_tap();
    test_tagged_press();
    test_pointer_down_up_move();
    test_key_named();
    test_key_alias();
    test_key_function();
    test_key_character();
    test_query_family();
    test_dump_default_frames();
    test_dump_explicit_frames();
    test_dump_clamps();
    test_mt_two_points();
    test_mt_count_out_of_range();
    test_extension_explicit();
    test_extension_unknown_prefix();
    test_empty_input();
    test_malformed();
    return 0;
}
