// PARITY: N/A — toolchain seam, no rlvgl counterpart (Rust's
//         panic=abort posture is the moral equivalent).
// LVGL:   N/A.
// DELTA:  N/A.
//
// Embedded-posture shims (CLAUDE.md § "Embedded posture mirrors
// rlvgl": throwing paths call abort-equivalents instead).
//
// The prebuilt libstdc++ archive is compiled WITH exceptions: pulling
// any std::__throw_* member from functexcept.o drags in the
// exception-allocation machinery (eh_alloc.o → malloc → _sbrk →
// newlib syscall stubs), none of which can link in this bare-metal
// image. std::visit / std::get on a std::variant reference
// __throw_bad_variant_access statically even though a -fno-exceptions
// program can never reach it (a variant cannot become valueless
// without exceptions in flight).
//
// Defining the symbols here makes the linker prefer this object file
// over the archive member, severing the chain. Each lands on a bkpt
// trap — consistent with the wait_until()-timeout trap() in
// clocks.cpp.
//
// SAFETY:
//   Adding definitions to namespace std is formally reserved, but
//   these match the libstdc++ declarations (bits/functexcept.h)
//   exactly and exist precisely so the library's own definitions are
//   never loaded. No object identity or ABI variance is possible:
//   the functions never return.

namespace {

[[noreturn]] void trap() noexcept {
    for (;;) asm volatile ("bkpt 1");
}

} // namespace

namespace std {

[[noreturn]] void __throw_bad_variant_access(const char*) { trap(); }
[[noreturn]] void __throw_bad_optional_access() { trap(); }
[[noreturn]] void __throw_bad_function_call() { trap(); }
[[noreturn]] void __throw_bad_alloc() { trap(); }
[[noreturn]] void __throw_length_error(const char*) { trap(); }
[[noreturn]] void __throw_out_of_range_fmt(const char*, ...) { trap(); }

// _GLIBCXX_ASSERTIONS failure hook (Debug builds): the libstdc++
// definition fprintf's to stderr — another stdio/heap pull.
[[noreturn]] void __glibcxx_assert_fail(const char*, int,
                                        const char*, const char*) {
    trap();
}

} // namespace std

// newlib's abort() raises SIGABRT through the reentrant signal
// machinery (_kill/_getpid) and falls back to _exit — none of which
// exist here. Overriding it keeps libc_a-abort.o (and the libnosys
// stub farm behind it) out of the link.
extern "C" [[noreturn]] void abort() { trap(); }
