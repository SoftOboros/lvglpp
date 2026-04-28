// lvglpp_expected.hpp — minimal std::expected polyfill.
//
// PARITY: C++23 std::expected (P0323R12).
// SCOPE:  see ./README.md — this is a deliberately small subset; do not
//         grow it without writing a parity test against std::expected.
//
// This header lives under third_party/ because it is intended to be
// removed once the project's toolchain floor includes <expected>. The
// seam in include/lvglpp/std/expected.hpp selects between this and the
// standard library at compile time. Consumers should write
// `lvglpp::expected<T, E>` and never reach into this namespace.

#ifndef LVGLPP_EXPECTED_HPP
#define LVGLPP_EXPECTED_HPP

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace lvglpp::detail {

struct unexpect_t {
    explicit constexpr unexpect_t() = default;
};
inline constexpr unexpect_t unexpect{};

// unexpected<E> — single-value tag carrying an error payload.
//
// Args:
//   e: owns E; consumed into the unexpected.
template <class E>
class unexpected {
public:
    constexpr explicit unexpected(const E& e) : err_(e) {}
    constexpr explicit unexpected(E&& e) noexcept(std::is_nothrow_move_constructible_v<E>)
        : err_(std::move(e)) {}

    constexpr const E& error() const& noexcept { return err_; }
    constexpr E&       error() &      noexcept { return err_; }
    constexpr E&&      error() &&     noexcept { return std::move(err_); }

private:
    // owns: the error payload; freed when this unexpected is destroyed.
    E err_;
};

template <class E>
unexpected(E) -> unexpected<E>;

// expected<T, E> — sum type holding either a T (success) or an E (error).
//
// Hand-rolled storage avoids std::variant's index/runtime tag, lets us
// give T and E the same in-place addresses, and keeps codegen tight on
// embedded targets where this header is included into hot paths.
//
// Constraints (deliberate, see README):
//   - T and E must be distinct, complete object types.
//   - No void / reference T specialization.
//   - No monadic ops (and_then/or_else/transform/transform_error).
template <class T, class E>
class expected {
    static_assert(!std::is_reference_v<T>, "expected<T&, E> not supported");
    static_assert(!std::is_void_v<T>,      "expected<void, E> not supported");

public:
    using value_type      = T;
    using error_type      = E;
    using unexpected_type = unexpected<E>;

    // Default-construct as a value-initialized T.
    constexpr expected()
        noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::is_default_constructible_v<T>
        : has_value_(true) {
        ::new (static_cast<void*>(&storage_.val)) T();
    }

    // Args: v: owns T; consumed.
    constexpr expected(const T& v) : has_value_(true) {
        ::new (static_cast<void*>(&storage_.val)) T(v);
    }
    constexpr expected(T&& v)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        : has_value_(true) {
        ::new (static_cast<void*>(&storage_.val)) T(std::move(v));
    }

    // Args: u: owns unexpected<E>; consumed.
    constexpr expected(const unexpected<E>& u) : has_value_(false) {
        ::new (static_cast<void*>(&storage_.err)) E(u.error());
    }
    constexpr expected(unexpected<E>&& u)
        noexcept(std::is_nothrow_move_constructible_v<E>)
        : has_value_(false) {
        ::new (static_cast<void*>(&storage_.err)) E(std::move(u).error());
    }

    // In-place error construction: lvglpp::expected<T,E>(lvglpp::unexpect, e).
    template <class... Args>
    constexpr expected(unexpect_t, Args&&... args) : has_value_(false) {
        ::new (static_cast<void*>(&storage_.err)) E(std::forward<Args>(args)...);
    }

    constexpr expected(const expected& other) : has_value_(other.has_value_) {
        if (has_value_) ::new (static_cast<void*>(&storage_.val)) T(other.storage_.val);
        else            ::new (static_cast<void*>(&storage_.err)) E(other.storage_.err);
    }

    constexpr expected(expected&& other)
        noexcept(std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_move_constructible_v<E>)
        : has_value_(other.has_value_) {
        if (has_value_) ::new (static_cast<void*>(&storage_.val)) T(std::move(other.storage_.val));
        else            ::new (static_cast<void*>(&storage_.err)) E(std::move(other.storage_.err));
    }

    constexpr expected& operator=(const expected& other) {
        if (this == &other) return *this;
        destroy();
        has_value_ = other.has_value_;
        if (has_value_) ::new (static_cast<void*>(&storage_.val)) T(other.storage_.val);
        else            ::new (static_cast<void*>(&storage_.err)) E(other.storage_.err);
        return *this;
    }

    constexpr expected& operator=(expected&& other)
        noexcept(std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_move_constructible_v<E>) {
        if (this == &other) return *this;
        destroy();
        has_value_ = other.has_value_;
        if (has_value_) ::new (static_cast<void*>(&storage_.val)) T(std::move(other.storage_.val));
        else            ::new (static_cast<void*>(&storage_.err)) E(std::move(other.storage_.err));
        return *this;
    }

    ~expected() { destroy(); }

    constexpr bool has_value() const noexcept { return has_value_; }
    constexpr explicit operator bool() const noexcept { return has_value_; }

    constexpr T&       value() &      noexcept { return storage_.val; }
    constexpr const T& value() const& noexcept { return storage_.val; }
    constexpr T&&      value() &&     noexcept { return std::move(storage_.val); }

    constexpr E&       error() &      noexcept { return storage_.err; }
    constexpr const E& error() const& noexcept { return storage_.err; }
    constexpr E&&      error() &&     noexcept { return std::move(storage_.err); }

    template <class U>
    constexpr T value_or(U&& alt) const& {
        return has_value_ ? storage_.val : static_cast<T>(std::forward<U>(alt));
    }
    template <class U>
    constexpr T value_or(U&& alt) && {
        return has_value_ ? std::move(storage_.val) : static_cast<T>(std::forward<U>(alt));
    }

private:
    void destroy() noexcept {
        if (has_value_) storage_.val.~T();
        else            storage_.err.~E();
    }

    union Storage {
        // owns T xor E depending on has_value_; lifetime managed by destroy().
        Storage() {}
        ~Storage() {}
        T val;
        E err;
    };

    Storage storage_;
    bool    has_value_;
};

}  // namespace lvglpp::detail

#endif  // LVGLPP_EXPECTED_HPP
