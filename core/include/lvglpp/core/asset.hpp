// asset.hpp — typed asset sources + RAII over LVGL's filesystem-driver
// and image-decoder registrations (LPAR-09).
//
// PARITY: rlvgl/core/src/asset.rs + core/src/fs.rs (v0.2.4 @ 343f596).
//         AssetPath mirrors rlvgl's source-kind variant set; the C++
//         wrappers own the LVGL registrations rather than re-implementing
//         a VFS or decoder registry.
// LVGL:   lvgl/src/misc/lv_fs.h (lv_fs_drv_t, lv_fs_drv_init,
//         lv_fs_drv_register), lvgl/src/draw/lv_image_decoder.h
//         (lv_image_decoder_create / _delete / _set_info_cb /
//         _set_open_cb).
// DELTA:  FsDriver owns the lv_fs_drv_t storage as a member because LVGL
//         saves only the pointer and ships no lv_fs_drv_unregister; the
//         registration therefore persists for the process and the storage
//         MUST outlive it. See docs/core-asset/00-asset-filesystem.md §5.

#ifndef LVGLPP_CORE_ASSET_HPP
#define LVGLPP_CORE_ASSET_HPP

#include <cstdint>

#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core {

// Frozen mirror of rlvgl's asset source kinds (rlvgl/core/src/asset.rs
// AssetPath; v0.2.4 @ 343f596). Standards Action to add a value — it must
// agree with rlvgl. See docs/core-asset/00-asset-filesystem.md §3/§5.
enum class AssetPath : std::uint8_t {
    Embedded  = 0,  // C array / lv_image_dsc_t baked into the binary.
    Fatfs     = 1,  // FATFS path behind an LVGL drive letter (CORE-07 LVGLPP_CORE_FATFS).
    Simulator = 2,  // Host file accessed through a simulator fs driver.
    Memory    = 3,  // In-RAM blob (lv_fs memory-mapped path / pointer).
};

// Errors the ImageDecoder factory can produce. (FsDriver does not use this:
// its only failure mode — a non-'A'..'Z' drive letter — aborts/throws at
// construction, because the type is pinned and cannot return an expected.)
enum class AssetError : std::uint8_t {
    DecoderCreateFailed = 1,  // lv_image_decoder_create returned nullptr (OOM).
};

// RAII owner of a registered lv_image_decoder_t. This is the seam through
// which CORE-07 / CORE-07n decoders (e.g. the RLE decoder) plug in: create
// one, attach info/open callbacks, and let it live for as long as the
// decoder should be registered.
//
// Ownership: owns its lv_image_decoder_t. The decoder is the resource;
// the destructor calls lv_image_decoder_delete iff still owning. Move-only;
// the moved-from ImageDecoder is left empty so its destructor is a no-op.
// Empty-safe: every accessor is a no-op / safe default when empty.
class ImageDecoder {
public:
    // Create and register a fresh decoder with no callbacks attached.
    // Returns: owns ImageDecoder on success; DecoderCreateFailed on OOM.
    [[nodiscard]] static lvglpp::expected<ImageDecoder, AssetError> try_make() noexcept;

    // Throwing convenience over try_make: aborts under embedded posture,
    // throws std::runtime_error on host, on failure. Mirrors Object::make.
    [[nodiscard]] static ImageDecoder make();

    ImageDecoder(const ImageDecoder&)            = delete;
    ImageDecoder& operator=(const ImageDecoder&) = delete;

    // Move-construct only: transfers the decoder handle. The lv_image_-
    // decoder_t* is the whole resource, so nothing else needs rebinding.
    // Move-assignment is deleted (mirrors Object).
    ImageDecoder(ImageDecoder&& other) noexcept;
    ImageDecoder& operator=(ImageDecoder&&) = delete;

    ~ImageDecoder();

    // Attach the info / open callbacks. No-ops when empty.
    //   info_cb / open_cb: external function pointers; LVGL invokes them
    //                      while this decoder is registered. The pointed-to
    //                      code (and any data it dereferences) MUST outlive
    //                      this ImageDecoder.
    void set_info_cb(lv_image_decoder_info_f_t info_cb) noexcept;
    void set_open_cb(lv_image_decoder_open_f_t open_cb) noexcept;

    // borrows: valid only while this ImageDecoder owns a live decoder.
    [[nodiscard]] lv_image_decoder_t* borrow_raw() const noexcept { return decoder_; }
    [[nodiscard]] bool                empty()      const noexcept { return decoder_ == nullptr; }

private:
    // Adopt an already-created decoder (used by try_make). May be nullptr.
    explicit ImageDecoder(lv_image_decoder_t* decoder) noexcept : decoder_{decoder} {}

    // owns: the LVGL decoder. Nulled on move-out; deleted by the dtor.
    lv_image_decoder_t* decoder_ = nullptr;
};

// RAII over an lv_fs_drv_t registration (lv_fs_drv_init + lv_fs_drv_register).
//
// Ownership: owns the lv_fs_drv_t *storage* — it lives as a member, not on
// the heap, because lv_fs_drv_register saves only the pointer (lv_fs.h:124
// "Only pointer is saved, so the driver should be static or dynamically
// allocated"). LVGL 9.6 ships NO lv_fs_drv_unregister / _remove / _delete
// (confirmed by grep over lvgl/src), so a registered driver persists for
// the life of the process. This wrapper therefore does NOT pretend to
// de-register on destruction: the contract is that an FsDriver must outlive
// any LVGL use of its drive letter — in practice it is a long-lived / static
// object. Destroying it frees only the C++ wrapper; LVGL still holds the
// now-dangling pointer, so do not destroy an FsDriver while LVGL may still
// resolve its letter.
//
// Pinned and non-movable. Because LVGL stores &drv_ at register time and
// offers no way to drop it, the storage cannot relocate after registration:
// a move would dangle the LVGL-held pointer. The driver is therefore
// registered *in place* by the constructor (which runs on the object's
// final address) and the type is neither copyable nor movable. Allocate an
// FsDriver where it will live for as long as its drive letter is used —
// typically a long-lived or static object.
//
// This shape is dictated by the missing unregister API; do not "fix" it by
// adding a returning factory, which a non-movable type cannot satisfy.
class FsDriver {
public:
    // Initialise (lv_fs_drv_init) and register (lv_fs_drv_register) a driver
    // for `drive_letter`, in place. `ready_cb` is the LVGL readiness probe
    // (may be nullptr). Other callbacks are left at their lv_fs_drv_init
    // defaults; callers needing open/read/close attach them via borrow_raw()
    // before first use.
    //   drive_letter: must be in 'A'..'Z'. An invalid letter aborts under
    //                 embedded posture / throws std::runtime_error on host
    //                 (mirrors the make() factories elsewhere in core).
    //   ready_cb:     external function pointer; must outlive this FsDriver.
    FsDriver(char drive_letter, bool (*ready_cb)(lv_fs_drv_t* drv));

    FsDriver(const FsDriver&)            = delete;
    FsDriver& operator=(const FsDriver&) = delete;
    // Move is deleted: LVGL holds &drv_ from registration time, so the
    // storage cannot relocate. The object is pinned at its construction
    // site. See the class comment on the missing unregister API.
    FsDriver(FsDriver&&)            = delete;
    FsDriver& operator=(FsDriver&&) = delete;

    ~FsDriver();

    [[nodiscard]] char drive_letter() const noexcept { return drv_.letter; }
    [[nodiscard]] bool registered()   const noexcept { return registered_; }

    // borrows the owned lv_fs_drv_t for callback attachment. The returned
    // pointer is the exact pointer LVGL holds; valid for this object's life.
    [[nodiscard]] lv_fs_drv_t* borrow_raw() noexcept { return &drv_; }

private:
    // owns: the driver storage. LVGL holds &drv_ after registration and has
    // no API to drop it, so this member must outlive any LVGL use of the
    // drive letter. Marked registered_ once lv_fs_drv_register has run.
    lv_fs_drv_t drv_{};
    bool        registered_ = false;
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_ASSET_HPP
