// Small character classification helpers shared by the core sources.
#pragma once

#include <string>
#include <string_view>

namespace salap::detail {

inline bool is_ascii_digit(char32_t c) { return c >= U'0' && c <= U'9'; }
inline bool is_thai_digit(char32_t c) { return c >= 0x0E50 && c <= 0x0E59; }
inline bool is_digit(char32_t c) { return is_ascii_digit(c) || is_thai_digit(c); }
inline int digit_value(char32_t c) {
    return is_ascii_digit(c) ? static_cast<int>(c - U'0') : static_cast<int>(c - 0x0E50);
}

inline bool is_ascii_letter(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
}

// Thai consonants, vowels and marks: everything in the Thai block that is
// not a digit, the baht sign, or the two end-of-text punctuation marks.
inline bool is_thai_letter(char32_t c) {
    return c >= 0x0E01 && c <= 0x0E4E && c != 0x0E3F;
}

// convert_years, also reporting how many years went each way.
std::u32string convert_years_counted(std::u32string_view text, int* to_ce, int* to_be);

}  // namespace salap::detail
