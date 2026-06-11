// format.cpp — {name} placeholder substitution.
//
// PARITY: rlvgl/i18n/src/lib.rs:135–159 (v0.2.0 @ 79f730d) — t_format,
//         ported branch-for-branch: unknown placeholders are preserved
//         with braces, an unterminated '{' copies the remainder.
//
// docs/i18n/00-rltn-core.md §5.4.

#include "lvglpp/i18n/format.hpp"

namespace lvglpp::i18n {

std::string format(std::string_view tmpl, std::span<const Param> params) {
  std::string out;
  out.reserve(tmpl.size() + 16);

  std::string_view rest = tmpl;
  for (;;) {
    const std::size_t start = rest.find('{');
    if (start == std::string_view::npos) {
      break;
    }
    out.append(rest.substr(0, start));
    const std::string_view after = rest.substr(start + 1);
    const std::size_t end = after.find('}');
    if (end == std::string_view::npos) {
      // Unterminated placeholder: copy the rest verbatim
      // (lib.rs:153–156).
      out.append(rest.substr(start));
      return out;
    }
    const std::string_view name = after.substr(0, end);
    bool replaced = false;
    for (const Param &p : params) {
      if (p.name() == name) {
        out.append(p.value());
        replaced = true;
        break;
      }
    }
    if (!replaced) {
      // No matching parameter: keep the placeholder verbatim
      // (lib.rs:147–150).
      out.push_back('{');
      out.append(name);
      out.push_back('}');
    }
    rest = after.substr(end + 1);
  }
  out.append(rest);
  return out;
}

} // namespace lvglpp::i18n
