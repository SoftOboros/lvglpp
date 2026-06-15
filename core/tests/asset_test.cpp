// asset_test.cpp — LPAR-09 acceptance: ImageDecoder + FsDriver RAII over
// LVGL's lv_image_decoder_t / lv_fs_drv_t registrations.
//
// See docs/core-asset/00-asset-filesystem.md §12.

#include "lvglpp/core/asset.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::Runtime;
using lvglpp::core::AssetPath;
using lvglpp::core::FsDriver;
using lvglpp::core::ImageDecoder;

namespace {

// --- Tiny stub callbacks ---------------------------------------------------

// Trivial decoder info/open stubs: report failure so LVGL never tries to
// actually decode. They exist only to prove the callbacks can be attached.
lv_result_t stub_info(lv_image_decoder_t* /*decoder*/, lv_image_decoder_dsc_t* /*dsc*/,
                      lv_image_header_t* /*header*/) {
    return LV_RESULT_INVALID;
}

lv_result_t stub_open(lv_image_decoder_t* /*decoder*/, lv_image_decoder_dsc_t* /*dsc*/) {
    return LV_RESULT_INVALID;
}

// Stub readiness probe for the filesystem driver: always "not ready" so the
// drive resolves but never serves a file.
bool stub_ready(lv_fs_drv_t* /*drv*/) {
    return false;
}

// --- ImageDecoder ----------------------------------------------------------

void test_image_decoder() {
    {
        auto dec = ImageDecoder::try_make();
        assert(dec.has_value());
        ImageDecoder decoder = std::move(dec.value());
        assert(!decoder.empty());
        decoder.set_info_cb(&stub_info);
        decoder.set_open_cb(&stub_open);
        // decoder destructs here -> lv_image_decoder_delete.
    }
    {
        // Create + destruct again to prove clean, repeatable teardown.
        ImageDecoder decoder = ImageDecoder::make();
        assert(!decoder.empty());
    }
}

// Empty-ImageDecoder safety: a moved-from decoder is empty and its
// accessors are no-ops.
void test_image_decoder_empty_safe() {
    ImageDecoder decoder = ImageDecoder::make();
    ImageDecoder moved    = std::move(decoder);
    (void)moved;
    assert(decoder.empty());
    assert(decoder.borrow_raw() == nullptr);
    decoder.set_info_cb(&stub_info);  // no-op, must not crash
    decoder.set_open_cb(&stub_open);  // no-op, must not crash
}

// --- FsDriver --------------------------------------------------------------

void test_fs_driver() {
    // Register a driver for drive 'Z' with a stub ready_cb. FsDriver is
    // pinned (non-movable): construct it directly as a local. Registration
    // succeeds (no crash); LVGL ships no lv_fs_drv_unregister, so the
    // registration persists past this scope — FsDriver only frees its own
    // C++ wrapper on destruction. That is safe here because nothing resolves
    // drive 'Z' after this function returns.
    FsDriver driver('Z', &stub_ready);
    assert(driver.registered());
    assert(driver.drive_letter() == 'Z');
    assert(driver.borrow_raw() != nullptr);
    assert(driver.borrow_raw()->letter == 'Z');
}

// --- AssetPath -------------------------------------------------------------

// The frozen mirror enum exists and its values are distinct (Standards
// Action set; docs/core-asset/00-asset-filesystem.md §3/§5).
void test_asset_path_enum() {
    assert(static_cast<std::uint8_t>(AssetPath::Embedded) == 0U);
    assert(static_cast<std::uint8_t>(AssetPath::Fatfs) == 1U);
    assert(static_cast<std::uint8_t>(AssetPath::Simulator) == 2U);
    assert(static_cast<std::uint8_t>(AssetPath::Memory) == 3U);
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    test_image_decoder();
    test_image_decoder_empty_safe();
    test_fs_driver();
    test_asset_path_enum();

    return 0;
}
