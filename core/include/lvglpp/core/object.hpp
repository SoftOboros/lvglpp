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
#include <functional>  // LPAR-04: std::function event handlers
#include <memory>      // LPAR-04: std::unique_ptr handler holders
#include <vector>      // LPAR-04: per-Object handler list

#include "lvglpp/core/draw.hpp"     // FONT-00: Font handle (text_font selection)
#include "lvglpp/core/runtime.hpp"  // lvglpp::ObjectView
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core {

// Forward declarations for the LPAR-07 style cascade. The full definitions
// live in style_cascade.hpp (nested namespace style); Object::add_style takes
// them by reference / by value, so a forward declaration suffices in this
// header. object.cpp includes style_cascade.hpp for the definitions.
namespace style {
class Style;
class Selector;
}  // namespace style

// Errors that the Object / Screen factories can produce.
enum class ObjectError : std::uint8_t {
    CreateFailed = 1,  // lv_obj_create returned nullptr (e.g. OOM) or bad parent.
    NoDisplay    = 2,  // Screen requested but no default lv_display exists.
};

// LPAR-02 — mirror of LVGL object flags (lv_obj_flag_t,
// lvgl/src/core/lv_obj.h). Standards Action to add a value that must
// agree with rlvgl ObjectFlags. See docs/core-object/00-object-substrate.md.
enum class ObjectFlag : std::uint32_t {
    Hidden         = LV_OBJ_FLAG_HIDDEN,
    Clickable      = LV_OBJ_FLAG_CLICKABLE,
    ClickFocusable = LV_OBJ_FLAG_CLICK_FOCUSABLE,
    Checkable      = LV_OBJ_FLAG_CHECKABLE,
    Scrollable     = LV_OBJ_FLAG_SCROLLABLE,
    ScrollOnFocus  = LV_OBJ_FLAG_SCROLL_ON_FOCUS,
    Floating       = LV_OBJ_FLAG_FLOATING,
};

// LPAR-02 — mirror of LVGL interaction states (lv_state_t,
// lvgl/src/core/lv_obj_style.h). Standards Action to diverge.
enum class ObjectState : std::uint16_t {
    Default  = LV_STATE_DEFAULT,
    Checked  = LV_STATE_CHECKED,
    Focused  = LV_STATE_FOCUSED,
    FocusKey = LV_STATE_FOCUS_KEY,
    Edited   = LV_STATE_EDITED,
    Hovered  = LV_STATE_HOVERED,
    Pressed  = LV_STATE_PRESSED,
    Scrolled = LV_STATE_SCROLLED,
    Disabled = LV_STATE_DISABLED,
};

// LPAR-05 — scroll enums mirroring LVGL (lvgl/src/core/lv_obj_scroll.h,
// lvgl/src/misc/lv_area.h). Standards Action to diverge. See
// docs/core-scroll/00-scroll-runtime.md.
enum class ScrollbarMode : std::uint8_t {
    Off    = LV_SCROLLBAR_MODE_OFF,
    On     = LV_SCROLLBAR_MODE_ON,
    Active = LV_SCROLLBAR_MODE_ACTIVE,
    Auto   = LV_SCROLLBAR_MODE_AUTO,
};

enum class ScrollSnap : std::uint8_t {
    None   = LV_SCROLL_SNAP_NONE,
    Start  = LV_SCROLL_SNAP_START,
    End    = LV_SCROLL_SNAP_END,
    Center = LV_SCROLL_SNAP_CENTER,
};

enum class ScrollDir : std::uint8_t {
    None   = LV_DIR_NONE,
    Left   = LV_DIR_LEFT,
    Right  = LV_DIR_RIGHT,
    Top    = LV_DIR_TOP,
    Bottom = LV_DIR_BOTTOM,
    Hor    = LV_DIR_HOR,
    Ver    = LV_DIR_VER,
    All    = LV_DIR_ALL,
};

// LPAR-10 — layout enums mirroring LVGL flex/grid
// (lvgl/src/layouts/{flex,grid}). Standards Action to diverge. See
// docs/core-layout/00-layout.md.
enum class FlexFlow : std::uint8_t {
    Row               = LV_FLEX_FLOW_ROW,
    Column            = LV_FLEX_FLOW_COLUMN,
    RowWrap           = LV_FLEX_FLOW_ROW_WRAP,
    RowReverse        = LV_FLEX_FLOW_ROW_REVERSE,
    RowWrapReverse    = LV_FLEX_FLOW_ROW_WRAP_REVERSE,
    ColumnWrap        = LV_FLEX_FLOW_COLUMN_WRAP,
    ColumnReverse     = LV_FLEX_FLOW_COLUMN_REVERSE,
    ColumnWrapReverse = LV_FLEX_FLOW_COLUMN_WRAP_REVERSE,
};

enum class FlexAlign : std::uint8_t {
    Start        = LV_FLEX_ALIGN_START,
    End          = LV_FLEX_ALIGN_END,
    Center       = LV_FLEX_ALIGN_CENTER,
    SpaceEvenly  = LV_FLEX_ALIGN_SPACE_EVENLY,
    SpaceAround  = LV_FLEX_ALIGN_SPACE_AROUND,
    SpaceBetween = LV_FLEX_ALIGN_SPACE_BETWEEN,
};

enum class GridAlign : std::uint8_t {
    Start        = LV_GRID_ALIGN_START,
    End          = LV_GRID_ALIGN_END,
    Center       = LV_GRID_ALIGN_CENTER,
    Stretch      = LV_GRID_ALIGN_STRETCH,
    SpaceEvenly  = LV_GRID_ALIGN_SPACE_EVENLY,
    SpaceAround  = LV_GRID_ALIGN_SPACE_AROUND,
    SpaceBetween = LV_GRID_ALIGN_SPACE_BETWEEN,
};

// LPAR-04 — event codes mirroring a useful subset of lv_event_code_t
// (lvgl/src/misc/lv_event.h). Standards Action to add a value that must agree
// with rlvgl's event surface. See docs/core-event/01-event-focus-input.md.
enum class EventCode : std::uint32_t {
    All          = LV_EVENT_ALL,
    Pressed      = LV_EVENT_PRESSED,
    Pressing     = LV_EVENT_PRESSING,
    Released     = LV_EVENT_RELEASED,
    Clicked      = LV_EVENT_CLICKED,
    ValueChanged = LV_EVENT_VALUE_CHANGED,
    Focused      = LV_EVENT_FOCUSED,
    Defocused    = LV_EVENT_DEFOCUSED,
    Key          = LV_EVENT_KEY,
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

    // --- LPAR-02: flags, state, hit-test, tree queries (over lv_obj_*) ---
    // All are no-ops / empty-safe defaults when this Object is empty.
    void add_flag(ObjectFlag f) noexcept;
    void remove_flag(ObjectFlag f) noexcept;
    [[nodiscard]] bool has_flag(ObjectFlag f) const noexcept;

    void add_state(ObjectState s) noexcept;
    void remove_state(ObjectState s) noexcept;
    [[nodiscard]] bool has_state(ObjectState s) const noexcept;
    // Active-state bitmask (bitwise-or of ObjectState bits).
    [[nodiscard]] ObjectState state() const noexcept;

    // True if screen-coordinate (x, y) hits this object.
    [[nodiscard]] bool hit_test(std::int32_t x, std::int32_t y) const noexcept;

    // Non-owning tree neighbors (observes). An empty Object or an
    // out-of-range index yields an empty ObjectView.
    [[nodiscard]] ObjectView    parent()      const noexcept;
    [[nodiscard]] std::uint32_t child_count() const noexcept;
    [[nodiscard]] ObjectView    child(std::uint32_t index) const noexcept;

    // --- LPAR-03: invalidation (over lv_obj_invalidate) ---
    // Mark this object's area dirty; LVGL repaints it on the next refresh
    // (lv_timer_handler). No-op when this Object is empty.
    void invalidate() noexcept;

    // --- LPAR-05: scroll (over lv_obj_scroll_*) ---
    void scroll_to(std::int32_t x, std::int32_t y, bool animate) noexcept;
    void scroll_by(std::int32_t dx, std::int32_t dy, bool animate) noexcept;
    [[nodiscard]] std::int32_t scroll_x() const noexcept;
    [[nodiscard]] std::int32_t scroll_y() const noexcept;
    void set_scroll_dir(ScrollDir dir) noexcept;
    void set_scrollbar_mode(ScrollbarMode mode) noexcept;
    void set_scroll_snap(ScrollSnap x, ScrollSnap y) noexcept;

    // --- LPAR-10: layout & sizing (over lv_obj_set_flex_*/grid_*) ---
    void set_size(std::int32_t w, std::int32_t h) noexcept;
    void set_flex_flow(FlexFlow flow) noexcept;
    void set_flex_grow(std::uint8_t grow) noexcept;
    void set_flex_align(FlexAlign main, FlexAlign cross, FlexAlign track) noexcept;
    // col_dsc / row_dsc are caller-owned arrays terminated with
    // LV_GRID_TEMPLATE_LAST. LVGL stores the pointers, so they MUST
    // outlive this object (docs/core-layout/00-layout.md §5.2).
    //   col_dsc, row_dsc: borrows; must outlive this object.
    void set_grid_dsc(const std::int32_t* col_dsc, const std::int32_t* row_dsc) noexcept;
    void set_grid_cell(GridAlign col_align, std::int32_t col_pos, std::int32_t col_span,
                       GridAlign row_align, std::int32_t row_pos, std::int32_t row_span) noexcept;
    void set_grid_align(GridAlign col_align, GridAlign row_align) noexcept;

    // Sizing helpers: LV_SIZE_CONTENT and percentage-encoded sizes.
    [[nodiscard]] static std::int32_t size_content() noexcept;
    [[nodiscard]] static std::int32_t pct(std::int32_t value) noexcept;

    // --- LPAR-07: style cascade (over lv_obj_add_style / lv_obj_set_style_*) ---
    // Attach a reusable Style to this object for the given selector
    // (lv_obj_add_style). The Style is borrowed, NOT owned: LVGL stores a
    // pointer to its lv_style_t, so `s` MUST outlive this object (or be
    // removed first). See docs/core-style/01-style-cascade-theme.md §5.1.
    //   s: borrows; must outlive this object or be removed via remove_style.
    void add_style(style::Style& s, style::Selector sel) noexcept;
    void remove_style(style::Style& s, style::Selector sel) noexcept;
    // Drop every attached style from every part/state (lv_obj_remove_style_all).
    void remove_all_styles() noexcept;

    // Per-object local property overrides (lv_obj_set_style_*). Unlike a
    // shared Style these are copied into the object, so they carry no
    // outlives-object hazard. A representative subset; reach
    // lv_obj_set_style_* directly for properties not surfaced here.
    void set_local_bg_color(lv_color_t c, style::Selector sel) noexcept;
    void set_local_bg_opa(lv_opa_t opa, style::Selector sel) noexcept;
    void set_local_text_color(lv_color_t c, style::Selector sel) noexcept;
    void set_local_border_width(std::int32_t w, style::Selector sel) noexcept;
    void set_local_radius(std::int32_t r, style::Selector sel) noexcept;
    void set_local_pad_all(std::int32_t p, style::Selector sel) noexcept;

    // --- FONT-00: text-font selection (over lv_obj_*_style_text_font) ---
    // Set this object's text font for `sel` (lv_obj_set_style_text_font). The
    // Font is borrowed into LVGL (its lv_font_t* is stored, not copied), so it
    // MUST outlive this object (FONT-00 §5.2). An empty Font is a no-op.
    //   font: borrows; must outlive this object.
    void set_local_text_font(const Font& font, style::Selector sel) noexcept;
    // The resolved effective text font of this object (cascade + theme +
    // inheritance) via lv_obj_get_style_text_font(MAIN). Empty Font when this
    // Object is empty. This is FONT-00's font-resolution read path.
    [[nodiscard]] Font text_font() const noexcept;

    // --- LPAR-04: per-object event callbacks (over lv_obj_add_event_cb) ---
    // Register a C++ handler for `code`. The handler is OWNED by this Object:
    // it is stored in a heap holder kept in the Object's handler list and
    // torn down when the Object (and its lv_obj) is destroyed — mechanically
    // safe per CLAUDE.md callback rule 9. The holder pointer is the per-cb
    // user_data, so it survives Object moves (the holder address is stable).
    //
    // Two shapes: the `lv_event_t*` overload gives the handler full access to
    // the live event (call lvglpp::core::event_from_lv(e) for the CORE-02
    // view); the zero-arg overload is the ergonomic common case. No-op when
    // this Object is empty.
    //   handler: owns; moved into this Object, invoked until the Object dies.
    void on(EventCode code, std::function<void(lv_event_t*)> handler) noexcept;
    void on(EventCode code, std::function<void()> handler) noexcept;

    // --- LVGLPP-WRAP-0N: playit tag channel (over the lv_obj name) ---
    // The LVGL object name doubles as the playit tag (WRAP-00 §5.4 reserves
    // the name field for exactly this). set_tag copies the string
    // (lv_obj_set_name), so `tag` need not outlive the call. No-op when empty.
    void set_tag(const char* tag) noexcept;
    // The object's tag, or "" if unset / empty (lv_obj_get_name).
    [[nodiscard]] const char* tag() const noexcept;
    // Breadth-first search this object's subtree for a descendant whose tag
    // matches `name` (lv_obj_find_by_name). Does NOT match this object itself.
    // Returns an empty ObjectView if not found or this Object is empty.
    [[nodiscard]] ObjectView find_by_tag(const char* name) const noexcept;

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

    // LPAR-04 event trampoline: recovers the heap handler holder via the
    // event's per-cb user_data (NOT the object's user_data, which holds the
    // delete-safety back-pointer) and invokes the stored function. Must not
    // throw.
    static void on_event_(lv_event_t* e);

    // The stored handler type: every `on` overload normalizes to this.
    using EventHandler = std::function<void(lv_event_t*)>;

    // owns: the LVGL object. Nulled by on_delete_ if LVGL deletes it first.
    lv_obj_t* obj_ = nullptr;
    // owns: the per-object event handlers. Each holder's address is the
    // user_data registered with lv_obj_add_event_cb, so it must stay stable
    // across Object moves — unique_ptr gives that (the held function does not
    // move when the vector reallocates). Freed after lv_obj_delete in ~Object.
    std::vector<std::unique_ptr<EventHandler>> handlers_;
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
