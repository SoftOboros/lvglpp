#!/usr/bin/env python3
"""Validate extracted release sources with host tests and a Linux consumer link."""
import argparse
import subprocess
from pathlib import Path


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('build', type=Path)
    parser.add_argument('--linux-backends', action='store_true')
    args = parser.parse_args()
    source = args.source.resolve()
    build = args.build.resolve()
    if not args.linux_backends:
        run('cmake', '-S', source, '-B', build, '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Debug', '-DCMAKE_CXX_FLAGS=-Werror',
            '-DLVGLPP_BUILD_EXAMPLES=ON', '-DLVGLPP_CORE_RLE=ON',
            '-DLVGLPP_USE_RLVGL=OFF', '-DLV_FETCH_DEPENDENCIES=OFF',
            '-DCONFIG_LV_USE_THORVG_INTERNAL=OFF')
        run('cmake', '--build', build, '--parallel', '4')
        run('ctest', '--test-dir', build, '--output-on-failure', '--parallel', '4')
        return
    # This is an actual downstream add_subdirectory consumer, with no Git/Rust.
    project = build.parent / (build.name + '-consumer')
    project.mkdir(parents=True)
    config = (source / 'include/lvglpp/lv_conf.h').read_text().replace(
        'LV_COLOR_FORMAT_ARGB8888', 'LV_COLOR_FORMAT_RGB565')
    config = config.replace('#endif /* LV_CONF_H */', '''
#define LVGLPP_RELEASE_CONFIG_MARKER 1234
#define LV_USE_WAYLAND 1
#define LV_WAYLAND_USE_EGL 0
#define LV_WAYLAND_USE_G2D 0
#define LV_WAYLAND_USE_DMABUF 0
#define LV_USE_LINUX_FBDEV 1
#define LV_USE_EVDEV 1
#endif /* LV_CONF_H */
''')
    config_dir = project / 'consumer config'
    config_dir.mkdir()
    (config_dir / 'lv_conf.h').write_text(config)
    (project / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(release_consumer LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(LVGLPP_BUILD_TESTS OFF CACHE BOOL "")
set(LVGLPP_BUILD_EXAMPLES OFF CACHE BOOL "")
set(LVGLPP_USE_RLVGL OFF CACHE BOOL "")
set(LVGLPP_EMBEDDED_POSTURE ON CACHE BOOL "")
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "")
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "")
set(CONFIG_LV_USE_WAYLAND ON CACHE BOOL "")
set(CONFIG_LV_WAYLAND_USE_EGL OFF CACHE BOOL "")
set(CONFIG_LV_WAYLAND_USE_G2D OFF CACHE BOOL "")
set(CONFIG_LV_WAYLAND_USE_DMABUF OFF CACHE BOOL "")
set(CONFIG_LV_USE_EVDEV ON CACHE BOOL "")
set(LV_BUILD_CONF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/consumer config")
add_subdirectory("${LVGLPP_SOURCE_DIR}" lvglpp)
add_executable(consumer_link probe.cpp)
target_link_libraries(consumer_link PRIVATE lvglpp::core)
''')
    (project / 'probe.cpp').write_text('''#include "lvgl.h"
#if LVGLPP_RELEASE_CONFIG_MARKER != 1234
#error Consumer configuration was not propagated
#endif
static_assert(LV_COLOR_FORMAT_DEFAULT == LV_COLOR_FORMAT_RGB565);
int main() {
    lv_init();
    (void)lv_linux_fbdev_create();
    (void)lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event0");
    (void)lv_wayland_get_fd();
}
''')
    run('cmake', '-S', project, '-B', build, '-G', 'Ninja',
        '-DLVGLPP_SOURCE_DIR=' + str(source), '-DCMAKE_CXX_FLAGS=-Werror')
    run('cmake', '--build', build, '--parallel', '4')
    # Link proof only: device/compositor runtime qualification is downstream.


if __name__ == '__main__':
    main()
