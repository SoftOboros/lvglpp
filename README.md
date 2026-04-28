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
