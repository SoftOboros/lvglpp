// runtime.cpp — Runtime + ObjectView implementation.
//
// PARITY: rlvgl/core/src/application.rs (v0.2.0 @ d99f793).
// LVGL:   lvgl/src/core/lv_obj.c (lv_init).

#include "lvglpp/core/runtime.hpp"

#include <atomic>

#if !defined(LVGLPP_EMBEDDED_POSTURE)
#  include <stdexcept>
#endif

namespace lvglpp {

namespace {
// Singleton-presence flag. This does not own any LVGL resource; LVGL
// owns its globals. The flag enforces the "one Runtime at a time" rule
// without taking ownership.
std::atomic<bool> g_runtime_alive{false};

bool acquire_alive_slot() noexcept {
    bool expected = false;
    return g_runtime_alive.compare_exchange_strong(expected, true);
}

void release_alive_slot() noexcept {
    g_runtime_alive.store(false);
}
}  // namespace

Runtime::Runtime() {
    if (!acquire_alive_slot()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        // Embedded posture: panic ≡ abort, mirroring rlvgl no_std +
        // panic=abort. Host code must use try_make() to avoid this.
        std::abort();
#else
        throw std::runtime_error("lvglpp::Runtime: a Runtime is already alive");
#endif
    }
    lv_init();
}

Runtime::Runtime(PrivateTag) noexcept {
    // Slot already acquired by try_make(); only the lv_init() side-effect
    // remains. lv_init() in LVGL 9 has no failure return.
    lv_init();
}

Runtime::Runtime(Runtime&& other) noexcept : alive_owned_{other.alive_owned_} {
    other.alive_owned_ = false;
}

Runtime::~Runtime() {
    // LVGL has no global teardown entry point; releasing the singleton
    // flag is the only cleanup required (and only by the owner of the
    // slot — moved-from Runtimes skip this).
    if (alive_owned_) {
        release_alive_slot();
    }
}

lvglpp::expected<Runtime, RuntimeError> Runtime::try_make() noexcept {
    if (!acquire_alive_slot()) {
        return lvglpp::unexpected<RuntimeError>{RuntimeError::AlreadyAlive};
    }
    return Runtime{PrivateTag{}};
}

}  // namespace lvglpp
