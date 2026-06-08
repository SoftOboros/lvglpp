// disco_controller.hpp — shared 747-style demo controller.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/lib.rs (v0.2.0 @ 79f730d)
//         — DiscoController (:820, new :831, root :1240, dispatch_event
//         :1245, handle_event :1252, tick :1284, drain_commands :1290,
//         publish_status :1295), ControllerState (:285), FocusState /
//         WingKind (:157-167).
// LVGL:   N/A (app controller).
// DELTA:  rlvgl's Rc<RefCell> graph collapses to a single-owner tree +
//         non-owning observers (DEMO-00 §5). DiscoController owns the root
//         core::WidgetNode by value and ControllerState by unique_ptr;
//         ControllerState reaches widgets via raw observing Widget* (never
//         WidgetNode*). Move-only. FocusState/WingKind are exposed (rlvgl
//         keeps them private; the SDL-free parity tests read focus()).
//
// docs/disco-demo/06-controller-and-host-target.md (DEMO-06) §3/§4/§5.

#ifndef LVGLPP_APP_DISCO_DEMO_DISCO_CONTROLLER_HPP
#define LVGLPP_APP_DISCO_DEMO_DISCO_CONTROLLER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "lvglpp/app/disco_demo/capabilities.hpp"
#include "lvglpp/app/disco_demo/command.hpp"
#include "lvglpp/core/event.hpp"
#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/platform/screen.hpp"

namespace lvglpp::app::disco_demo {

// Which wing a Wing focus state refers to. FROZEN — mirror lib.rs:157.
enum class WingKind : std::uint8_t {
    Settings,
    Info,
};

// Navigation focus. Either a main-strip slot index, or a wing-slot index
// for one of the two wings. FROZEN — mirror lib.rs:163 FocusState.
//
// Exposed (rlvgl keeps it module-private) so the SDL-free parity suite can
// assert FSM state directly, mirroring rlvgl's in-module focus checks.
struct FocusState {
    enum class Tag : std::uint8_t { Main, Wing };

    Tag         tag   = Tag::Main;
    WingKind    wing  = WingKind::Settings;  // meaningful only when tag==Wing.
    std::size_t index = 0;

    [[nodiscard]] static constexpr FocusState main(std::size_t i) noexcept {
        return FocusState{Tag::Main, WingKind::Settings, i};
    }
    [[nodiscard]] static constexpr FocusState wing_at(WingKind k,
                                                      std::size_t i) noexcept {
        return FocusState{Tag::Wing, k, i};
    }
    [[nodiscard]] constexpr bool is_wing() const noexcept {
        return tag == Tag::Wing;
    }
    [[nodiscard]] constexpr bool operator==(const FocusState&) const noexcept =
        default;
};

// Controller-internal mutable state. Defined in disco_controller.cpp; the
// controller owns it behind a unique_ptr so its address (and therefore the
// observer pointers captured into callbacks) is stable across moves.
struct ControllerState;

// Shared controller that owns the 747-style demo widget tree and command
// queue.
//
// Ownership (DEMO-00 §5 / DEMO-06 §3):
//   root_  — owns the entire widget tree (every widget, via unique_ptr in
//            the WidgetNode children). Sole owner (O-1/O-2).
//   state_ — owns ControllerState (O-2); unique_ptr gives it a stable
//            address so observer pointers + callbacks stay valid (O-4/O-8).
// Move-only: copy is deleted; moving preserves all observers because the
// heap Widget objects and the ControllerState are address-stable.
class DiscoController {
public:
    // make_*: creates and returns ownership (CLAUDE.md naming). Builds the
    // tree, captures observers post-assembly, wires callbacks, and applies
    // the initial focus + home content. Mirror lib.rs:831.
    //
    // Args:
    //   screen: by value; only logical width/height are read (rotation is
    //           the platform layer's concern, mirror lib.rs:829).
    //   caps:   by value; copied into ControllerState.
    // Returns: owns DiscoController via RAII.
    [[nodiscard]] static DiscoController make(platform::Screen screen,
                                              DiscoCapabilities caps);

    DiscoController(const DiscoController&)            = delete;
    DiscoController& operator=(const DiscoController&) = delete;
    DiscoController(DiscoController&&) noexcept;             // move-only (O-8)
    DiscoController& operator=(DiscoController&&) noexcept;
    ~DiscoController();

    // Dispatch an event through the widget tree then the controller FSM.
    // Mirror lib.rs:1245. Returns true iff a widget consumed the event.
    //
    // Args:
    //   event: borrows for the duration of the call.
    [[nodiscard]] bool dispatch_event(const core::Event& event);

    // Handle an event after widget dispatch. Consumes Tick / KeyDown /
    // PressRelease per DEMO-00 §7 E-1. Mirror lib.rs:1252.
    void handle_event(const core::Event& event);

    // Advance one shared demo tick (tree Tick + handle_event Tick).
    // Mirror lib.rs:1284.
    void tick();

    // Drain platform commands queued since the previous call (moves out the
    // queue and clears it). Mirror lib.rs:1290.
    [[nodiscard]] std::vector<DiscoCommand> drain_commands();

    // Surface a runtime-produced status line (footer + event window + queue
    // a ShowStatus command). Mirror lib.rs:1295.
    //
    // Args:
    //   text: by value; consumed into the status surfaces.
    void publish_status(std::string text);

    // Borrow the owned root tree for draw / dispatch (host loop). Mirror
    // lib.rs:1240, but returns a borrow rather than a shared handle
    // (DEMO-00 §9 — single-owner tree).
    [[nodiscard]] core::WidgetNode& root() noexcept { return root_; }
    [[nodiscard]] const core::WidgetNode& root() const noexcept { return root_; }

    // Current navigation focus. Test-support accessor (rlvgl reads the
    // private field from its in-module tests).
    [[nodiscard]] FocusState focus() const noexcept;

private:
    // owns: root widget tree (O-1/O-2).
    core::WidgetNode root_{};
    // owns: controller state behind a stable address (O-2/O-4/O-8).
    std::unique_ptr<ControllerState> state_;

    DiscoController(core::WidgetNode&& root,
                    std::unique_ptr<ControllerState> state) noexcept;
};

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_DISCO_CONTROLLER_HPP
