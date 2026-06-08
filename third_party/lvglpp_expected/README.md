# lvglpp_expected — minimal `std::expected<T, E>` polyfill

This directory holds a small, self-contained polyfill used by lvglpp until
all supported toolchains ship `<expected>` (C++23, `__cpp_lib_expected >=
202202L`).

The seam header `include/lvglpp/std/expected.hpp` selects between the
standard implementation and this polyfill at compile time, exposing the
single-name surface `lvglpp::expected<T, E>` and `lvglpp::unexpected<E>`.

## Why we vendor this

- AppleClang 14, gcc < 12, clang < 16, and most embedded ARM toolchains
  do not yet ship `<expected>`.
- rlvgl uses `Result<T, E>` pervasively; lvglpp must not throw across the
  embedded path (see `LVGLPP_EMBEDDED_POSTURE` in the top-level
  `CMakeLists.txt`). `expected` is the C++ analog.
- A vendored polyfill keeps the surface stable while toolchains catch up.

## Drop-in path forward

When the project floor moves to a toolchain that ships `<expected>`,
the seam header silently flips to it; nothing in `lvglpp::*` namespace
needs to move. This directory becomes deletable, and the entry under
`third_party/` in CMake can be dropped.

## Scope of the polyfill

The polyfill implements **only** the subset lvglpp uses today:

- `expected(const T&)` / `expected(T&&)` value constructors
- `expected(unexpected<E>)` error constructor
- `expected(unexpect_t, E)` in-place error construction
- `has_value()`, `operator bool()`
- `value()` / `error()` accessors (lvalue, const lvalue, rvalue)
- `value_or(U&&)`
- The `expected<void, E>` partial specialization (success carries no
  payload; `value()` returns `void`). Added for DEMO-04's `decode_into`,
  whose frozen signature is `expected<void, Error>`. Faithful to
  `std::expected<void, E>` for the subset above.

It deliberately does not implement:

- Monadic `and_then` / `or_else` / `transform` / `transform_error`
  — add only when a real call site needs them, with a parity test against
  `std::expected`.
- The `T == E` disambiguation tags. Don't instantiate `expected<X, X>`.
- Reference `T` specializations (`void` `T` is now supported, see above).

If a richer surface is needed before `<expected>` is universal, swap this
file for upstream `tl::expected`
([github.com/TartanLlama/expected](https://github.com/TartanLlama/expected))
and update the seam header.

## License

The polyfill is original lvglpp code under the project license.
