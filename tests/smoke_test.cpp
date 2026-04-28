// Smoke test: exercise the Runtime RAII singleton, the move-only
// shape, the try_make() factory + lvglpp::expected seam, and the
// ObjectView wrapper. Host-side only; no display driver registered.

#include "lvglpp/lvglpp.hpp"

#include <cassert>
#include <stdexcept>

int main() {
    // Throwing constructor (host posture only).
    {
        lvglpp::Runtime rt;

        bool threw = false;
        try {
            lvglpp::Runtime second;
            (void)second;
        } catch (const std::runtime_error&) {
            threw = true;
        }
        assert(threw && "second Runtime must throw under host posture");
    }

    // After rt destruction, a fresh Runtime must construct cleanly.
    {
        lvglpp::Runtime rt2;
        (void)rt2;
    }

    // try_make() factory + expected<Runtime, RuntimeError> seam.
    {
        auto a = lvglpp::Runtime::try_make();
        assert(a.has_value() && "first try_make must succeed");

        auto b = lvglpp::Runtime::try_make();
        assert(!b.has_value() && "second try_make must fail");
        assert(b.error() == lvglpp::RuntimeError::AlreadyAlive);
    }

    // After both expected wrappers go out of scope, slot is free again.
    {
        auto c = lvglpp::Runtime::try_make();
        assert(c.has_value());
    }

    lvglpp::ObjectView empty{nullptr};
    assert(empty.empty());
    assert(empty.borrow_raw() == nullptr);

    return 0;
}
