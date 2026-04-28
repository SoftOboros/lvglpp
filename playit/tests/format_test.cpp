// format_test.cpp — PLAYIT-04b acceptance: every Response variant
// produces the §5.1 wire bytes plus §5.2 CRLF; truncation matches
// rlvgl on short buffers; INT32_MIN renders without UB.

#include "lvglpp/playit/format.hpp"
#include "lvglpp/playit/response.hpp"

#include <array>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

using namespace lvglpp::playit;

namespace {

// Format `resp` into a 256-byte stack buffer and return the line
// (without CRLF for easier assertions).
std::string format_line(const Response& resp) {
    std::array<char, 256> buf{};
    std::size_t n = format_response(resp, std::span<char>{buf.data(), buf.size()});
    assert(n >= 2);  // CRLF at minimum
    assert(buf[n - 2] == '\r');
    assert(buf[n - 1] == '\n');
    return std::string{buf.data(), n - 2};
}

void test_ok() {
    assert(format_line(Response{response::Ok{}}) == "OK");
}

void test_error() {
    assert(format_line(Response{response::Error{"bad tap"}}) == "ERR: bad tap");
}

void test_bounds_positive() {
    Response r{response::Bounds{10, 20, 30, 40}};
    assert(format_line(r) == "BOUNDS:10,20,30,40");
}

void test_bounds_negative() {
    Response r{response::Bounds{-5, -7, 30, 40}};
    assert(format_line(r) == "BOUNDS:-5,-7,30,40");
}

void test_exists_true_false() {
    assert(format_line(Response{response::Exists{true}})  == "EXISTS:1");
    assert(format_line(Response{response::Exists{false}}) == "EXISTS:0");
}

void test_child_count() {
    assert(format_line(Response{response::ChildCount{0}})    == "CHILDREN:0");
    assert(format_line(Response{response::ChildCount{7}})    == "CHILDREN:7");
    assert(format_line(Response{response::ChildCount{12345}}) == "CHILDREN:12345");
}

void test_status() {
    Response r{response::Status{StatusData{1234, 567}}};
    assert(format_line(r) == "STAT:1234,567");
}

void test_dump_end() {
    assert(format_line(Response{response::DumpEnd{}}) == "END");
}

// INT32_MIN edge case: -2147483648 must render without UB. The
// negation x → -x overflows a signed int; the formatter routes
// through unsigned to sidestep.
void test_bounds_int32_min() {
    Response r{response::Bounds{INT32_MIN, 0, 0, 0}};
    auto line = format_line(r);
    // Expected literal: "-2147483648"
    assert(line == "BOUNDS:-2147483648,0,0,0");
}

// Short buffer truncates silently; return value never exceeds buffer size.
void test_short_buffer_truncates() {
    std::array<char, 4> small{};
    std::size_t n = format_response(Response{response::Bounds{1, 2, 3, 4}},
                                    std::span<char>{small.data(), small.size()});
    assert(n == small.size());
    assert(small[0] == 'B');
    assert(small[1] == 'O');
    assert(small[2] == 'U');
    assert(small[3] == 'N');
}

void test_zero_buffer_no_writes() {
    std::array<char, 1> zero{};  // intentionally not used
    (void)zero;
    std::size_t n = format_response(Response{response::Ok{}},
                                    std::span<char>{});
    assert(n == 0);
}

}  // namespace

int main() {
    test_ok();
    test_error();
    test_bounds_positive();
    test_bounds_negative();
    test_exists_true_false();
    test_child_count();
    test_status();
    test_dump_end();
    test_bounds_int32_min();
    test_short_buffer_truncates();
    test_zero_buffer_no_writes();
    return 0;
}
