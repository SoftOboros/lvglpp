// format_test.cpp — I18N-01 acceptance for format()/Param.
//
// PARITY: mirrors the substitution behavior exercised by rlvgl's
// `parameterized` / `parameterized_fr` tests (rlvgl/i18n/src/lib.rs:
// 185–205) plus the edge cases frozen in docs/i18n/00 §5.4 (unknown
// placeholder preserved; unterminated '{' copied once — see
// docs/i18n/rlvgl-t-format-unterminated-dup.md).

#include "lvglpp/i18n/format.hpp"

#include <cassert>
#include <string>

namespace li = lvglpp::i18n;

int main() {
  // Plain passthrough: no placeholders.
  assert(li::format("Plugins", {}) == "Plugins");

  // rlvgl `parameterized` vector: "Clicks: {count}", count = 42.
  assert(li::format("Clicks: {count}", {{"count", 42}}) == "Clicks: 42");

  // rlvgl `parameterized_fr` vector: "Clics : {count}", count = 7.
  assert(li::format("Clics : {count}", {{"count", 7}}) == "Clics : 7");

  // String parameter + multiple placeholders + repeated use.
  assert(li::format("{a}-{b}-{a}", {{"a", "x"}, {"b", "y"}}) == "x-y-x");

  // Mixed text around placeholders (demo.title shape).
  assert(li::format("rlvgl Demo v{version}", {{"version", "0.1.9"}}) ==
         "rlvgl Demo v0.1.9");

  // Unknown placeholder preserved verbatim with braces (lib.rs:147–150).
  assert(li::format("Touch: ({x}, {y})", {{"x", 12}}) == "Touch: (12, {y})");

  // Unterminated '{' copies the remainder exactly once — the
  // documented intent; rlvgl currently duplicates the tail here
  // (upstream finding).
  assert(li::format("abc{def", {}) == "abc{def");
  assert(li::format("abc{def", {{"def", 1}}) == "abc{def");

  // Empty placeholder name.
  assert(li::format("a{}b", {}) == "a{}b");

  // Negative and 64-bit integrals render via to_chars.
  assert(li::format("{n}", {{"n", -42}}) == "-42");
  assert(li::format("{n}", {{"n", std::int64_t{1} << 40}}) == "1099511627776");

  // bool renders as true/false (Display parity).
  assert(li::format("{b}", {{"b", true}}) == "true");

  return 0;
}
