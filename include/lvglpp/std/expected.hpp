// expected.hpp — seam header that exposes lvglpp::expected<T, E>.
//
// PARITY: rlvgl Result<T, E> (Rust std::result::Result).
// LVGL:   N/A — LVGL C uses lv_result_t (a small enum). lvglpp's
//         expected wraps richer error payloads at the C++ boundary.
// DELTA:  Rust ?-operator is implicit; C++ uses LVGLPP_TRY (see
//         lvglpp/std/try.hpp once that lands).
//
// Selects the standard <expected> when available, falls back to the
// minimal vendored polyfill otherwise. Consumers of lvglpp must always
// write `lvglpp::expected<T, E>` and `lvglpp::unexpected<E>` — never
// reach into std:: or the polyfill namespace directly.

#ifndef LVGLPP_STD_EXPECTED_HPP
#define LVGLPP_STD_EXPECTED_HPP

#include <version>

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L

#include <expected>

namespace lvglpp {

template <class T, class E>
using expected = std::expected<T, E>;

template <class E>
using unexpected = std::unexpected<E>;

using std::unexpect_t;
using std::unexpect;

}  // namespace lvglpp

#else  // polyfill path

#include <lvglpp_expected.hpp>

namespace lvglpp {

template <class T, class E>
using expected = ::lvglpp::detail::expected<T, E>;

template <class E>
using unexpected = ::lvglpp::detail::unexpected<E>;

using ::lvglpp::detail::unexpect_t;
using ::lvglpp::detail::unexpect;

}  // namespace lvglpp

#endif  // <expected> selection

#endif  // LVGLPP_STD_EXPECTED_HPP
