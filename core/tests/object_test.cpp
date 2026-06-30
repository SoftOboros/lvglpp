// object_test.cpp - LPAR-CPP-02 acceptance for LVGL-backed object owners.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <utility>

namespace {

void test_make_screen_and_child() {
    auto screen = lvglpp::LvObject::make_screen();
    assert(screen.valid());
    assert(screen.parent().empty());
    assert(screen.child_count() == 0);

    auto child = lvglpp::LvObject::make_child(screen.borrow());
    assert(child.valid());
    assert(screen.child_count() == 1);
    assert(screen.child(0).borrow_raw() == child.borrow_raw());
    assert(screen.child(-1).borrow_raw() == child.borrow_raw());
    assert(child.parent().borrow_raw() == screen.borrow_raw());
}

void test_empty_child_factory_stays_empty() {
    auto child = lvglpp::LvObject::make_child(lvglpp::ObjectView{nullptr});
    assert(child.empty());
    assert(!child.valid());
}

void test_release_disarms_deletion() {
    auto screen = lvglpp::LvObject::make_screen();
    lv_obj_t* raw = screen.release();

    assert(screen.empty());
    assert(raw != nullptr);
    assert(lv_obj_is_valid(raw));

    lv_obj_delete(raw);
}

void test_move_transfers_ownership() {
    auto source = lvglpp::LvObject::make_screen();
    lv_obj_t* raw = source.borrow_raw();

    lvglpp::LvObject dest{std::move(source)};
    assert(source.empty());
    assert(dest.borrow_raw() == raw);
    assert(dest.valid());

    auto replacement = lvglpp::LvObject::make_screen();
    raw = replacement.borrow_raw();
    dest = std::move(replacement);
    assert(replacement.empty());
    assert(dest.borrow_raw() == raw);
    assert(dest.valid());
}

void test_flags_and_states() {
    auto screen = lvglpp::LvObject::make_screen();
    auto child  = lvglpp::LvObject::make_child(screen.borrow());

    child.add_flag(lvglpp::ObjectFlag::Hidden);
    assert(child.has_flag(lvglpp::ObjectFlag::Hidden));
    child.remove_flag(lvglpp::ObjectFlag::Hidden);
    assert(!child.has_flag(lvglpp::ObjectFlag::Hidden));
    child.set_flag(lvglpp::ObjectFlag::Clickable, true);
    assert(child.has_flag(lvglpp::ObjectFlag::Clickable));
    child.set_flag(lvglpp::ObjectFlag::Clickable, false);
    assert(!child.has_flag(lvglpp::ObjectFlag::Clickable));

    child.add_state(lvglpp::ObjectState::Pressed);
    assert(child.has_state(lvglpp::ObjectState::Pressed));
    child.remove_state(lvglpp::ObjectState::Pressed);
    assert(!child.has_state(lvglpp::ObjectState::Pressed));
    child.set_state(lvglpp::ObjectState::Checked, true);
    assert(child.has_state(lvglpp::ObjectState::Checked));
    child.set_state(lvglpp::ObjectState::Checked, false);
    assert(!child.has_state(lvglpp::ObjectState::Checked));
}

void test_reparent_and_order() {
    auto first_parent = lvglpp::LvObject::make_screen();
    auto next_parent  = lvglpp::LvObject::make_screen();
    auto a            = lvglpp::LvObject::make_child(first_parent.borrow());
    auto b            = lvglpp::LvObject::make_child(first_parent.borrow());

    assert(first_parent.child_count() == 2);
    assert(first_parent.child(0).borrow_raw() == a.borrow_raw());
    assert(first_parent.child(1).borrow_raw() == b.borrow_raw());

    a.raise_to_front();
    assert(first_parent.child(0).borrow_raw() == b.borrow_raw());
    assert(first_parent.child(1).borrow_raw() == a.borrow_raw());

    a.lower_to_back();
    assert(first_parent.child(0).borrow_raw() == a.borrow_raw());
    assert(first_parent.child(1).borrow_raw() == b.borrow_raw());

    b.set_parent(next_parent.borrow());
    assert(first_parent.child_count() == 1);
    assert(next_parent.child_count() == 1);
    assert(next_parent.child(0).borrow_raw() == b.borrow_raw());
    assert(b.parent().borrow_raw() == next_parent.borrow_raw());
}

void test_parent_deletion_invalidates_child_owner() {
    lvglpp::LvObject child;
    {
        auto screen = lvglpp::LvObject::make_screen();
        child       = lvglpp::LvObject::make_child(screen.borrow());
        assert(child.valid());
    }

    assert(!child.valid());
    child.reset();
    assert(child.empty());
}

void test_clean_children_invalidates_child_owner() {
    auto screen = lvglpp::LvObject::make_screen();
    auto child  = lvglpp::LvObject::make_child(screen.borrow());

    assert(screen.child_count() == 1);
    screen.clean_children();
    assert(screen.child_count() == 0);
    assert(!child.valid());
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;
    auto display = lvglpp::LvDisplay::make(480, 320);
    display.set_default();

    test_make_screen_and_child();
    test_empty_child_factory_stays_empty();
    test_release_disarms_deletion();
    test_move_transfers_ownership();
    test_flags_and_states();
    test_reparent_and_order();
    test_parent_deletion_invalidates_child_owner();
    test_clean_children_invalidates_child_owner();

    return 0;
}
