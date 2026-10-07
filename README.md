# lvglpp

Modern-C++ wrapper around [LVGL](https://github.com/lvgl/lvgl), with the
same target surface as the Rust [rlvgl](https://github.com/SoftOboros/rlvgl)
project. The two live as parallel submodules in this tree; agents and
contributors should read [`CLAUDE.md`](./CLAUDE.md) before making changes.

The project is built around a **strict and explicit ownership** discipline
inspired by Rust's borrow rules but expressed in idiomatic C++20: every
pointer, reference, handle, and buffer carries an explicit ownership tag,
and every ownership transfer is visible at the call site.

## Layout

```
lvglpp/
├── CMakeLists.txt
├── CLAUDE.md            # ownership discipline + agent runbook
├── include/lvglpp/      # public headers
├── src/                 # implementation
├── tests/               # host-side tests
├── examples/            # board / desktop demos (added per target)
├── lvgl/                # upstream LVGL submodule (built)
└── rlvgl/               # softoboros/rlvgl submodule (reference, not built)
```

## Cloning

`rlvgl/` is intentionally non-recursive — pull only the top-level
submodules:

```bash
git clone git@github.com:SoftOboros/lvglpp.git
cd lvglpp
git submodule update --init lvgl rlvgl   # NO --recursive
```

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Requires CMake >= 3.20 and a C++20 toolchain.

## Consumer and Buildroot builds

Only the `lvgl/` submodule is required to build. Consumers that do not
need the Rust parity reference can initialize just that checkout:

```bash
git submodule update --init lvgl
```

Supply your existing configuration with `LV_BUILD_CONF_PATH` (a header
filename) or `LV_BUILD_CONF_DIR` (a directory containing `lv_conf.h`).
These are upstream LVGL options and work both with `-D` and as variables
set before `add_subdirectory(lvglpp)`. Set only one and prefer absolute paths.
With neither set, lvglpp uses `include/lvglpp/lv_conf.h`; it does not
overwrite a supplied configuration. Upstream `LV_BUILD_USE_KCONFIG` is
also passed through without supplying that fallback header.

For a library-only cross build using a Buildroot-generated CMake toolchain:

```bash
cmake -S . -B build-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/output/host/share/buildroot/toolchainfile.cmake \
  -DLV_BUILD_CONF_PATH=/path/to/DisplayManager/lv_conf.h \
  -DLVGLPP_BUILD_LIBRARIES=ON \
  -DLVGLPP_BUILD_TESTS=OFF \
  -DLVGLPP_BUILD_EXAMPLES=OFF \
  -DLVGLPP_USE_RLVGL=OFF \
  -DLVGLPP_EMBEDDED_POSTURE=ON \
  -DLV_FETCH_DEPENDENCIES=OFF
cmake --build build-target -j
```

`LVGLPP_BUILD_LIBRARIES` defaults to `ON` except for `Generic` targets,
where it defaults to `OFF` to preserve the minimal DISCO example build.
Set it to `ON` for freestanding wrapper-library builds. Cross builds skip
host tests and require embedded posture for the wrapper libraries.
Disabling examples also defaults upstream `CONFIG_LV_BUILD_EXAMPLES` and
`CONFIG_LV_BUILD_DEMOS` to `OFF`; explicit upstream settings take precedence.

Keep downstream Wayland, Linux FBDEV, and EVDEV selections in your own
`lv_conf.h` (`LV_USE_WAYLAND`, `LV_USE_LINUX_FBDEV`, `LV_USE_EVDEV`).
LVGL's CMake dependency selection uses matching `CONFIG_LV_*` variables:
for a Wayland header, pass `-DCONFIG_LV_USE_WAYLAND=ON` and any matching
Wayland suboptions; for an EVDEV header, pass `-DCONFIG_LV_USE_EVDEV=ON`.
LVGL then resolves Wayland/xkbcommon or libevdev from the target toolchain
and pkg-config environment. FBDEV uses Linux headers without an extra
library dependency. Keep header and CMake selections consistent; lvglpp
does not force backend features. See the pinned
[`LVGL dependency setup`](lvgl/env_support/cmake/dependencies.cmake).
The wrapper's separate `LVGLPP_PLATFORM_LINUX_FBDEV` option selects its
own platform adapter and is not needed when DisplayManager owns the drivers.

Buildroot needs host CMake **3.20 or newer**, a target C++ compiler and
standard library with **C++20** support, and the selected backend's target
development dependencies. Use the toolchain/sysroot and target pkg-config
settings supplied by Buildroot. Embedded posture disables exceptions and
RTTI; it does not eliminate the C++ standard library requirement.
Keep the LVGL widget features used by the selected wrapper modules enabled.
Applications embedding the tree can link `lvgl::lvgl` for the C library
or `lvglpp::core` / `lvglpp::lvglpp` for the wrapper surface. Build all
consumers against the same configuration header to preserve the LVGL ABI.
DisplayManager can continue building LVGL inside its existing package;
this integration does not require a separate Buildroot package.
