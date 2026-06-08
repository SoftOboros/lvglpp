// text_wrap.hpp — greedy word-wrap helper for DashboardPanel.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/dashboard_panel.rs:196
//         (wrap_text) (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite text layout).
// DELTA:  byte-wise port of the rlvgl char-wise wrapper (BitmapFont is
//         ASCII-only, so byte count == char count). Pulled into a header so
//         it is unit-testable exactly like the rlvgl `wrap_text` tests.
//
// docs/disco-demo/05-composite-widgets.md (DEMO-05).

#ifndef LVGLPP_APP_DISCO_DEMO_DETAIL_TEXT_WRAP_HPP
#define LVGLPP_APP_DISCO_DEMO_DETAIL_TEXT_WRAP_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lvglpp::app::disco_demo::detail {

inline bool wrap_is_space(char c) noexcept {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

// Greedy word-wrap to a maximum column count (characters). Explicit '\n'
// starts a new line; words longer than max_cols break at column boundaries;
// blank paragraphs are preserved. Mirrors dashboard_panel.rs:196.
inline std::vector<std::string> wrap_text(std::string_view text,
                                          std::size_t max_cols) {
    std::vector<std::string> out;
    if (max_cols == 0) {
        out.emplace_back();
        return out;
    }
    std::size_t start = 0;
    while (true) {
        const std::size_t nl  = text.find('\n', start);
        const std::size_t end = (nl == std::string_view::npos) ? text.size() : nl;
        const std::string_view paragraph = text.substr(start, end - start);
        const std::size_t start_len = out.size();
        std::string current;

        std::size_t i = 0;
        while (i < paragraph.size()) {
            while (i < paragraph.size() && wrap_is_space(paragraph[i])) ++i;
            const std::size_t ws = i;
            while (i < paragraph.size() && !wrap_is_space(paragraph[i])) ++i;
            if (ws == i) break;
            const std::string_view word = paragraph.substr(ws, i - ws);

            if (word.size() > max_cols) {
                if (!current.empty()) {
                    out.push_back(std::move(current));
                    current.clear();
                }
                for (std::size_t c = 0; c < word.size(); c += max_cols) {
                    out.emplace_back(
                        word.substr(c, std::min(max_cols, word.size() - c)));
                }
                continue;
            }
            const std::size_t extra = current.empty() ? 0U : 1U;
            if (current.size() + extra + word.size() > max_cols) {
                out.push_back(std::move(current));
                current.clear();
            }
            if (!current.empty()) current.push_back(' ');
            current.append(word);
        }

        if (!current.empty()) {
            out.push_back(std::move(current));
        } else if (out.size() == start_len) {
            out.emplace_back();
        }

        if (nl == std::string_view::npos) break;
        start = nl + 1;
    }
    return out;
}

// Join wrapped lines with '\n'. Mirrors the `.join("\n")` in set_caption.
inline std::string join_newline(const std::vector<std::string>& lines) {
    std::string joined;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) joined.push_back('\n');
        joined.append(lines[i]);
    }
    return joined;
}

}  // namespace lvglpp::app::disco_demo::detail

#endif  // LVGLPP_APP_DISCO_DEMO_DETAIL_TEXT_WRAP_HPP
