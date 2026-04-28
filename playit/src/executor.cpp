// executor.cpp — line accumulator + dispatch loop.
//
// PARITY: rlvgl/playit/src/executor.rs (v0.2.0 @ 79f730d).

#include "lvglpp/playit/executor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

#include "lvglpp/playit/command.hpp"
#include "lvglpp/playit/event_spec.hpp"
#include "lvglpp/playit/format.hpp"
#include "lvglpp/playit/parser.hpp"
#include "lvglpp/playit/response.hpp"

namespace lvglpp::playit {

namespace {

inline constexpr std::string_view kErrEmpty   = "empty command";
inline constexpr std::string_view kErrParse   = "parse error";
inline constexpr std::string_view kErrTooLong = "line too long";

inline std::span<const std::uint8_t>
as_bytes(const char* p, std::size_t n) noexcept {
    return std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(p), n};
}

}  // namespace

void Executor::dispatch_line() noexcept {
    Response resp = [this]() -> Response {
        if (overflowed_) {
            return Response{response::Error{kErrTooLong}};
        }
        const std::string_view line{line_buf_.data(), line_len_};
        if (line.empty()) {
            return Response{response::Error{kErrEmpty}};
        }
        auto parsed = parse_command(line);
        if (!parsed.has_value()) {
            return Response{response::Error{kErrParse}};
        }

        // Recorder side-effects per PLAYIT-06 §5.4.
        if (recorder_ != nullptr) {
            // RS / RE / RD: handled at the Executor level. Bypass
            // the Dispatcher's "not implemented" arm and emit the
            // wire bytes directly. Returning a Response::Ok here
            // would conflict with the literal rlvgl reply line, so
            // we write the line ourselves and return a sentinel
            // empty-string Response::Error to skip the trailing
            // format_response call below.
            if (std::holds_alternative<command::RecordStart>(*parsed)) {
                recorder_->start();
                static constexpr std::string_view kRecRecording = "REC:recording\r\n";
                transport_->write_bytes(as_bytes(kRecRecording.data(),
                                                 kRecRecording.size()));
                return Response{response::Error{std::string_view{}}};
            }
            if (std::holds_alternative<command::RecordStop>(*parsed)) {
                recorder_->stop();
                dump_recording();
                return Response{response::Error{std::string_view{}}};
            }
            if (std::holds_alternative<command::RecordDump>(*parsed)) {
                dump_recording();
                return Response{response::Error{std::string_view{}}};
            }

            // Inject / InjectTagged: record the spec before dispatch.
            if (recorder_->running()) {
                if (const auto* inj = std::get_if<command::Inject>(&*parsed)) {
                    recorder_->record(inj->event);
                } else if (const auto* tag =
                               std::get_if<command::InjectTagged>(&*parsed)) {
                    recorder_->record(tag->event);
                }
            }
        }

        return dispatcher_->dispatch(*parsed);
    }();

    // Skip the formatter on the recorder-handled sentinel
    // (Error{empty string_view} marks "already wrote the wire bytes").
    if (const auto* err = std::get_if<response::Error>(&resp)) {
        if (err->reason.empty()) {
            line_len_   = 0;
            overflowed_ = false;
            return;
        }
    }

    std::array<char, RESPONSE_BUF_BYTES> out{};
    const std::size_t n = format_response(
        resp, std::span<char>{out.data(), out.size()});

    transport_->write_bytes(as_bytes(out.data(), n));

    line_len_   = 0;
    overflowed_ = false;
}

void Executor::dump_recording() noexcept {
    if (recorder_ == nullptr) {
        // Defensive: caller arm-checked before calling.
        return;
    }

    // Header: REC:START,<count>\r\n  (PLAYIT-06 §5.3)
    {
        std::array<char, 32> hdr{};
        std::size_t pos = 0;
        constexpr std::string_view kStart = "REC:START,";
        for (char c : kStart) {
            if (pos < hdr.size()) hdr[pos++] = c;
        }
        // uint16_t count → decimal.
        std::array<char, 6> digits{};
        std::size_t d = 0;
        std::uint32_t count = static_cast<std::uint32_t>(recorder_->size());
        if (count == 0) {
            if (pos < hdr.size()) hdr[pos++] = '0';
        } else {
            while (count > 0 && d < digits.size()) {
                digits[d++] = static_cast<char>('0' + (count % 10U));
                count /= 10U;
            }
            for (std::size_t i = d; i > 0; --i) {
                if (pos < hdr.size()) hdr[pos++] = digits[i - 1];
            }
        }
        if (pos + 2 <= hdr.size()) {
            hdr[pos++] = '\r';
            hdr[pos++] = '\n';
        }
        transport_->write_bytes(as_bytes(hdr.data(), pos));
    }

    // Per-entry lines: @<tick_delta> <event-line>\r\n
    // (PLAYIT-06a §5.5; supersedes PLAYIT-06 §5.3's seq form.)
    recorder_->for_each([this](const EventRecorder::Entry& entry) noexcept {
        std::array<char, 16> prefix{};
        std::size_t pos = 0;
        prefix[pos++] = '@';
        // tick_delta → decimal (uint16_t, up to 5 digits).
        std::array<char, 5> digits{};
        std::size_t d = 0;
        std::uint32_t v = static_cast<std::uint32_t>(entry.tick_delta);
        if (v == 0) {
            prefix[pos++] = '0';
        } else {
            while (v > 0 && d < digits.size()) {
                digits[d++] = static_cast<char>('0' + (v % 10U));
                v /= 10U;
            }
            for (std::size_t i = d; i > 0; --i) {
                if (pos < prefix.size()) prefix[pos++] = digits[i - 1];
            }
        }
        if (pos < prefix.size()) prefix[pos++] = ' ';
        transport_->write_bytes(as_bytes(prefix.data(), pos));

        std::array<char, 128> spec_buf{};
        const std::size_t sn = format_event_spec(
            entry.spec,
            std::span<char>{spec_buf.data(), spec_buf.size()});
        transport_->write_bytes(as_bytes(spec_buf.data(), sn));

        constexpr std::string_view kCrlf = "\r\n";
        transport_->write_bytes(as_bytes(kCrlf.data(), kCrlf.size()));
    });

    // Footer: REC:END\r\n
    constexpr std::string_view kEnd = "REC:END\r\n";
    transport_->write_bytes(as_bytes(kEnd.data(), kEnd.size()));
}

std::size_t Executor::poll() noexcept {
    std::size_t commands = 0;

    for (std::size_t consumed = 0; consumed < POLL_MAX_BYTES; ++consumed) {
        auto byte_opt = transport_->read_byte();
        if (!byte_opt.has_value()) {
            break;
        }
        const std::uint8_t b = *byte_opt;

        if (b == static_cast<std::uint8_t>('\n')) {
            if (line_len_ > 0 &&
                line_buf_[line_len_ - 1] == '\r') {
                --line_len_;
            }
            dispatch_line();
            ++commands;
            continue;
        }

        if (line_len_ < line_buf_.size()) {
            line_buf_[line_len_++] = static_cast<char>(b);
        } else {
            overflowed_ = true;
        }
    }
    return commands;
}

}  // namespace lvglpp::playit
