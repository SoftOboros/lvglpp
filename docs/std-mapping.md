# lvglpp — Rust → C++ standard-library mapping

This document is **normative** for lvglpp. When a concept exists on the
rlvgl side and lvglpp ports it, the C++ surface MUST follow the mapping
below unless the per-call-site comment names a documented exception.

The goal: lvgl's *behavior* + rlvgl's *discipline* + C++20 std's
*spelling*. When the C++ std library deviates from Rust std semantics,
the difference is called a **delta** and is normalized — usually by
providing a small lvglpp helper that restores the Rust semantics
without surprising the C++ reader.

See [`CLAUDE.md`](../CLAUDE.md) § "Strict and Explicit Ownership" for
the ownership-tag system this mapping relies on, and § "Spec-Before-Code
Planning Discipline" for when changes here require a concepts-doc
amendment.

## Owning types

| rlvgl (Rust) | lvglpp (C++20) | Ownership tag | Notes |
| --- | --- | --- | --- |
| `Box<T>` | `std::unique_ptr<T>` | `owns` | Same intent. `make_unique<T>(...)` mirrors `Box::new(...)`. |
| `Arc<T>` | `std::shared_ptr<T>` | `shares` | Use only when shared lifetime is **intrinsic** (per CLAUDE.md ownership rule 3). `unique_ptr` first. |
| `Rc<T>` | `std::shared_ptr<T>` (single-thread context) | `shares` | C++ has no thread-unaware equivalent; pay for `shared_ptr`'s atomicity. |
| `Weak<T>` | `std::weak_ptr<T>` | `observes` | Same intent. |
| `Vec<T>` | `std::vector<T>` | `owns` | Same intent. |
| `String` | `std::string` | `owns` | Same intent (UTF-8 by convention; enforced at boundaries). |
| `Cow<'a, T>` | `std::variant<View, Owned>` per site | `borrows` xor `owns` | No general analog. Define the variant locally and document. |

## Borrowing types

| rlvgl (Rust) | lvglpp (C++20) | Ownership tag | Notes |
| --- | --- | --- | --- |
| `&T` | `const T&` | `borrows` | Lifetime documented in function contract comment. |
| `&mut T` | `T&` | `borrows` (mut) | Same. |
| `&str` | `std::string_view` | `borrows` | Non-owning; lifetime tag required. |
| `&[T]` | `std::span<const T>` | `borrows` | Non-owning view. |
| `&mut [T]` | `std::span<T>` | `borrows` (mut) | Same. |
| `Option<&T>` | `const T*` (with `// observes:` tag) **or** `std::optional<std::reference_wrapper<const T>>` | `observes` | Prefer raw pointer for hot paths; document nullability. |

## Sum / option types

| rlvgl (Rust) | lvglpp (C++20/23) | Notes |
| --- | --- | --- |
| `Option<T>` | `std::optional<T>` | **DELTA — `take`:** Rust `Option::take()` empties `self` and returns the old value. C++ `optional::value()` does **not** move-clear. lvglpp provides `lvglpp::take(opt)` (in `lvglpp/std/optional.hpp` once that header lands) that mirrors Rust semantics: `auto v = lvglpp::take(opt); // opt is now empty`. Use `lvglpp::take` whenever the Rust source spells `.take()`. |
| `Result<T, E>` | `lvglpp::expected<T, E>` (alias for `std::expected` when `<expected>` is available; vendored polyfill otherwise — see `include/lvglpp/std/expected.hpp`) | **DELTA — `?` operator:** Rust's `expr?` early-returns on `Err`. C++ has no operator analog. lvglpp will provide `LVGLPP_TRY(expr)` (a macro that early-returns on `!has_value()`) when the first call site needs it. Until then, write the unwrap explicitly. |
| `Result<(), E>` | `lvglpp::expected<void, E>` | Polyfill caveat: the vendored polyfill does not yet specialize `expected<void, E>`. If a call site needs it, either grow the polyfill or use `lvglpp::expected<std::monostate, E>` as a workaround. Prefer growing the polyfill. |
| `enum { A, B }` (data-less) | `enum class : T { A, B }` | Pick the underlying type explicitly to mirror Rust's `#[repr(u8)]` etc. |
| `enum { A(T), B }` (with payload) | `std::variant<A_payload, B_payload>` or hand-rolled tagged union | Hand-roll on hot paths or under `LVGLPP_EMBEDDED_POSTURE` where `<variant>` overhead matters. |

## Trait / generic dispatch

| rlvgl (Rust) | lvglpp (C++20) | Notes |
| --- | --- | --- |
| `dyn Trait` | abstract base class with virtual methods, **or** `std::function<Sig>` for callbacks | Choose per call-site cost. Document choice. |
| `impl Trait` (return position) | C++20 concept + `auto` return | Same intent. |
| `impl Trait` (argument position) | C++20 concept + template parameter | Same intent. |
| `where T: Trait` | `requires` clause | Same intent. |
| `PhantomData<T>` | empty tag struct + `[[no_unique_address]]` member | Same intent. |
| `'a` lifetime parameter | comment in function contract + ownership tag | Lifetimes are not in the C++ type system; document them. The strict-and-explicit-ownership discipline carries the load. |

## Iteration

| rlvgl (Rust) | lvglpp (C++20) | Notes |
| --- | --- | --- |
| `for x in iter` | range-based `for (auto& x : range)` | Prefer ranges over manual iterator pairs. |
| `iter.map(f).filter(g).collect::<Vec<_>>()` | `std::ranges::to<std::vector>(rng \| std::views::transform(f) \| std::views::filter(g))` (or compose by hand if the toolchain lacks `ranges::to`) | C++20 ranges + `<ranges>` views are the closest analog. AppleClang 14 lacks `ranges::to`; use `std::vector v(rng.begin(), rng.end())` until then. |
| `iter.collect::<Result<Vec<_>, E>>()` | hand-roll a loop that early-returns on the first error | No standard short-circuiting collect. |

## Failure model

| rlvgl (Rust) | lvglpp (C++) | Notes |
| --- | --- | --- |
| `panic!("...")` | `LVGLPP_PANIC("...")` (planned) — `std::abort()` after a logged message | Same intent. The macro will live in `lvglpp/std/panic.hpp` once a panic site exists. |
| `unwrap()` / `expect("...")` | call `value()` on `expected` / `optional`; on absence, `std::abort()` under embedded posture, throw under host | Avoid `unwrap`-style calls on critical paths; prefer `LVGLPP_TRY` when it lands. |
| `unreachable!()` | `std::unreachable()` (C++23) or `__builtin_unreachable()` fallback | Same intent. Wrap in `LVGLPP_UNREACHABLE` when first needed. |
| `assert!(cond)` | `assert(cond)` (`<cassert>`) for debug; static assertions via `static_assert` | Same intent. |
| `debug_assert!(cond)` | `assert(cond)` (becomes no-op under `NDEBUG`) | Same intent. |
| `?` operator | `LVGLPP_TRY(expr)` (planned macro) | See `Result<T, E>` row above. |

## Embedded posture

When `LVGLPP_EMBEDDED_POSTURE` is on (CMake option, default OFF), the
project mirrors rlvgl's `no_std` + `panic = abort`:

- **Compiler flags:** `-fno-exceptions -fno-rtti` are added to every
  lvglpp target via the `lvglpp_posture` INTERFACE library.
- **Throwing APIs:** every constructor / function that documents a
  `Throws:` line MUST have a non-throwing factory equivalent that
  returns `lvglpp::expected<T, E>`. The throwing path becomes
  `std::abort()` under embedded posture.
- **Host-only convenience:** any `Throws:`-line API may stay in the
  public surface, but its body MUST be guarded by
  `#if !defined(LVGLPP_EMBEDDED_POSTURE)`.

### Freestanding subset

Under embedded posture, lvglpp targets MAY include only the
freestanding subset of the C++ standard library:

- **Allowed (always):** `<cstdint>`, `<cstddef>`, `<cstring>`,
  `<cstdlib>` (for `std::abort`), `<utility>`, `<type_traits>`,
  `<new>`, `<atomic>`.
- **Allowed (with care):** `<span>`, `<optional>`, `<expected>` (or
  the polyfill), `<string_view>`, `<array>` — these are all
  freestanding-friendly and used pervasively.
- **Disallowed:** `<iostream>`, `<fstream>`, `<thread>`, `<mutex>`,
  `<filesystem>`, `<chrono>` clock types other than
  `steady_clock` (board-supplied), `<regex>`, anything pulling
  `<locale>` or full `<exception>`.
- **Disallowed unless gated by `#if !defined(LVGLPP_EMBEDDED_POSTURE)`:**
  `<stdexcept>`, `<vector>` (allowed if the embedded target supplies a
  custom allocator and the call site documents it), `<string>`.

When a header is in the third column ("with care"), the file MUST
either compile freestanding or be gated by
`#if !defined(LVGLPP_EMBEDDED_POSTURE)`.

## Naming conventions

Verbs that expose ownership intent (carry over from CLAUDE.md):

| Rust spelling | lvglpp spelling | Ownership semantics |
| --- | --- | --- |
| `Foo::new(...)` | `make_foo(...)` (free function) or `Foo::make(...)` (factory) | Returns `owns Foo` |
| `Foo::try_new(...)` | `Foo::try_make(...)` | Returns `owns expected<Foo, E>` |
| `take_foo(self)` | `take_foo(Foo&&)` | Consumes ownership in |
| `as_foo(&self)` | `borrow_foo()` (member) or `borrow_foo(const Foo&)` | Returns `borrows` |
| `iter()` / `iter_mut()` | `view_foo()` / `borrow_foo()` | Returns non-owning view / mut borrow |
| `into_inner(self)` | `release_foo() &&` | Gives up ownership |
| `attach_to_parent(self, parent)` | `attach_to(Parent&)` | Transfers ownership into parent |
| `detach()` | `detach()` | Removes ownership from parent |

Avoid: `set_ptr`, `set_buffer`, `register_callback`. Prefer the
explicit ownership verbs above.

## Polyfill convention

When a C++ std-library type is not yet available on a supported
toolchain, lvglpp ships a **minimal vendored polyfill** under
`third_party/<feature>/` and a **seam header** under
`include/lvglpp/std/<feature>.hpp` that selects between
`<feature>` and the polyfill at compile time.

- Consumers always write `lvglpp::<name>`. Never `std::<name>` or
  `::tl::<name>` directly.
- Polyfills cover only the subset lvglpp uses. They are deliberately
  small and not a general-purpose C++ futurology library.
- When the project floor moves to a toolchain that ships the real
  feature, the polyfill is deleted and the seam header drops to the
  `<feature>` branch with no other changes required.

The first such polyfill is `lvglpp::expected` — see
`third_party/lvglpp_expected/README.md`.

## Change log

- 2026-04-27 — Initial draft. Owning / borrowing / sum-type / trait /
  iteration / failure-model tables landed; embedded posture and
  freestanding subset codified; polyfill convention codified.
