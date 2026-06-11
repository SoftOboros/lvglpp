// format.hpp — {name} placeholder substitution for translation templates.
//
// PARITY: rlvgl/i18n/src/lib.rs:135–159 (v0.2.0 @ 79f730d) — t_format.
// LVGL:   N/A (lv_translation has no parameterization).
// DELTA:  parameter values are strings or integral types (rlvgl accepts
//         any core::fmt::Display); floating-point values must be
//         pre-formatted by the caller. docs/i18n/00-rltn-core.md §5.4.
#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>

namespace lvglpp::i18n {

// One named parameter for format(). Self-contained value type:
//   - string arguments: borrows; the referenced storage must outlive
//     the format() call (template-substitution scope only).
//   - integral arguments: owns; rendered into the inline buffer at
//     construction (std::to_chars, no heap).
class Param {
public:
  // Args:
  //   name:  borrows; placeholder name without braces.
  //   value: borrows for the duration of the format() call.
  constexpr Param(std::string_view name, std::string_view value) noexcept
      : name_(name), view_(value) {}
  constexpr Param(std::string_view name, const char *value) noexcept
      : name_(name), view_(value) {}
  Param(std::string_view name, bool value) noexcept
      : name_(name), view_(value ? "true" : "false") {}
  template <typename T>
    requires(std::integral<T> && !std::same_as<T, bool>)
  Param(std::string_view name, T value) noexcept : name_(name) {
    auto [end, ec] = std::to_chars(buf_, buf_ + sizeof(buf_), value);
    len_ = (ec == std::errc{}) ? static_cast<std::uint8_t>(end - buf_) : 0;
  }

  [[nodiscard]] constexpr std::string_view name() const noexcept {
    return name_;
  }
  [[nodiscard]] std::string_view value() const noexcept {
    return len_ != 0 ? std::string_view{buf_, len_} : view_;
  }

private:
  std::string_view name_; // borrows: placeholder name.
  std::string_view view_; // borrows: string argument storage.
  char buf_[24] = {};     // owns: numeric rendering.
  std::uint8_t len_ = 0;  // 0 → view_ is authoritative.
};

// Replace `{name}` placeholders in `tmpl` with matching parameter
// values, exactly per rlvgl t_format (lib.rs:135–159):
//   - a placeholder with no matching parameter is preserved verbatim
//     (including braces);
//   - an unterminated `{` copies the remainder of the template as-is.
// Args:
//   tmpl:   borrows for the duration of the call.
//   params: borrows for the duration of the call.
// Returns:
//   owns the formatted string.
[[nodiscard]] std::string format(std::string_view tmpl,
                                 std::span<const Param> params);

[[nodiscard]] inline std::string format(std::string_view tmpl,
                                        std::initializer_list<Param> params) {
  return format(tmpl, std::span<const Param>{params.begin(), params.size()});
}

} // namespace lvglpp::i18n
