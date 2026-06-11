// rltn.hpp — RLTN v1 translation blob reader + runtime locale state.
//
// PARITY: rlvgl/i18n/src/lib.rs (v0.2.0 @ 79f730d) — blob format
//         (lib.rs:14–29), set_locale/locale (lib.rs:102–109),
//         t_static (lib.rs:129), load_translations (lib.rs:94),
//         builtin_blob (lib.rs:164).
// LVGL:   N/A (upstream lv_translation is bridged separately; see
//         docs/i18n/01-lv-translation-bridge.md).
// DELTA:  load_translations validates the blob and returns
//         lvglpp::expected (rlvgl: unsafe, caller-trusted); the
//         built-in blob is registered at startup by the generated
//         init_rltn() instead of include_bytes! — the runtime is
//         key-agnostic, typed Locale/Key wrappers are generated.
//         docs/i18n/00-rltn-core.md §5.3–§5.4.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "lvglpp/std/expected.hpp"

namespace lvglpp::i18n {

inline constexpr std::size_t RLTN_HEADER_SIZE = 8;
inline constexpr std::size_t RLTN_ENTRY_SIZE = 6;

enum class BlobError : std::uint8_t {
  TooShort,      // smaller than the fixed header
  BadMagic,      // header != "RLTN"
  BadVersion,    // version != 1
  CountMismatch, // locale/key counts differ from the built-in blob
  OutOfBounds,   // an index entry points past the end of the blob
  NoBuiltin,     // register_builtin() has not been called yet
};

// Validate an RLTN v1 blob: header, version, expected dimensions, and
// every index entry within the string-data region.
// Args:
//   blob:        borrows for the duration of the call.
//   num_locales: expected locale count (from the generated enums).
//   num_keys:    expected key count (from the generated enums).
[[nodiscard]] lvglpp::expected<void, BlobError>
validate_blob(std::span<const std::uint8_t> blob, std::uint8_t num_locales,
              std::uint16_t num_keys);

// Register the application's built-in blob. The generated init_rltn()
// calls this once at startup, before any lookup.
// Args:
//   blob: external; generated rodata with static storage duration —
//         never freed, never mutated.
void register_builtin(std::span<const std::uint8_t> blob);

// Returns the built-in blob (empty span before register_builtin).
// PARITY: lib.rs:164 builtin_blob().
[[nodiscard]] std::span<const std::uint8_t> builtin_blob();

// Override the built-in translations with a blob loaded from media.
// PARITY: lib.rs:94 load_translations(Some(..)); adapted: validates
// against the built-in blob's dimensions instead of trusting the
// caller (named DELTA, docs/i18n/00 §5.4).
// Args:
//   blob: external; caller guarantees static storage duration (e.g. a
//         leaked/permanent buffer read from SD) — never freed by this
//         module.
[[nodiscard]] lvglpp::expected<void, BlobError>
load_translations(std::span<const std::uint8_t> blob);

// Revert to the built-in blob. PARITY: lib.rs:94 load_translations(None).
void reset_translations();

// Set / get the active locale index for subsequent lookups.
// PARITY: lib.rs:102–109 set_locale/locale (relaxed atomics). The
// generated header provides the typed Locale wrappers.
void set_locale_index(std::uint8_t locale);
[[nodiscard]] std::uint8_t locale_index();

// Look up a string in `blob` without touching runtime state.
// PARITY: lib.rs:114–125 lookup_in. No bounds checks beyond what the
// blob's index encodes — callers pass validated blobs and in-range
// indices (generated enums guarantee both for application code).
// Returns: borrows the blob's storage.
[[nodiscard]] std::string_view lookup_in(std::span<const std::uint8_t> blob,
                                         std::uint8_t locale,
                                         std::uint16_t key);

// Plain (non-parameterized) lookup in the active blob at the current
// locale. PARITY: lib.rs:129 t_static. Returns an empty view before
// register_builtin (rlvgl cannot reach this state; documented DELTA).
// Returns: borrows the active blob's static storage.
[[nodiscard]] std::string_view t_static_index(std::uint16_t key);

} // namespace lvglpp::i18n
