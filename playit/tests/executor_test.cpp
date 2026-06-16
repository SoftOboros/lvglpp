// executor_test.cpp — PLAYIT-07 acceptance, re-targeted onto lv_obj
// (LVGLPP-WRAP-0N). End-to-end wire round-trip through a MemoryTransport: the
// Executor reads bytes, accumulates lines, dispatches via parse_command +
// ObjDispatcher (lv_obj tree), formats Response via format_response, writes
// back. The dispatcher-agnostic Executor pumps ObjDispatcher unchanged.

#include "lvglpp/playit/executor.hpp"

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/playit/obj_dispatcher.hpp"
#include "lvglpp/playit/transport.hpp"
#include "lvglpp/widgets/button.hpp"
#include "lvglpp/widgets/label.hpp"

#include <cassert>
#include <cstdint>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lp = lvglpp::playit;
namespace lw = lvglpp::widgets;

namespace {

// Test-only Transport: callers pre-fill in_bytes, then poll the Executor;
// written bytes accumulate in out_bytes.
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
        for (char c : s) {
            in_bytes.push_back(static_cast<std::uint8_t>(c));
        }
    }

    std::string drain_response() {
        std::string s{out_bytes.begin(), out_bytes.end()};
        out_bytes.clear();
        return s;
    }
};

// lv_obj equivalent of the WidgetNode fixture:
//   screen [tag=root]
//   ├── status label [tag=status] {0,0,100,20}
//   └── ok button     [tag=ok]     {10,30,80,40}, clickable
struct Fixture {
    int        button_clicks = 0;
    lc::Screen screen = lc::Screen::make();
    lw::Label  status = lw::Label::make(screen.view());
    lw::Button ok     = lw::Button::make(screen.view());

    Fixture() {
        screen.set_tag("root");

        status.set_tag("status");
        status.set_text("hi");
        lv_obj_set_pos(status.borrow_raw(), 0, 0);
        status.set_size(100, 20);

        ok.set_tag("ok");
        lv_obj_set_pos(ok.borrow_raw(), 10, 30);
        ok.set_size(80, 40);
        ok.set_on_click([this] { ++button_clicks; });

        lv_obj_update_layout(screen.borrow_raw());
    }
};

void test_executor_roundtrip_status() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    dispatcher.set_status_snapshot(lp::StatusData{42, 7});
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("?\n");
    auto count = exec.poll();
    assert(count == 1);
    assert(transport.drain_response() == "STAT:42,7\r\n");
}

void test_executor_tagged_inject_drives_button() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("T@ok:50,50\n");
    assert(exec.poll() == 1);
    assert(fx.button_clicks == 1);
    assert(transport.drain_response() == "OK\r\n");
}

void test_executor_multiple_commands_one_poll() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("QE:status\nQE:nope\nQB:ok\n");
    assert(exec.poll() == 3);
    auto resp = transport.drain_response();
    assert(resp == "EXISTS:1\r\nEXISTS:0\r\nBOUNDS:10,30,80,40\r\n");
}

void test_executor_crlf_tolerance() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("QE:status\r\n");
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "EXISTS:1\r\n");
}

void test_executor_partial_line_accumulates_across_polls() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("QE:");
    assert(exec.poll() == 0);                       // no '\n' yet
    assert(transport.out_bytes.empty());
    transport.feed("status\n");
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "EXISTS:1\r\n");
}

void test_executor_empty_line_yields_error() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("\n");
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "ERR: empty command\r\n");
}

void test_executor_unknown_prefix_is_extension_error() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    // 'Z' falls through parse_command's switch into Extension(line)
    // — which the dispatcher reports as "unhandled extension".
    transport.feed("Zhello\n");
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "ERR: unhandled extension\r\n");
}

void test_executor_long_line_overflow() {
    Fixture fx;
    lp::ObjDispatcher dispatcher{fx.screen.view()};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    // Construct a line longer than LINE_BUF_BYTES to trigger overflow.
    std::string long_line(lp::LINE_BUF_BYTES + 32, 'x');
    long_line += '\n';
    transport.feed(long_line);
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "ERR: line too long\r\n");
}

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

}  // namespace

int main() {
    auto runtime = lvglpp::Runtime::try_make();
    assert(runtime.has_value());

    static std::uint8_t draw_buf[100 * 20 * 4];
    lv_display_t* disp = lv_display_create(100, 100);
    assert(disp != nullptr);
    lv_display_set_flush_cb(disp, noop_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr,
                           static_cast<std::uint32_t>(sizeof(draw_buf)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    test_executor_roundtrip_status();
    test_executor_tagged_inject_drives_button();
    test_executor_multiple_commands_one_poll();
    test_executor_crlf_tolerance();
    test_executor_partial_line_accumulates_across_polls();
    test_executor_empty_line_yields_error();
    test_executor_unknown_prefix_is_extension_error();
    test_executor_long_line_overflow();

    lv_display_delete(disp);
    return 0;
}
