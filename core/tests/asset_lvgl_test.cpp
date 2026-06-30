// asset_lvgl_test.cpp - LPAR-CPP-09 acceptance for LVGL-backed asset wrappers.

#include "lvglpp/core/asset_lvgl.hpp"
#include "lvglpp/core/runtime.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>

namespace {

struct MemoryFile {
    // borrows: bytes owned by MemoryFs fixture storage.
    std::span<const std::uint8_t> bytes{};
    std::uint32_t pos = 0;
};

struct MemoryDir {
    std::uint32_t index = 0;
};

struct MemoryFs {
    std::array<std::uint8_t, 5> file{{'h', 'e', 'l', 'l', 'o'}};
    MemoryFile file_handle{};
    MemoryDir dir_handle{};
    bool removed = false;
};

MemoryFs& memory_fs(lv_fs_drv_t* drv) {
    return *static_cast<MemoryFs*>(drv->user_data);
}

bool ready_cb(lv_fs_drv_t* /*drv*/) {
    return true;
}

void remove_cb(lv_fs_drv_t* drv) {
    memory_fs(drv).removed = true;
}

void* open_cb(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode) {
    if ((mode & LV_FS_MODE_RD) == 0 || std::strcmp(path, "/hello.txt") != 0) {
        return nullptr;
    }
    auto& fs = memory_fs(drv);
    fs.file_handle = MemoryFile{
        std::span<const std::uint8_t>(fs.file.data(), fs.file.size()), 0};
    return &fs.file_handle;
}

lv_fs_res_t close_cb(lv_fs_drv_t* /*drv*/, void* /*file*/) {
    return LV_FS_RES_OK;
}

lv_fs_res_t read_cb(lv_fs_drv_t* /*drv*/,
                    void* file,
                    void* buffer,
                    std::uint32_t bytes_to_read,
                    std::uint32_t* bytes_read) {
    auto& handle = *static_cast<MemoryFile*>(file);
    const std::uint32_t available =
        static_cast<std::uint32_t>(handle.bytes.size()) - handle.pos;
    const std::uint32_t count = std::min(bytes_to_read, available);
    std::memcpy(buffer, handle.bytes.data() + handle.pos, count);
    handle.pos += count;
    if (bytes_read != nullptr) {
        *bytes_read = count;
    }
    return LV_FS_RES_OK;
}

lv_fs_res_t seek_cb(lv_fs_drv_t* /*drv*/,
                    void* file,
                    std::uint32_t pos,
                    lv_fs_whence_t whence) {
    auto& handle = *static_cast<MemoryFile*>(file);
    std::uint32_t next = pos;
    if (whence == LV_FS_SEEK_CUR) {
        next = handle.pos + pos;
    } else if (whence == LV_FS_SEEK_END) {
        next = static_cast<std::uint32_t>(handle.bytes.size()) + pos;
    }
    if (next > handle.bytes.size()) {
        return LV_FS_RES_INV_PARAM;
    }
    handle.pos = next;
    return LV_FS_RES_OK;
}

lv_fs_res_t tell_cb(lv_fs_drv_t* /*drv*/, void* file, std::uint32_t* pos) {
    *pos = static_cast<MemoryFile*>(file)->pos;
    return LV_FS_RES_OK;
}

void* dir_open_cb(lv_fs_drv_t* drv, const char* path) {
    if (std::strcmp(path, "/") != 0) {
        return nullptr;
    }
    auto& fs = memory_fs(drv);
    fs.dir_handle = MemoryDir{};
    return &fs.dir_handle;
}

lv_fs_res_t dir_read_cb(lv_fs_drv_t* /*drv*/,
                        void* dir,
                        char* filename,
                        std::uint32_t filename_len) {
    auto& handle = *static_cast<MemoryDir*>(dir);
    if (handle.index > 0) {
        filename[0] = '\0';
        return LV_FS_RES_OK;
    }
    const char* name = "hello.txt";
    std::strncpy(filename, name, filename_len);
    filename[filename_len - 1] = '\0';
    ++handle.index;
    return LV_FS_RES_OK;
}

lv_fs_res_t dir_close_cb(lv_fs_drv_t* /*drv*/, void* /*dir*/) {
    return LV_FS_RES_OK;
}

lvglpp::LvFsDriver make_memory_driver(MemoryFs& fs) {
    lvglpp::LvFsDriver driver{'Z'};
    driver.set_user_data(&fs);
    driver.set_ready_callback(ready_cb);
    driver.set_remove_callback(remove_cb);
    driver.set_open_callback(open_cb);
    driver.set_close_callback(close_cb);
    driver.set_read_callback(read_cb);
    driver.set_seek_callback(seek_cb);
    driver.set_tell_callback(tell_cb);
    driver.set_dir_open_callback(dir_open_cb);
    driver.set_dir_read_callback(dir_read_cb);
    driver.set_dir_close_callback(dir_close_cb);
    driver.register_driver();
    return driver;
}

void test_driver_file_directory_and_paths() {
    MemoryFs fs;
    {
        auto driver = make_memory_driver(fs);
        assert(driver.registered());
        assert(lvglpp::filesystem_driver('Z').borrow_raw() == driver.borrow_raw());
        assert(lvglpp::filesystem_ready('Z'));

        char letters[LV_FS_MAX_FN_LENGTH]{};
        assert(std::strchr(lvglpp::filesystem_letters(letters), 'Z') != nullptr);

        lvglpp::LvFile file;
        assert(file.open_path("Z:/hello.txt", lvglpp::FileMode::Read) == LV_FS_RES_OK);

        std::uint32_t size = 0;
        assert(file.size(size) == LV_FS_RES_OK);
        assert(size == fs.file.size());

        std::uint32_t pos = 0;
        assert(file.tell(pos) == LV_FS_RES_OK);
        assert(pos == 0);

        char buffer[6]{};
        std::uint32_t read = 0;
        assert(file.read(buffer, 5, &read) == LV_FS_RES_OK);
        assert(read == 5);
        assert(std::strcmp(buffer, "hello") == 0);

        assert(file.seek(1, lvglpp::SeekWhence::Set) == LV_FS_RES_OK);
        assert(file.tell(pos) == LV_FS_RES_OK);
        assert(pos == 1);
        assert(file.close() == LV_FS_RES_OK);

        std::uint32_t path_bytes = 0;
        assert(lvglpp::path_size("Z:/hello.txt", path_bytes) == LV_FS_RES_OK);
        assert(path_bytes == fs.file.size());

        char loaded[5]{};
        assert(lvglpp::load_to_buffer(loaded, sizeof(loaded), "Z:/hello.txt") ==
               LV_FS_RES_OK);
        assert(std::memcmp(loaded, fs.file.data(), fs.file.size()) == 0);

        lvglpp::LvDirectory dir;
        assert(dir.open_path("Z:/") == LV_FS_RES_OK);
        char name[32]{};
        assert(dir.read(name, sizeof(name)) == LV_FS_RES_OK);
        assert(std::strcmp(name, "hello.txt") == 0);
        assert(dir.close() == LV_FS_RES_OK);

        char joined[32]{};
        assert(lvglpp::path_join(joined, sizeof(joined), "Z:/dir", "file.png") > 0);
        assert(std::strcmp(joined, "Z:/dir/file.png") == 0);
        assert(std::strcmp(lvglpp::path_extension(joined), "png") == 0);
        assert(std::strcmp(lvglpp::path_last_component(joined), "file.png") == 0);
        assert(std::strcmp(lvglpp::path_up(joined), "Z:/dir") == 0);
    }
    assert(fs.removed);
}

void test_path_from_buffer_and_source_catalog() {
    const std::array<std::uint8_t, 4> bytes{{1, 2, 3, 4}};
    lvglpp::LvPathFromBuffer path{'M', bytes, "bin"};
    assert(path.path()[0] == 'M');

    void* raw = nullptr;
    std::uint32_t size = 0;
    assert(path.buffer(raw, size));
    assert(raw == bytes.data());
    assert(size == bytes.size());

    lvglpp::StaticAssetRecord record{
        "demo.bytes",
        std::span<const std::uint8_t>(bytes.data(), bytes.size()),
        path.path(),
        nullptr,
    };
    assert(std::strcmp(record.id, "demo.bytes") == 0);
    assert(record.bytes.size() == bytes.size());
    assert(record.path == path.path());
}

void test_decoder_owner_and_cache_helpers() {
    auto decoder = lvglpp::LvImageDecoder::make();
    assert(!decoder.empty());
    decoder.set_info_callback(nullptr);
    decoder.set_open_callback(nullptr);
    decoder.set_get_area_callback(nullptr);
    decoder.set_close_callback(nullptr);

    lv_image_decoder_t* raw = decoder.borrow_raw();
    auto moved = std::move(decoder);
    assert(decoder.empty());
    assert(moved.borrow_raw() == raw);

    raw = moved.release();
    assert(moved.empty());
    assert(raw != nullptr);
    lv_image_decoder_delete(raw);

    assert(lvglpp::image_cache_init(4096) == LV_RESULT_OK);
    lvglpp::image_cache_resize(4096, false);
    assert(lvglpp::image_cache_enabled());
    lvglpp::image_cache_drop_all();
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;

    test_driver_file_directory_and_paths();
    test_path_from_buffer_and_source_catalog();
    test_decoder_owner_and_cache_helpers();

    return 0;
}
