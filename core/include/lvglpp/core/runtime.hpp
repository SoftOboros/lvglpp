// runtime.hpp — RAII guard around lv_init() and a non-owning view over
// lv_obj_t* for use as the C++ surface of LVGL's object tree.
//
// PARITY: rlvgl/core/src/application.rs (Runtime/Application bootstrap)
//         and rlvgl/core/src/widget.rs (WidgetNode borrowing rules,
//         v0.2.0 @ d99f793).
// LVGL:   lvgl/src/core/lv_obj.h (lv_obj_t, lv_obj_del, the object tree
//         lifecycle this view defers to).
// DELTA:  rlvgl returns Result on init failure; lvglpp's Runtime mirrors
//         that under LVGLPP_EMBEDDED_POSTURE (factory returns
//         lvglpp::expected). On host builds it throws std::runtime_error
//         to match idiomatic C++ RAII. See docs/std-mapping.md.

#ifndef LVGLPP_CORE_RUNTIME_HPP
#define LVGLPP_CORE_RUNTIME_HPP

#include <cstdint>
#include <string_view>

#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp {

// Returns the lvglpp library version as a compile-time string view.
// Returns: borrows a static string; caller must not free.
[[nodiscard]] constexpr std::string_view library_version() noexcept {
    return "0.1.0";
}

// Errors that the Runtime constructor / factory can produce.
enum class RuntimeError : std::uint8_t {
    AlreadyAlive = 1,
    InitFailed   = 2,
};

// RAII guard that calls lv_init() on construction. Constructing a second
// instance while one is alive is a programmer error.
//
// Ownership: owns the LVGL global runtime for its lifetime. There must
// be exactly one live Runtime in a process.
class Runtime {
public:
    // Args: (none)
    // Returns: owns Runtime via RAII.
    // Throws (host posture only): std::runtime_error on AlreadyAlive.
    //
    // Under LVGLPP_EMBEDDED_POSTURE the throwing constructor is
    // unavailable; use try_make() which returns lvglpp::expected.
    Runtime();

    Runtime(const Runtime&)            = delete;
    Runtime& operator=(const Runtime&) = delete;

    // Move-construct only — the source is left inert so its destructor
    // is a no-op. Move-assignment is deleted: there is no defined
    // semantics for "replace an alive Runtime with another alive
    // Runtime" because the alive-slot is a singleton.
    //
    // This shape mirrors a Rust `struct Runtime` without `Copy` —
    // by-value moves are fine, double-ownership is not.
    Runtime(Runtime&& other) noexcept;
    Runtime& operator=(Runtime&&) = delete;

    ~Runtime();

    // Non-throwing factory mirroring rlvgl's Result<Runtime, _>.
    // Returns: owns Runtime on success; RuntimeError on failure.
    [[nodiscard]] static lvglpp::expected<Runtime, RuntimeError> try_make() noexcept;

private:
    struct PrivateTag {};
    explicit Runtime(PrivateTag) noexcept;

    // owns the alive-slot when true; moved-from Runtimes set this to
    // false so the destructor does not release the slot twice.
    bool alive_owned_ = true;
};

// Thin non-owning view of an lv_obj_t* tree node. Wrapping is intentional:
// LVGL owns the object via its parent in the object tree, so a C++
// wrapper that owned the pointer would double-free at lv_obj_del time.
//
// Ownership: external. The underlying lv_obj_t is owned by the LVGL
// object tree (typically rooted at the active screen). Detach/delete via
// lv_obj_del() — never via the wrapper.
class ObjectView {
public:
    // Args:
    //   raw: external; owned by the LVGL object tree. Must remain valid
    //        for the lifetime of this view.
    explicit ObjectView(lv_obj_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_obj_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool      empty()      const noexcept { return raw_ == nullptr; }

private:
    // external: owned by the LVGL object tree; freed by lv_obj_del() on
    // the owning parent. This view never frees it.
    lv_obj_t* raw_;
};

}  // namespace lvglpp

#endif  // LVGLPP_CORE_RUNTIME_HPP
