// asset_lvgl.cpp - LVGL-backed filesystem, decoder, cache, and asset wrappers.
//
// PARITY: rlvgl/docs/concepts/LPAR-09-ASSET-FILESYSTEM.md
//         (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_fs.c, lvgl/src/draw/lv_image_decoder.c,
//         and lvgl/src/misc/cache/instance/lv_image_cache.c.
// DELTA:  delegates source dispatch, decoder registration, and cache policy to LVGL.

#include "lvglpp/core/asset_lvgl.hpp"

#include <utility>

namespace lvglpp {

LvFsDriver::LvFsDriver() : raw_{std::make_unique<lv_fs_drv_t>()} {
    lv_fs_drv_init(raw_.get());
}

LvFsDriver::LvFsDriver(char letter) : LvFsDriver{} {
    set_letter(letter);
}

LvFsDriver::LvFsDriver(LvFsDriver&& other) noexcept
    : raw_{std::move(other.raw_)}, registered_{other.registered_} {
    other.registered_ = false;
}

LvFsDriver& LvFsDriver::operator=(LvFsDriver&& other) noexcept {
    if (this != &other) {
        reset();
        raw_ = std::move(other.raw_);
        registered_ = other.registered_;
        other.registered_ = false;
    }
    return *this;
}

LvFsDriver::~LvFsDriver() {
    reset();
}

FsDriverView LvFsDriver::borrow() const noexcept {
    return FsDriverView{raw_.get()};
}

char LvFsDriver::letter() const noexcept {
    return raw_ == nullptr ? '\0' : raw_->letter;
}

void LvFsDriver::set_letter(char letter) noexcept {
    if (raw_ != nullptr && !registered_) {
        raw_->letter = letter;
    }
}

void LvFsDriver::set_cache_size(std::uint32_t cache_size) noexcept {
    if (raw_ != nullptr && !registered_) {
        raw_->cache_size = cache_size;
    }
}

void LvFsDriver::set_user_data(void* user_data) noexcept {
    if (raw_ != nullptr) {
        raw_->user_data = user_data;
    }
}

void LvFsDriver::set_ready_callback(bool (*callback)(lv_fs_drv_t*)) noexcept {
    if (raw_ != nullptr) raw_->ready_cb = callback;
}

void LvFsDriver::set_remove_callback(void (*callback)(lv_fs_drv_t*)) noexcept {
    if (raw_ != nullptr) raw_->remove_cb = callback;
}

void LvFsDriver::set_open_callback(
    void* (*callback)(lv_fs_drv_t*, const char*, lv_fs_mode_t)) noexcept {
    if (raw_ != nullptr) raw_->open_cb = callback;
}

void LvFsDriver::set_close_callback(
    lv_fs_res_t (*callback)(lv_fs_drv_t*, void*)) noexcept {
    if (raw_ != nullptr) raw_->close_cb = callback;
}

void LvFsDriver::set_read_callback(
    lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, void*, std::uint32_t, std::uint32_t*)) noexcept {
    if (raw_ != nullptr) raw_->read_cb = callback;
}

void LvFsDriver::set_write_callback(
    lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, const void*, std::uint32_t, std::uint32_t*)) noexcept {
    if (raw_ != nullptr) raw_->write_cb = callback;
}

void LvFsDriver::set_seek_callback(
    lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, std::uint32_t, lv_fs_whence_t)) noexcept {
    if (raw_ != nullptr) raw_->seek_cb = callback;
}

void LvFsDriver::set_tell_callback(
    lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, std::uint32_t*)) noexcept {
    if (raw_ != nullptr) raw_->tell_cb = callback;
}

void LvFsDriver::set_dir_open_callback(
    void* (*callback)(lv_fs_drv_t*, const char*)) noexcept {
    if (raw_ != nullptr) raw_->dir_open_cb = callback;
}

void LvFsDriver::set_dir_read_callback(
    lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, char*, std::uint32_t)) noexcept {
    if (raw_ != nullptr) raw_->dir_read_cb = callback;
}

void LvFsDriver::set_dir_close_callback(
    lv_fs_res_t (*callback)(lv_fs_drv_t*, void*)) noexcept {
    if (raw_ != nullptr) raw_->dir_close_cb = callback;
}

void LvFsDriver::register_driver() noexcept {
    if (raw_ != nullptr && !registered_) {
        lv_fs_drv_register(raw_.get());
        registered_ = true;
    }
}

void LvFsDriver::reset() noexcept {
    if (raw_ != nullptr && registered_) {
        if (raw_->remove_cb != nullptr) {
            raw_->remove_cb(raw_.get());
        }
        (void)raw_.release();
        registered_ = false;
        return;
    }
    raw_.reset();
}

FsDriverView filesystem_driver(char letter) noexcept {
    return FsDriverView{lv_fs_get_drv(letter)};
}

bool filesystem_ready(char letter) noexcept {
    return lv_fs_is_ready(letter);
}

LvFile::LvFile(LvFile&& other) noexcept
    : raw_{other.raw_}, open_{other.open_} {
    other.raw_ = lv_fs_file_t{};
    other.open_ = false;
}

LvFile& LvFile::operator=(LvFile&& other) noexcept {
    if (this != &other) {
        (void)close();
        raw_ = other.raw_;
        open_ = other.open_;
        other.raw_ = lv_fs_file_t{};
        other.open_ = false;
    }
    return *this;
}

LvFile::~LvFile() {
    (void)close();
}

FsResult LvFile::open_path(const char* path, FileMode mode) noexcept {
    (void)close();
    const FsResult result = lv_fs_open(&raw_, path, to_lv(mode));
    open_ = result == LV_FS_RES_OK;
    return result;
}

FsResult LvFile::close() noexcept {
    if (!open_) {
        return LV_FS_RES_INV_PARAM;
    }
    const FsResult result = lv_fs_close(&raw_);
    open_ = false;
    return result;
}

FsResult LvFile::read(void* buffer,
                      std::uint32_t bytes_to_read,
                      std::uint32_t* bytes_read) noexcept {
    return lv_fs_read(&raw_, buffer, bytes_to_read, bytes_read);
}

FsResult LvFile::write(const void* buffer,
                       std::uint32_t bytes_to_write,
                       std::uint32_t* bytes_written) noexcept {
    return lv_fs_write(&raw_, buffer, bytes_to_write, bytes_written);
}

FsResult LvFile::seek(std::uint32_t pos, SeekWhence whence) noexcept {
    return lv_fs_seek(&raw_, pos, to_lv(whence));
}

FsResult LvFile::tell(std::uint32_t& pos) noexcept {
    return lv_fs_tell(&raw_, &pos);
}

FsResult LvFile::size(std::uint32_t& size) noexcept {
    return lv_fs_get_size(&raw_, &size);
}

LvDirectory::LvDirectory(LvDirectory&& other) noexcept
    : raw_{other.raw_}, open_{other.open_} {
    other.raw_ = lv_fs_dir_t{};
    other.open_ = false;
}

LvDirectory& LvDirectory::operator=(LvDirectory&& other) noexcept {
    if (this != &other) {
        (void)close();
        raw_ = other.raw_;
        open_ = other.open_;
        other.raw_ = lv_fs_dir_t{};
        other.open_ = false;
    }
    return *this;
}

LvDirectory::~LvDirectory() {
    (void)close();
}

FsResult LvDirectory::open_path(const char* path) noexcept {
    (void)close();
    const FsResult result = lv_fs_dir_open(&raw_, path);
    open_ = result == LV_FS_RES_OK;
    return result;
}

FsResult LvDirectory::read(char* filename, std::uint32_t filename_len) noexcept {
    return lv_fs_dir_read(&raw_, filename, filename_len);
}

FsResult LvDirectory::close() noexcept {
    if (!open_) {
        return LV_FS_RES_INV_PARAM;
    }
    const FsResult result = lv_fs_dir_close(&raw_);
    open_ = false;
    return result;
}

LvPathFromBuffer::LvPathFromBuffer(char letter,
                                   std::span<const std::uint8_t> buffer,
                                   const char* ext) noexcept {
    lv_fs_make_path_from_buffer(&raw_,
                                letter,
                                buffer.data(),
                                static_cast<std::uint32_t>(buffer.size()),
                                ext);
}

bool LvPathFromBuffer::buffer(void*& out_buffer,
                              std::uint32_t& out_size) noexcept {
    return lv_fs_get_buffer_from_path(&raw_, &out_buffer, &out_size) == LV_RESULT_OK;
}

const char* path_extension(const char* path) noexcept {
    return lv_fs_get_ext(path);
}

const char* path_last_component(const char* path) noexcept {
    return lv_fs_get_last(path);
}

char* path_up(char* path) noexcept {
    return lv_fs_up(path);
}

int path_join(char* buffer,
              std::size_t buffer_size,
              const char* base,
              const char* end) noexcept {
    return lv_fs_path_join(buffer, buffer_size, base, end);
}

char* filesystem_letters(char* buffer) noexcept {
    return lv_fs_get_letters(buffer);
}

FsResult path_size(const char* path, std::uint32_t& size) noexcept {
    return lv_fs_path_get_size(path, &size);
}

FsResult load_to_buffer(void* buffer,
                        std::uint32_t buffer_size,
                        const char* path) noexcept {
    return lv_fs_load_to_buf(buffer, buffer_size, path);
}

LvImageDecoder LvImageDecoder::make() noexcept {
    return LvImageDecoder{lv_image_decoder_create()};
}

LvImageDecoder::LvImageDecoder(LvImageDecoder&& other) noexcept
    : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvImageDecoder& LvImageDecoder::operator=(LvImageDecoder&& other) noexcept {
    if (this != &other) {
        reset();
        raw_ = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvImageDecoder::~LvImageDecoder() {
    reset();
}

void LvImageDecoder::set_info_callback(
    lv_image_decoder_info_f_t callback) noexcept {
    if (raw_ != nullptr) lv_image_decoder_set_info_cb(raw_, callback);
}

void LvImageDecoder::set_open_callback(
    lv_image_decoder_open_f_t callback) noexcept {
    if (raw_ != nullptr) lv_image_decoder_set_open_cb(raw_, callback);
}

void LvImageDecoder::set_get_area_callback(
    lv_image_decoder_get_area_cb_t callback) noexcept {
    if (raw_ != nullptr) lv_image_decoder_set_get_area_cb(raw_, callback);
}

void LvImageDecoder::set_close_callback(
    lv_image_decoder_close_f_t callback) noexcept {
    if (raw_ != nullptr) lv_image_decoder_set_close_cb(raw_, callback);
}

lv_image_decoder_t* LvImageDecoder::release() noexcept {
    lv_image_decoder_t* released = raw_;
    raw_ = nullptr;
    return released;
}

void LvImageDecoder::reset() noexcept {
    if (raw_ != nullptr) {
        lv_image_decoder_delete(raw_);
        raw_ = nullptr;
    }
}

lv_result_t image_cache_init(std::uint32_t size_bytes) noexcept {
    return lv_image_cache_init(size_bytes);
}

void image_cache_resize(std::uint32_t size_bytes, bool evict_now) noexcept {
    lv_image_cache_resize(size_bytes, evict_now);
}

void image_cache_drop(const void* source) noexcept {
    lv_image_cache_drop(source);
}

void image_cache_drop(ImageDescriptorView descriptor) noexcept {
    lv_image_cache_drop(descriptor.borrow_raw());
}

void image_cache_drop_all() noexcept {
    lv_image_cache_drop(nullptr);
}

bool image_cache_enabled() noexcept {
    return lv_image_cache_is_enabled();
}

}  // namespace lvglpp
