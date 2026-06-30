// asset_lvgl.hpp - LVGL-backed filesystem, decoder, cache, and asset wrappers.
//
// PARITY: rlvgl/docs/concepts/LPAR-09-ASSET-FILESYSTEM.md
//         (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_fs.h, lvgl/src/draw/lv_image_decoder.h,
//         and lvgl/src/misc/cache/instance/lv_image_cache.h.
// DELTA:  lvglpp delegates filesystem dispatch, image decoder registration,
//         and image cache policy to LVGL instead of porting rlvgl's Rust
//         AssetRegistry/ImageCache runtime.

#ifndef LVGLPP_CORE_ASSET_LVGL_HPP
#define LVGLPP_CORE_ASSET_LVGL_HPP

#include "lvglpp/core/draw_lvgl.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

extern "C" {
#include "src/draw/lv_image_decoder.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "src/misc/lv_fs.h"
}

namespace lvglpp {

using FsResult = lv_fs_res_t;

enum class FileMode : std::uint8_t {
    Write = LV_FS_MODE_WR,
    Read = LV_FS_MODE_RD,
    ReadWrite = LV_FS_MODE_WR | LV_FS_MODE_RD,
};

[[nodiscard]] constexpr lv_fs_mode_t to_lv(FileMode mode) noexcept {
    return static_cast<lv_fs_mode_t>(mode);
}

enum class SeekWhence : std::uint8_t {
    Set = LV_FS_SEEK_SET,
    Current = LV_FS_SEEK_CUR,
    End = LV_FS_SEEK_END,
};

[[nodiscard]] constexpr lv_fs_whence_t to_lv(SeekWhence whence) noexcept {
    return static_cast<lv_fs_whence_t>(whence);
}

class FsDriverView {
public:
    // Args:
    //   raw: observes external or registered LVGL filesystem driver storage.
    explicit FsDriverView(lv_fs_drv_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_fs_drv_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool         empty() const noexcept { return raw_ == nullptr; }

private:
    // observes: driver storage owned outside this view; never removed here.
    lv_fs_drv_t* raw_ = nullptr;
};

class LvFsDriver {
public:
    LvFsDriver();
    explicit LvFsDriver(char letter);

    LvFsDriver(const LvFsDriver&)            = delete;
    LvFsDriver& operator=(const LvFsDriver&) = delete;

    LvFsDriver(LvFsDriver&& other) noexcept;
    LvFsDriver& operator=(LvFsDriver&& other) noexcept;

    ~LvFsDriver();

    [[nodiscard]] FsDriverView borrow() const noexcept;
    [[nodiscard]] lv_fs_drv_t* borrow_raw() noexcept { return raw_.get(); }
    [[nodiscard]] const lv_fs_drv_t* borrow_raw() const noexcept { return raw_.get(); }
    [[nodiscard]] bool empty() const noexcept { return raw_ == nullptr; }
    [[nodiscard]] bool registered() const noexcept { return registered_; }
    [[nodiscard]] char letter() const noexcept;

    void set_letter(char letter) noexcept;
    void set_cache_size(std::uint32_t cache_size) noexcept;
    // Args:
    //   user_data: external; observed by LVGL callbacks stored in raw_.
    void set_user_data(void* user_data) noexcept;

    void set_ready_callback(bool (*callback)(lv_fs_drv_t*)) noexcept;
    void set_remove_callback(void (*callback)(lv_fs_drv_t*)) noexcept;
    void set_open_callback(
        void* (*callback)(lv_fs_drv_t*, const char*, lv_fs_mode_t)) noexcept;
    void set_close_callback(lv_fs_res_t (*callback)(lv_fs_drv_t*, void*)) noexcept;
    void set_read_callback(
        lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, void*, std::uint32_t, std::uint32_t*)) noexcept;
    void set_write_callback(
        lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, const void*, std::uint32_t, std::uint32_t*)) noexcept;
    void set_seek_callback(
        lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, std::uint32_t, lv_fs_whence_t)) noexcept;
    void set_tell_callback(
        lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, std::uint32_t*)) noexcept;
    void set_dir_open_callback(void* (*callback)(lv_fs_drv_t*, const char*)) noexcept;
    void set_dir_read_callback(
        lv_fs_res_t (*callback)(lv_fs_drv_t*, void*, char*, std::uint32_t)) noexcept;
    void set_dir_close_callback(lv_fs_res_t (*callback)(lv_fs_drv_t*, void*)) noexcept;

    void register_driver() noexcept;
    // Safe unregister is blocked by LVGL v9 lv_fs_remove_drive continuing
    // iteration after removing a matching node. reset() calls remove_cb and
    // preserves registered driver storage until lv_deinit clears the registry.
    void reset() noexcept;

private:
    // owns: stable driver storage before registration. Once registered, LVGL
    // observes this storage from its driver list; reset() may intentionally
    // release it to avoid a dangling LVGL pointer when unregister is blocked.
    std::unique_ptr<lv_fs_drv_t> raw_;
    bool registered_ = false;
};

[[nodiscard]] FsDriverView filesystem_driver(char letter) noexcept;
[[nodiscard]] bool filesystem_ready(char letter) noexcept;

class LvFile {
public:
    LvFile() noexcept = default;

    LvFile(const LvFile&)            = delete;
    LvFile& operator=(const LvFile&) = delete;

    LvFile(LvFile&& other) noexcept;
    LvFile& operator=(LvFile&& other) noexcept;

    ~LvFile();

    [[nodiscard]] lv_fs_file_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_fs_file_t* borrow_raw() const noexcept { return &raw_; }
    [[nodiscard]] bool open() const noexcept { return open_; }

    // Args:
    //   path: external null-terminated LVGL path; borrowed for call only.
    [[nodiscard]] FsResult open_path(const char* path, FileMode mode) noexcept;
    [[nodiscard]] FsResult close() noexcept;
    [[nodiscard]] FsResult read(void* buffer,
                                std::uint32_t bytes_to_read,
                                std::uint32_t* bytes_read = nullptr) noexcept;
    [[nodiscard]] FsResult write(const void* buffer,
                                 std::uint32_t bytes_to_write,
                                 std::uint32_t* bytes_written = nullptr) noexcept;
    [[nodiscard]] FsResult seek(std::uint32_t pos, SeekWhence whence) noexcept;
    [[nodiscard]] FsResult tell(std::uint32_t& pos) noexcept;
    [[nodiscard]] FsResult size(std::uint32_t& size) noexcept;

private:
    // owns: open LVGL file session while open_.
    lv_fs_file_t raw_{};
    bool open_ = false;
};

class LvDirectory {
public:
    LvDirectory() noexcept = default;

    LvDirectory(const LvDirectory&)            = delete;
    LvDirectory& operator=(const LvDirectory&) = delete;

    LvDirectory(LvDirectory&& other) noexcept;
    LvDirectory& operator=(LvDirectory&& other) noexcept;

    ~LvDirectory();

    [[nodiscard]] bool open() const noexcept { return open_; }
    // Args:
    //   path: external null-terminated LVGL path; borrowed for call only.
    [[nodiscard]] FsResult open_path(const char* path) noexcept;
    [[nodiscard]] FsResult read(char* filename,
                                std::uint32_t filename_len) noexcept;
    [[nodiscard]] FsResult close() noexcept;

private:
    // owns: open LVGL directory session while open_.
    lv_fs_dir_t raw_{};
    bool open_ = false;
};

class LvPathFromBuffer {
public:
    LvPathFromBuffer() noexcept = default;
    // Args:
    //   buffer: external; LVGL path observes these bytes when opened.
    //   ext: external optional extension string; borrowed for call only.
    LvPathFromBuffer(char letter,
                     std::span<const std::uint8_t> buffer,
                     const char* ext = nullptr) noexcept;

    [[nodiscard]] const lv_fs_path_ex_t* borrow_raw() const noexcept {
        return &raw_;
    }
    [[nodiscard]] lv_fs_path_ex_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const char* path() const noexcept { return raw_.path; }
    [[nodiscard]] bool buffer(void*& out_buffer,
                              std::uint32_t& out_size) noexcept;

private:
    // owns: path object value. The encoded path observes external buffer bytes.
    lv_fs_path_ex_t raw_{};
};

[[nodiscard]] const char* path_extension(const char* path) noexcept;
[[nodiscard]] const char* path_last_component(const char* path) noexcept;
char* path_up(char* path) noexcept;
int path_join(char* buffer,
              std::size_t buffer_size,
              const char* base,
              const char* end) noexcept;
char* filesystem_letters(char* buffer) noexcept;
[[nodiscard]] FsResult path_size(const char* path, std::uint32_t& size) noexcept;
[[nodiscard]] FsResult load_to_buffer(void* buffer,
                                      std::uint32_t buffer_size,
                                      const char* path) noexcept;

class LvImageDecoder {
public:
    LvImageDecoder() noexcept = default;

    [[nodiscard]] static LvImageDecoder make() noexcept;

    LvImageDecoder(const LvImageDecoder&)            = delete;
    LvImageDecoder& operator=(const LvImageDecoder&) = delete;

    LvImageDecoder(LvImageDecoder&& other) noexcept;
    LvImageDecoder& operator=(LvImageDecoder&& other) noexcept;

    ~LvImageDecoder();

    [[nodiscard]] lv_image_decoder_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool empty() const noexcept { return raw_ == nullptr; }

    void set_info_callback(lv_image_decoder_info_f_t callback) noexcept;
    void set_open_callback(lv_image_decoder_open_f_t callback) noexcept;
    void set_get_area_callback(lv_image_decoder_get_area_cb_t callback) noexcept;
    void set_close_callback(lv_image_decoder_close_f_t callback) noexcept;

    // Returns: owns raw LVGL image decoder; caller must delete it.
    [[nodiscard]] lv_image_decoder_t* release() noexcept;
    void reset() noexcept;

private:
    explicit LvImageDecoder(lv_image_decoder_t* raw) noexcept : raw_{raw} {}

    // owns: deleted with lv_image_decoder_delete() while non-null.
    lv_image_decoder_t* raw_ = nullptr;
};

[[nodiscard]] lv_result_t image_cache_init(std::uint32_t size_bytes) noexcept;
void image_cache_resize(std::uint32_t size_bytes, bool evict_now) noexcept;
void image_cache_drop(const void* source) noexcept;
void image_cache_drop(ImageDescriptorView descriptor) noexcept;
void image_cache_drop_all() noexcept;
[[nodiscard]] bool image_cache_enabled() noexcept;

struct StaticAssetRecord {
    // external: stable asset identifier string, commonly static storage.
    const char* id = nullptr;
    // borrows: asset bytes owned by generated/static catalog storage.
    std::span<const std::uint8_t> bytes{};
    // external: optional stable LVGL file/symbol path string.
    const char* path = nullptr;
    // observes: optional LVGL image descriptor owned by catalog storage.
    const lv_image_dsc_t* descriptor = nullptr;
};

}  // namespace lvglpp

#endif  // LVGLPP_CORE_ASSET_LVGL_HPP
