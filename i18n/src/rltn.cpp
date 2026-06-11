// rltn.cpp — RLTN v1 blob reader + runtime locale state.
//
// PARITY: rlvgl/i18n/src/lib.rs (v0.2.0 @ 79f730d) — lookup_in
//         (lib.rs:114–125), active_blob (lib.rs:50–58), atomics
//         (lib.rs:44–48), load_translations (lib.rs:94).
//
// docs/i18n/00-rltn-core.md §5.4.

#include "lvglpp/i18n/rltn.hpp"

#include <atomic>

namespace lvglpp::i18n {

namespace {

// Runtime state. PARITY: lib.rs:44–48 (CURRENT_LOCALE: AtomicU8,
// ACTIVE_BLOB: AtomicPtr). Pointer+length pairs are published with
// release ordering and the registration calls are documented as
// startup-time / single-writer, mirroring rlvgl's Relaxed/Release mix.
std::atomic<std::uint8_t> g_locale{0};

// external: generated rodata (builtin) or caller-permanent media
// buffer (override); never freed here.
std::atomic<const std::uint8_t *> g_builtin_ptr{nullptr};
std::atomic<std::size_t> g_builtin_len{0};
std::atomic<const std::uint8_t *> g_override_ptr{nullptr};
std::atomic<std::size_t> g_override_len{0};

std::uint16_t read_u16(std::span<const std::uint8_t> b, std::size_t at) {
  return static_cast<std::uint16_t>(static_cast<unsigned>(b[at]) |
                                    static_cast<unsigned>(b[at + 1]) << 8);
}

std::uint32_t read_u32(std::span<const std::uint8_t> b, std::size_t at) {
  return static_cast<std::uint32_t>(b[at]) |
         static_cast<std::uint32_t>(b[at + 1]) << 8 |
         static_cast<std::uint32_t>(b[at + 2]) << 16 |
         static_cast<std::uint32_t>(b[at + 3]) << 24;
}

// PARITY: lib.rs:50–58 active_blob() — override if set, else builtin.
std::span<const std::uint8_t> active_blob() {
  const std::uint8_t *p = g_override_ptr.load(std::memory_order_acquire);
  if (p != nullptr) {
    return {p, g_override_len.load(std::memory_order_acquire)};
  }
  p = g_builtin_ptr.load(std::memory_order_acquire);
  if (p != nullptr) {
    return {p, g_builtin_len.load(std::memory_order_acquire)};
  }
  return {};
}

} // namespace

lvglpp::expected<void, BlobError>
validate_blob(std::span<const std::uint8_t> blob, std::uint8_t num_locales,
              std::uint16_t num_keys) {
  if (blob.size() < RLTN_HEADER_SIZE) {
    return lvglpp::unexpected<BlobError>{BlobError::TooShort};
  }
  if (blob[0] != 'R' || blob[1] != 'L' || blob[2] != 'T' || blob[3] != 'N') {
    return lvglpp::unexpected<BlobError>{BlobError::BadMagic};
  }
  if (blob[4] != 1) {
    return lvglpp::unexpected<BlobError>{BlobError::BadVersion};
  }
  if (blob[5] != num_locales || read_u16(blob, 6) != num_keys) {
    return lvglpp::unexpected<BlobError>{BlobError::CountMismatch};
  }
  const std::size_t num_entries =
      static_cast<std::size_t>(num_locales) * num_keys;
  const std::size_t data_start =
      RLTN_HEADER_SIZE + num_entries * RLTN_ENTRY_SIZE;
  if (blob.size() < data_start) {
    return lvglpp::unexpected<BlobError>{BlobError::TooShort};
  }
  for (std::size_t i = 0; i < num_entries; ++i) {
    const std::size_t base = RLTN_HEADER_SIZE + i * RLTN_ENTRY_SIZE;
    const std::size_t offset = read_u32(blob, base);
    const std::size_t len = read_u16(blob, base + 4);
    if (data_start + offset + len > blob.size()) {
      return lvglpp::unexpected<BlobError>{BlobError::OutOfBounds};
    }
  }
  return {};
}

void register_builtin(std::span<const std::uint8_t> blob) {
  g_builtin_len.store(blob.size(), std::memory_order_release);
  g_builtin_ptr.store(blob.data(), std::memory_order_release);
}

std::span<const std::uint8_t> builtin_blob() {
  const std::uint8_t *p = g_builtin_ptr.load(std::memory_order_acquire);
  return p != nullptr
             ? std::span<const std::uint8_t>{p, g_builtin_len.load(
                                                    std::memory_order_acquire)}
             : std::span<const std::uint8_t>{};
}

lvglpp::expected<void, BlobError>
load_translations(std::span<const std::uint8_t> blob) {
  const std::span<const std::uint8_t> builtin = builtin_blob();
  if (builtin.empty()) {
    return lvglpp::unexpected<BlobError>{BlobError::NoBuiltin};
  }
  // The built-in blob's dimensions are ground truth: they were
  // generated together with the application's Locale/Key enums.
  auto valid = validate_blob(blob, builtin[5], read_u16(builtin, 6));
  if (!valid) {
    return valid;
  }
  g_override_len.store(blob.size(), std::memory_order_release);
  g_override_ptr.store(blob.data(), std::memory_order_release);
  return {};
}

void reset_translations() {
  g_override_ptr.store(nullptr, std::memory_order_release);
  g_override_len.store(0, std::memory_order_release);
}

void set_locale_index(std::uint8_t locale) {
  g_locale.store(locale, std::memory_order_relaxed);
}

std::uint8_t locale_index() { return g_locale.load(std::memory_order_relaxed); }

std::string_view lookup_in(std::span<const std::uint8_t> blob,
                           std::uint8_t locale, std::uint16_t key) {
  // PARITY: lib.rs:114–125 lookup_in, field for field.
  const std::size_t num_keys = read_u16(blob, 6);
  const std::size_t idx = static_cast<std::size_t>(locale) * num_keys + key;
  const std::size_t base = RLTN_HEADER_SIZE + idx * RLTN_ENTRY_SIZE;
  const std::size_t offset = read_u32(blob, base);
  const std::size_t len = read_u16(blob, base + 4);
  const std::size_t data_start =
      RLTN_HEADER_SIZE +
      num_keys * static_cast<std::size_t>(blob[5]) * RLTN_ENTRY_SIZE;
  // SAFETY equivalent of lib.rs:123 from_utf8_unchecked: the blob
  // was validated (builtin: generated; override: validate_blob) and
  // codegen guarantees UTF-8 string data.
  return {reinterpret_cast<const char *>(blob.data()) + data_start + offset,
          len};
}

std::string_view t_static_index(std::uint16_t key) {
  const std::span<const std::uint8_t> blob = active_blob();
  if (blob.empty()) {
    return {}; // DELTA: pre-register_builtin state (docs/i18n/00 §5.4).
  }
  return lookup_in(blob, locale_index(), key);
}

} // namespace lvglpp::i18n
