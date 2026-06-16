// executor_test.cpp — PLAYIT-07 acceptance: end-to-end wire round-trip
// through a MemoryTransport. The Executor reads bytes, accumulates
// lines, dispatches via parse_command + Dispatcher, formats Response
// via format_response, writes back through the transport.

#include "lvglpp/playit/executor.hpp"

#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/playit/dispatcher.hpp"
#include "lvglpp/playit/transport.hpp"
#include "lvglpp/widgets/legacy/button.hpp"
#include "lvglpp/widgets/legacy/label.hpp"

#include <cassert>
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

// Test-only Transport: callers pre-fill in_bytes, then poll the
// Executor; written bytes accumulate in out_bytes.
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

struct Fixture {
    int                button_clicks = 0;
    lc::WidgetNode     root;

    Fixture() {
        auto root_filler = std::make_unique<lw::Label>(
            std::string{""}, lc::Rect{-1, -1, 0, 0});
        root = lc::WidgetNode{std::move(root_filler), "root"};

        auto label = std::make_unique<lw::Label>(
            std::string{"hi"}, lc::Rect{0, 0, 100, 20});
        root.add_child(lc::WidgetNode{std::move(label), "status"});

        auto btn = std::make_unique<lw::Button>(
            std::string{"OK"}, lc::Rect{10, 30, 80, 40});
        btn->set_on_click([this](lw::Button&) { ++button_clicks; });
        root.add_child(lc::WidgetNode{std::move(btn), "ok"});
    }
};

void test_executor_roundtrip_status() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
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
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("T@ok:50,50\n");
    assert(exec.poll() == 1);
    assert(fx.button_clicks == 1);
    assert(transport.drain_response() == "OK\r\n");
}

void test_executor_multiple_commands_one_poll() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("QE:status\nQE:nope\nQB:ok\n");
    assert(exec.poll() == 3);
    auto resp = transport.drain_response();
    assert(resp == "EXISTS:1\r\nEXISTS:0\r\nBOUNDS:10,30,80,40\r\n");
}

void test_executor_crlf_tolerance() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("QE:status\r\n");
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "EXISTS:1\r\n");
}

void test_executor_partial_line_accumulates_across_polls() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
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
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    transport.feed("\n");
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "ERR: empty command\r\n");
}

void test_executor_unknown_prefix_is_extension_error() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    // 'Z' falls through parse_command's switch into Extension(line)
    // — which the Dispatcher reports as "unhandled extension".
    transport.feed("Zhello\n");
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "ERR: unhandled extension\r\n");
}

void test_executor_long_line_overflow() {
    Fixture fx;
    lp::Dispatcher dispatcher{fx.root};
    MemoryTransport transport;
    lp::Executor exec{transport, dispatcher};

    // Construct a line longer than LINE_BUF_BYTES to trigger overflow.
    std::string long_line(lp::LINE_BUF_BYTES + 32, 'x');
    long_line += '\n';
    transport.feed(long_line);
    assert(exec.poll() == 1);
    assert(transport.drain_response() == "ERR: line too long\r\n");
}

}  // namespace

int main() {
    test_executor_roundtrip_status();
    test_executor_tagged_inject_drives_button();
    test_executor_multiple_commands_one_poll();
    test_executor_crlf_tolerance();
    test_executor_partial_line_accumulates_across_polls();
    test_executor_empty_line_yields_error();
    test_executor_unknown_prefix_is_extension_error();
    test_executor_long_line_overflow();
    return 0;
}
