// object.hpp — RAII owner of an lv_obj_t* and the single object model
// every lvglpp widget wrapper builds on (LVGLPP-WRAP-00).
//
// PARITY: rlvgl/core/src/widget.rs + object lifecycle (v0.2.4 @ 343f596).
//         The ownership story matches rlvgl's WidgetNode (exclusive
//         ownership of a tree node); the mechanism wraps lv_obj instead
//         of re-implementing the tree.
// LVGL:   lvgl/src/core/lv_obj.h, lv_obj_tree.h, lv_obj_event.h
//         (lv_obj_create / lv_obj_delete / user_data / LV_EVENT_DELETE).
// DELTA:  Object is move-only and self-registers an LV_EVENT_DELETE
//         callback so LVGL-driven deletion (parent deletion, lv_obj_clean)
//         cannot double-free or dangle the wrapper. The attach_/detach_/
//         release transfer verbs are deferred to LVGLPP-WRAP-01.

#ifndef LVGLPP_CORE_OBJECT_HPP
#define LVGLPP_CORE_OBJECT_HPP

#include <cstdint>

#include "lvglpp/core/runtime.hpp"  // lvglpp::ObjectView
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core {

// Errors that the Object / Screen factories can produce.
enum class ObjectError : std::uint8_t {
    CreateFailed = 1,  // lv_obj_create returned nullptr (e.g. OOM) or bad parent.
    NoDisplay    = 2,  // Screen requested but no default lv_display exists.
};

// RAII owner of a single lv_obj_t. See docs/wrap/00-concepts.md (§5).
//
// Ownership: owns its lv_obj_t. The destructor calls lv_obj_delete iff it
// still owns a live object. If LVGL deletes the object first (parent
// deletion, lv_obj_clean, explicit lv_obj_delete), the delete-safety
// callback nulls the handle so the destructor is a no-op — no double-free,
// no dangle.
class Object {
public:
    // Create a base lv_obj as a child of `parent`.
    // Args:
    //   parent: borrows an lv_obj that outlives the call; the new object
    //           is inserted into parent's child list (LVGL owns the tree
    //           link). Must be non-empty.
    // Returns: owns Object on success; ObjectError on failure.
    [[nodiscard]] static lvglpp::expected<Object, ObjectError>
    try_make(ObjectView parent) noexcept;

    // Throwing convenience over try_make: aborts under embedded posture,
    // throws std::runtime_error on host, if creation fails. Mirrors the
    // Runtime factory pattern (docs/wrap/00-concepts.md §5.5).
    [[nodiscard]] static Object make(ObjectView parent);

    Object(const Object&)            = delete;
    Object& operator=(const Object&) = delete;

    // Move-construct only: transfers the handle and rebinds the LVGL
    // user-data back-pointer to the new location. The moved-from Object is
    // left empty so its destructor is a no-op. Move-assignment is deleted
    // (mirrors Runtime); revisit if a container of Objects needs it.
    Object(Object&& other) noexcept;
    Object& operator=(Object&&) = delete;

    ~Object();

    // Non-owning view of the wrapped object (observes; never frees).
    [[nodiscard]] ObjectView view() const noexcept { return ObjectView{obj_}; }
    // borrows: valid only while this Object owns a live lv_obj.
    [[nodiscard]] lv_obj_t* borrow_raw() const noexcept { return obj_; }
    [[nodiscard]] bool      empty()      const noexcept { return obj_ == nullptr; }

protected:
    // Adopt an already-created lv_obj (used by try_make and by Screen).
    // Installs the user-data back-pointer and the delete-safety callback.
    // `obj` may be nullptr (yields an empty Object).
    explicit Object(lv_obj_t* obj) noexcept;

private:
    // LV_EVENT_DELETE handler: recovers the owning Object via the object's
    // user-data and nulls its handle. Must not throw or touch C++ state
    // beyond the handle (LVGL may invoke it mid-teardown).
    static void on_delete_(lv_event_t* e);

    // owns: the LVGL object. Nulled by on_delete_ if LVGL deletes it first.
    lv_obj_t* obj_ = nullptr;
};

// An Object that is a screen root: created parentless on the default
// display and activated via load(). See docs/wrap/00-concepts.md §3.
//
// Ownership: owns its screen lv_obj. Do not let the wrapper destroy a
// screen while it is the active screen — load another screen first (LVGL
// forbids deleting the active screen).
class Screen : public Object {
public:
    // Returns: owns Screen on success; NoDisplay if there is no default
    // display, CreateFailed on allocation failure.
    [[nodiscard]] static lvglpp::expected<Screen, ObjectError> try_make() noexcept;

    // Throwing convenience over try_make: aborts under embedded posture,
    // throws std::runtime_error on host, on failure.
    [[nodiscard]] static Screen make();

    // Make this screen the active screen (lv_screen_load). No-op if empty.
    void load() noexcept;

    Screen(Screen&&) noexcept = default;

private:
    explicit Screen(lv_obj_t* obj) noexcept : Object{obj} {}
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_OBJECT_HPP
