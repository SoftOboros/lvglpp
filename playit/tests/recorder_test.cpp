// recorder_test.cpp — PLAYIT-06 acceptance: EventRecorder ring +
// Executor RS/RE/RD wiring + dump wire format.

#include "lvglpp/playit/playit.hpp"

#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/widgets/legacy/button.hpp"
#include "lvglpp/widgets/legacy/label.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lp = lvglpp::playit;
namespace lw = lvglpp::widgets::legacy;

namespace {

struct MemoryTransport final : lp::Transport {
    std::deque<std::uint8_t> in_bytes;
    std::vector<std::uint8_t> out_bytes;

    [[nodiscard]] std::optional<std::uint8_t> read_byte() noexcept override {
        if (in_bytes.empty()) return std::nullopt;
        std::uint8_t b = in_bytes.front();
        in_bytes.pop_front();
        return b;
    }
    void write_bytes(std::span<const std::uint8_t> bytes) noexcept override {
        out_bytes.insert(out_bytes.end(), bytes.begin(), bytes.end());
    }
    void feed(std::string_view s) {
        for (char c : s) in_bytes.push_back(static_cast<std::uint8_t>(c));
    }
    std::string drain() {
        std::string s{out_bytes.begin(), out_bytes.end()};
        out_bytes.clear();
        return s;
    }
};

struct Fixture {
    lc::WidgetNode root;
    int            clicks = 0;
    Fixture() {
        auto root_w = std::make_unique<lw::Label>(std::string{""},
                                                   lc::Rect{-1, -1, 0, 0});
        root = lc::WidgetNode{std::move(root_w), "root"};

        auto btn = std::make_unique<lw::Button>(std::string{"OK"},
                                                 lc::Rect{0, 0, 100, 50});
        btn->set_on_click([this](lw::Button&) { ++clicks; });
        root.add_child(lc::WidgetNode{std::move(btn), "ok"});
    }
};

// ---- format_event_spec parity ---------------------------------------

std::string fmt(const lp::EventSpec& spec) {
    std::array<char, 128> buf{};
    std::size_t n = lp::format_event_spec(spec,
        std::span<char>{buf.data(), buf.size()});
    return std::string{buf.data(), n};
}

void test_format_event_spec_variants() {
    using namespace lp;
    assert(fmt(EventSpec{event_spec::Tick{}})                          == "TK");
    assert(fmt(EventSpec{event_spec::PressRelease{100, 200}})          == "T100,200");
    assert(fmt(EventSpec{event_spec::PressDown{42, 84}})               == "TD42,84");
    assert(fmt(EventSpec{event_spec::DoubleTap{70, 80}})               == "TT70,80");
    assert(fmt(EventSpec{event_spec::PointerDown{10, 20}})             == "PD10,20");
    assert(fmt(EventSpec{event_spec::PointerUp{30, 40}})               == "PU30,40");
    assert(fmt(EventSpec{event_spec::PointerMove{15, 25}})             == "PM15,25");
    assert(fmt(EventSpec{event_spec::KeyDown{KeySpec{KeySpec::Kind::Enter, 0}}}) == "KD:Enter");
    assert(fmt(EventSpec{event_spec::KeyUp{KeySpec{KeySpec::Kind::Function, 5}}}) == "KU:F5");
    assert(fmt(EventSpec{event_spec::KeyDown{KeySpec{KeySpec::Kind::Character, 'a'}}}) == "KD:a");
    assert(fmt(EventSpec{event_spec::KeyDown{KeySpec{KeySpec::Kind::Other, 12345}}}) == "KD:12345");
}

void test_format_event_spec_touch_frame() {
    lp::event_spec::Touch touch{};
    touch.count     = 2;
    touch.points[0] = lp::TouchPointSpec{0, lp::TouchStateSpec::Down,    10, 20};
    touch.points[1] = lp::TouchPointSpec{1, lp::TouchStateSpec::Contact, 30, 40};
    assert(fmt(lp::EventSpec{touch}) == "MT2:0,D,10,20;1,C,30,40");
}

// ---- EventRecorder shape (PLAYIT-06a) -------------------------------

void test_recorder_idle_drops_records() {
    lp::EventRecorder r;
    r.record(lp::EventSpec{lp::event_spec::Tick{}});
    assert(r.size() == 0);
}

void test_recorder_running_captures() {
    lp::EventRecorder r;
    r.start();
    assert(r.running());
    r.record(lp::EventSpec{lp::event_spec::PressRelease{1, 2}});
    r.record(lp::EventSpec{lp::event_spec::PressRelease{3, 4}});
    assert(r.size() == 2);
    int seen = 0;
    r.for_each([&](const auto& e) {
        // No tick() between records → both deltas are 0.
        assert(e.tick_delta == 0);
        ++seen;
    });
    assert(seen == 2);
}

void test_recorder_stop_keeps_buffer_for_dump() {
    lp::EventRecorder r;
    r.start();
    r.record(lp::EventSpec{lp::event_spec::Tick{}});
    r.stop();
    assert(!r.running());
    // Stopping does NOT clear; rlvgl drains during dump.
    assert(r.size() == 1);
}

// PLAYIT-06a §5.4 — fill-and-stop replaces ring overwrite.
// Mirrors rlvgl/playit/src/recorder.rs:236.
void test_recorder_fill_and_stop() {
    lp::EventRecorder r;
    r.start();
    for (std::uint32_t i = 0; i < lp::EventRecorder::CAPACITY + 5U; ++i) {
        r.record(lp::EventSpec{lp::event_spec::PressRelease{
            static_cast<std::int32_t>(i), 0}});
    }
    // Buffer filled exactly to CAPACITY; recorder auto-stopped.
    assert(r.size()  == lp::EventRecorder::CAPACITY);
    assert(r.is_full());
    assert(!r.running());

    // Earliest entries preserved (no ring overwrite). The first
    // entry's spec must be the very first one recorded.
    bool first = true;
    r.for_each([&](const auto& e) {
        if (first) {
            const auto* pr = std::get_if<lp::event_spec::PressRelease>(&e.spec);
            assert(pr != nullptr);
            assert(pr->x == 0);
            first = false;
        }
    });
    assert(!first);  // for_each ran at least once.
}

// PLAYIT-06a §5.3 — tick() between records produces the expected
// per-entry delta. Mirrors rlvgl/playit/src/recorder.rs:191.
void test_recorder_tick_delta_basic() {
    lp::EventRecorder r;
    r.start();

    // Frame 0: record an event.
    r.record(lp::EventSpec{lp::event_spec::PointerDown{10, 20}});
    // Advance 3 ticks.
    r.tick(); r.tick(); r.tick();
    // Frame 3: another event.
    r.record(lp::EventSpec{lp::event_spec::PointerMove{15, 25}});
    // Advance 2 more ticks.
    r.tick(); r.tick();
    r.record(lp::EventSpec{lp::event_spec::PointerUp{20, 30}});

    r.stop();
    assert(r.size() == 3);

    std::vector<std::uint16_t> deltas;
    r.for_each([&](const auto& e) { deltas.push_back(e.tick_delta); });
    assert(deltas.size() == 3);
    assert(deltas[0] == 0);
    assert(deltas[1] == 3);
    assert(deltas[2] == 2);
}

// PLAYIT-06a §5.3 — saturating to UINT16_MAX. Mirrors
// rlvgl/playit/src/recorder.rs:275.
void test_recorder_tick_delta_saturates() {
    lp::EventRecorder r;
    r.start();
    r.record(lp::EventSpec{lp::event_spec::Tick{}});
    for (std::uint32_t i = 0; i < 70'000U; ++i) {
        r.tick();
    }
    r.record(lp::EventSpec{lp::event_spec::Tick{}});

    std::vector<std::uint16_t> deltas;
    r.for_each([&](const auto& e) { deltas.push_back(e.tick_delta); });
    assert(deltas.size() == 2);
    assert(deltas[0] == 0);
    assert(deltas[1] == UINT16_MAX);
}

// PLAYIT-06a §5.2 — tick() while stopped is a no-op.
void test_recorder_tick_while_stopped_is_noop() {
    lp::EventRecorder r;
    for (int i = 0; i < 100; ++i) r.tick();  // recorder is stopped
    r.start();
    r.record(lp::EventSpec{lp::event_spec::Tick{}});
    // First record after start has delta 0 — pre-start ticks did
    // not contribute.
    bool checked = false;
    r.for_each([&](const auto& e) {
        if (!checked) {
            assert(e.tick_delta == 0);
            checked = true;
        }
    });
    assert(checked);
}

// ---- Executor RS / RD wiring ----------------------------------------

void test_executor_rs_emits_recording_line() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport tx;
    lp::EventRecorder rec;
    lp::Executor exec{tx, dispatcher};
    exec.set_recorder(&rec);

    tx.feed("RS\n");
    assert(exec.poll() == 1);
    assert(rec.running());
    assert(tx.drain() == "REC:recording\r\n");
}

void test_executor_records_inject_then_dumps() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport tx;
    lp::EventRecorder rec;
    lp::Executor exec{tx, dispatcher};
    exec.set_recorder(&rec);

    tx.feed("RS\nT@ok:50,25\nT100,200\nRD\n");
    while (exec.poll() > 0) {}

    const std::string out = tx.drain();
    // Expected sequence:
    //   REC:recording\r\n        (from RS)
    //   OK\r\n                   (from InjectTagged)
    //   OK\r\n                   (from Inject)
    //   REC:START,2\r\n
    //   @0 T50,25\r\n
    //   @0 T100,200\r\n          (no recorder.tick() between the
    //                             two records → both deltas = 0)
    //   REC:END\r\n
    const std::string expected =
        "REC:recording\r\n"
        "OK\r\n"
        "OK\r\n"
        "REC:START,2\r\n"
        "@0 T50,25\r\n"
        "@0 T100,200\r\n"
        "REC:END\r\n";
    assert(out == expected);

    // The button click fired during recording.
    assert(fx.clicks == 1);
}

// PLAYIT-06a end-to-end: tick-delta values come out correctly when
// recorder.tick() runs between Inject commands.
void test_executor_dump_with_tick_advance() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport tx;
    lp::EventRecorder rec;
    lp::Executor exec{tx, dispatcher};
    exec.set_recorder(&rec);

    // RS, then alternate: tap, tick, tick, tick, tap, tick, tick, tap, RD.
    tx.feed("RS\nT@ok:50,25\n");
    while (exec.poll() > 0) {}
    rec.tick(); rec.tick(); rec.tick();
    tx.feed("T@ok:60,30\n");
    while (exec.poll() > 0) {}
    rec.tick(); rec.tick();
    tx.feed("T@ok:70,35\nRD\n");
    while (exec.poll() > 0) {}

    const std::string out = tx.drain();
    const std::string expected =
        "REC:recording\r\n"
        "OK\r\n"
        "OK\r\n"
        "OK\r\n"
        "REC:START,3\r\n"
        "@0 T50,25\r\n"
        "@3 T60,30\r\n"
        "@2 T70,35\r\n"
        "REC:END\r\n";
    assert(out == expected);
}

void test_executor_re_dumps_then_stops() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport tx;
    lp::EventRecorder rec;
    lp::Executor exec{tx, dispatcher};
    exec.set_recorder(&rec);

    tx.feed("RS\nTK\nRE\n");
    while (exec.poll() > 0) {}

    const std::string out = tx.drain();
    const std::string expected =
        "REC:recording\r\n"
        "OK\r\n"                  // Inject(Tick) dispatch
        "REC:START,1\r\n"
        "@0 TK\r\n"
        "REC:END\r\n";
    assert(out == expected);
    assert(!rec.running());
}

void test_executor_without_recorder_falls_through() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport tx;
    lp::Executor exec{tx, dispatcher};  // no recorder attached

    tx.feed("RS\n");
    assert(exec.poll() == 1);
    // Falls through to Dispatcher: Error{not implemented}.
    assert(tx.drain() == "ERR: not implemented\r\n");
}

}  // namespace

int main() {
    test_format_event_spec_variants();
    test_format_event_spec_touch_frame();
    test_recorder_idle_drops_records();
    test_recorder_running_captures();
    test_recorder_stop_keeps_buffer_for_dump();
    test_recorder_fill_and_stop();
    test_recorder_tick_delta_basic();
    test_recorder_tick_delta_saturates();
    test_recorder_tick_while_stopped_is_noop();
    test_executor_rs_emits_recording_line();
    test_executor_records_inject_then_dumps();
    test_executor_dump_with_tick_advance();
    test_executor_re_dumps_then_stops();
    test_executor_without_recorder_falls_through();
    return 0;
}
