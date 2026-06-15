// asset.cpp — ImageDecoder + FsDriver implementation (LPAR-09).
//
// PARITY: rlvgl/core/src/asset.rs + core/src/fs.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/draw/lv_image_decoder.c (create/delete/set_*_cb),
//         lvgl/src/misc/lv_fs.c (lv_fs_drv_init / lv_fs_drv_register).
// DELTA:  see asset.hpp — FsDriver owns lv_fs_drv_t storage because LVGL
//         ships no unregister; FsDriver is therefore pinned (non-movable).

#include "lvglpp/core/asset.hpp"

#include <utility>  // std::move

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>  // std::abort
#else
#  include <stdexcept>  // std::runtime_error
#endif

namespace lvglpp::core {

// --- ImageDecoder ---

lvglpp::expected<ImageDecoder, AssetError> ImageDecoder::try_make() noexcept {
    lv_image_decoder_t* decoder = lv_image_decoder_create();
    if (decoder == nullptr) {
        return lvglpp::unexpected<AssetError>{AssetError::DecoderCreateFailed};
    }
    return ImageDecoder{decoder};
}

ImageDecoder ImageDecoder::make() {
    auto result = try_make();
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::ImageDecoder::make: lv_image_decoder_create failed");
#endif
    }
    return std::move(result.value());
}

ImageDecoder::ImageDecoder(ImageDecoder&& other) noexcept : decoder_{other.decoder_} {
    other.decoder_ = nullptr;
}

ImageDecoder::~ImageDecoder() {
    if (decoder_ != nullptr) {
        lv_image_decoder_delete(decoder_);
    }
}

void ImageDecoder::set_info_cb(lv_image_decoder_info_f_t info_cb) noexcept {
    if (decoder_ != nullptr) {
        lv_image_decoder_set_info_cb(decoder_, info_cb);
    }
}

void ImageDecoder::set_open_cb(lv_image_decoder_open_f_t open_cb) noexcept {
    if (decoder_ != nullptr) {
        lv_image_decoder_set_open_cb(decoder_, open_cb);
    }
}

// --- FsDriver ---

FsDriver::FsDriver(char drive_letter, bool (*ready_cb)(lv_fs_drv_t* drv)) {
    if (drive_letter < 'A' || drive_letter > 'Z') {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::FsDriver: drive letter must be 'A'..'Z'");
#endif
    }
    lv_fs_drv_init(&drv_);
    drv_.letter   = drive_letter;
    drv_.ready_cb = ready_cb;
    // SAFETY: lv_fs_drv_register saves &drv_ (lv_fs.h:124 — "Only pointer is
    //   saved"). drv_ is a member of this object, which is non-movable
    //   (move/copy deleted in asset.hpp) and constructed in place, so the
    //   registered pointer stays valid for this object's lifetime. There is
    //   no lv_fs_drv_unregister in LVGL 9.6 to undo the registration.
    lv_fs_drv_register(&drv_);
    registered_ = true;
}

FsDriver::~FsDriver() {
    // No lv_fs_drv_unregister exists in LVGL 9.6 (confirmed by grep over
    // lvgl/src). If this FsDriver was registered, LVGL still holds &drv_
    // after destruction. The class contract (asset.hpp) requires an
    // FsDriver to outlive any LVGL use of its drive letter, so destroying
    // it is only safe once the letter is no longer resolved. Nothing to
    // release here.
}

}  // namespace lvglpp::core
