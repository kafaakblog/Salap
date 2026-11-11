#include "salap/core.h"

#include <string>

#include "chars.h"

namespace salap {

using detail::digit_value;
using detail::is_ascii_digit;
using detail::is_digit;
using detail::is_thai_digit;

// ---- Numerals ---------------------------------------------------------------

std::u32string convert_numerals(std::u32string_view text, NumeralDirection direction) {
    if (direction == NumeralDirection::Auto) {
        size_t arabic = 0, thai = 0;
        for (char32_t c : text) {
            if (is_ascii_digit(c)) ++arabic;
            else if (is_thai_digit(c)) ++thai;
        }
        direction = (arabic > 0 && arabic >= thai) ? NumeralDirection::ToThai
                                                   : NumeralDirection::ToArabic;
    }
    std::u32string out(text);
    for (char32_t& c : out) {
        if (direction == NumeralDirection::ToThai && is_ascii_digit(c)) {
            c = 0x0E50 + (c - U'0');
        } else if (direction == NumeralDirection::ToArabic && is_thai_digit(c)) {
            c = U'0' + (c - 0x0E50);
        }
    }
    return out;
}

// ---- Baht text ----------------------------------------------------------------

namespace {

const char32_t* const kDigitWord[10] = {U"ศูนย์", U"หนึ่ง", U"สอง", U"สาม", U"สี่",
                                        U"ห้า",  U"หก",   U"เจ็ด", U"แปด", U"เก้า"};
const char32_t* const kPlaceWord[6] = {U"", U"สิบ", U"ร้อย", U"พัน", U"หมื่น", U"แสน"};

// Words for up to six digits (a value below one million). `has_higher` says
// whether a non-zero part of the number precedes these digits, which decides
// หนึ่ง versus เอ็ด for a final 1.
std::u32string group_words(const std::string& digits, bool has_higher) {
    std::u32string out;
    bool seen_nonzero = has_higher;
    int n = static_cast<int>(digits.size());
    for (int i = 0; i < n; ++i) {
        int d = digits[i] - '0';
        int place = n - 1 - i;
        if (d == 0) continue;
        if (place == 0 && d == 1 && seen_nonzero) {
            out += U"เอ็ด";
        } else if (place == 1 && d == 1) {
            out += U"สิบ";
        } else if (place == 1 && d == 2) {
            out += U"ยี่สิบ";
        } else {
            out += kDigitWord[d];
            out += kPlaceWord[place];
        }
        seen_nonzero = true;
    }
    return out;
}

bool all_zero(const std::string& s) {
    for (char c : s)
        if (c != '0') return false;
    return true;
}

// Words for a non-negative integer of any length, given without leading zeros.
std::u32string integer_words(const std::string& digits) {
    if (digits.size() <= 6) return group_words(digits, false);
    std::string head = digits.substr(0, digits.size() - 6);
    std::string tail = digits.substr(digits.size() - 6);
    std::u32string out = integer_words(head) + U"ล้าน";
    if (!all_zero(tail)) out += group_words(tail, true);
    return out;
}

// Adds one to a decimal string.
void increment(std::string& digits) {
    for (size_t i = digits.size(); i-- > 0;) {
        if (digits[i] != '9') {
            ++digits[i];
            return;
        }
        digits[i] = '0';
    }
    digits.insert(digits.begin(), '1');
}

bool is_space(char32_t c) {
    return c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == 0x00A0;
}

}  // namespace

bool baht_text(std::u32string_view number, std::u32string& out) {
    size_t b = 0, e = number.size();
    while (b < e && is_space(number[b])) ++b;
    while (e > b && is_space(number[e - 1])) --e;

    bool negative = false;
    if (b < e && (number[b] == U'-' || number[b] == 0x2212)) {
        negative = true;
        ++b;
    } else if (b < e && number[b] == U'+') {
        ++b;
    }
    if (b < e && number[b] == 0x0E3F) ++b;  // ฿
    while (b < e && is_space(number[b])) ++b;

    std::string whole, frac;
    bool in_frac = false;
    bool any_digit = false;
    for (size_t i = b; i < e; ++i) {
        char32_t c = number[i];
        if (is_digit(c)) {
            (in_frac ? frac : whole).push_back(static_cast<char>('0' + digit_value(c)));
            any_digit = true;
        } else if (c == U',' && !in_frac) {
            // A grouping comma must sit between digits.
            if (whole.empty() || i + 1 >= e || !is_digit(number[i + 1])) return false;
        } else if (c == U'.' && !in_frac) {
            in_frac = true;
        } else {
            return false;
        }
    }
    if (!any_digit) return false;

    // Round to two decimal places, half up.
    bool round_up = frac.size() > 2 && frac[2] >= '5';
    frac.resize(2, '0');
    size_t first = whole.find_first_not_of('0');
    whole = first == std::string::npos ? "0" : whole.substr(first);
    if (round_up) {
        increment(frac);
        if (frac.size() > 2) {  // .995 and up carries into the baht
            frac = "00";
            increment(whole);
        }
    }

    bool whole_zero = whole == "0";
    bool frac_zero = frac == "00";

    out.clear();
    if (negative && !(whole_zero && frac_zero)) out += U"ลบ";
    if (whole_zero && frac_zero) {
        out += U"ศูนย์บาทถ้วน";
        return true;
    }
    if (!whole_zero) {
        out += integer_words(whole);
        out += U"บาท";
    }
    if (frac_zero) {
        out += U"ถ้วน";
    } else {
        out += group_words(frac, false);
        out += U"สตางค์";
    }
    return true;
}

// ---- Baht text back to a number -------------------------------------------------

namespace {

bool eat(std::u32string_view s, size_t& i, std::u32string_view word) {
    if (s.substr(i, word.size()) != word) return false;
    i += word.size();
    return true;
}

// Reads Thai number words starting at i. Stops at the first word that is not
// part of a number. False if there was no number or it is too large.
bool parse_words(std::u32string_view s, size_t& i, unsigned long long& value) {
    static const std::u32string_view kDigits[] = {U"ศูนย์", U"หนึ่ง", U"สอง", U"สาม", U"สี่",
                                                  U"ห้า",  U"หก",   U"เจ็ด", U"แปด", U"เก้า"};
    static const std::u32string_view kPlaces[] = {U"สิบ", U"ร้อย", U"พัน", U"หมื่น", U"แสน"};
    unsigned long long total = 0, group = 0;
    int digit = -1;
    bool any = false;
    for (;;) {
        bool matched = false;
        for (int d = 0; d < 10 && !matched; ++d) {
            if (eat(s, i, kDigits[d])) {
                digit = d;
                matched = true;
            }
        }
        if (!matched && eat(s, i, U"เอ็ด")) {
            digit = 1;
            matched = true;
        }
        if (!matched && eat(s, i, U"ยี่")) {
            digit = 2;
            matched = true;
        }
        if (!matched) {
            unsigned long long place = 10;
            for (int p = 0; p < 5 && !matched; ++p, place *= 10) {
                if (eat(s, i, kPlaces[p])) {
                    group += (digit < 0 ? 1 : digit) * place;
                    digit = -1;
                    matched = true;
                }
            }
        }
        if (!matched && eat(s, i, U"ล้าน")) {
            if (digit >= 0) group += digit;
            digit = -1;
            total += group;
            group = 0;
            if (total > 999999999999ULL) return false;  // keeps the result under 10^18
            total *= 1000000ULL;
            matched = true;
        }
        if (!matched) break;
        any = true;
    }
    if (digit >= 0) group += digit;
    value = total + group;
    return any;
}

}  // namespace

bool parse_baht_text(std::u32string_view words, std::u32string& number_out) {
    size_t b = 0, e = words.size();
    while (b < e && is_space(words[b])) ++b;
    while (e > b && is_space(words[e - 1])) --e;
    std::u32string_view s = words.substr(b, e - b);

    size_t i = 0;
    bool negative = eat(s, i, U"ลบ");
    unsigned long long baht = 0, satang = 0, value = 0;
    if (!parse_words(s, i, value)) return false;
    if (eat(s, i, U"บาท")) {
        baht = value;
        if (!eat(s, i, U"ถ้วน")) {
            if (!parse_words(s, i, satang) || !eat(s, i, U"สตางค์")) return false;
        }
    } else if (eat(s, i, U"สตางค์")) {
        satang = value;
    } else {
        return false;
    }
    if (i != s.size() || satang > 99) return false;

    std::string digits = std::to_string(baht);
    std::u32string num;
    if (negative) num += U'-';
    for (size_t k = 0; k < digits.size(); ++k) {
        if (k > 0 && (digits.size() - k) % 3 == 0) num += U',';
        num += static_cast<char32_t>(digits[k]);
    }
    if (satang) {
        num += U'.';
        num += static_cast<char32_t>(U'0' + satang / 10);
        num += static_cast<char32_t>(U'0' + satang % 10);
    }

    // Accept only text that reads back exactly the same, so that loosely
    // matching phrases are never turned into a wrong number.
    std::u32string check;
    if (!baht_text(num, check) || check != s) return false;
    number_out = num;
    return true;
}

// ---- Years ----------------------------------------------------------------------

namespace {

const char32_t kPho = 0x0E1E;  // พ
const char32_t kKho = 0x0E04;  // ค
const char32_t kSo = 0x0E28;   // ศ

bool is_blank(char32_t c) { return c == U' ' || c == 0x00A0 || c == U'\t'; }

// Looks backwards from `end` for an era label: พ.ศ. พ.ศ พศ. พศ (or the same
// with ค), optionally followed by spaces. Returns the index of its first
// letter, or npos.
size_t era_label_before(const std::u32string& s, size_t end) {
    size_t p = end;
    while (p > 0 && is_blank(s[p - 1])) --p;
    if (p > 0 && s[p - 1] == U'.') --p;
    if (p == 0 || s[p - 1] != kSo) return std::u32string::npos;
    --p;
    if (p > 0 && s[p - 1] == U'.') --p;
    if (p == 0 || (s[p - 1] != kPho && s[p - 1] != kKho)) return std::u32string::npos;
    --p;
    // "พศ" at the end of a longer Thai word is not a label.
    if (p > 0 && detail::is_thai_letter(s[p - 1])) return std::u32string::npos;
    return p;
}

// A run of exactly four digits in one script starting at i, not part of a
// longer or decimal number. Returns its value, or -1.
int four_digit_year(const std::u32string& s, size_t i) {
    size_t n = s.size();
    if (i + 4 > n) return -1;
    bool thai = is_thai_digit(s[i]);
    int value = 0;
    for (size_t k = i; k < i + 4; ++k) {
        if (!is_digit(s[k]) || is_thai_digit(s[k]) != thai) return -1;
        value = value * 10 + digit_value(s[k]);
    }
    if (i + 4 < n && is_digit(s[i + 4])) return -1;
    if (i + 5 < n && (s[i + 4] == U',' || s[i + 4] == U'.') && is_digit(s[i + 5])) return -1;
    return value;
}

}  // namespace

namespace detail {

std::u32string convert_years_counted(std::u32string_view text, int* to_ce, int* to_be) {
    std::u32string out(text);
    int n_ce = 0, n_be = 0;
    bool any_thai_digit = false;
    for (char32_t c : out)
        if (is_thai_digit(c)) any_thai_digit = true;

    auto write_year = [&](size_t at, int value, bool thai) {
        char32_t zero = thai ? char32_t(0x0E50) : U'0';
        for (size_t k = at + 4; k-- > at;) {
            out[k] = zero + (value % 10);
            value /= 10;
        }
    };

    for (size_t i = 0; i < out.size(); ++i) {
        if (!is_digit(out[i]) || (i > 0 && is_digit(out[i - 1]))) continue;  // start of a digit run
        size_t label = era_label_before(out, i);
        if (label == std::u32string::npos) continue;
        int value = four_digit_year(out, i);
        if (value < 0) continue;

        const bool be_to_ce = out[label] == kPho;
        const int shift = be_to_ce ? -543 : 543;
        if (value + shift < 1 || value + shift > 9999) continue;
        // ค.ศ. years are always written in Arabic digits. พ.ศ. years follow
        // the text around them: Thai digits if the text uses any.
        const bool thai_out = !be_to_ce && any_thai_digit;

        write_year(i, value + shift, thai_out);
        out[label] = be_to_ce ? kKho : kPho;
        (be_to_ce ? n_ce : n_be)++;

        // A range: พ.ศ. 2540-2569. The second year has no label of its own.
        size_t j = i + 4;
        while (j < out.size() && is_blank(out[j])) ++j;
        if (j < out.size() && (out[j] == U'-' || out[j] == 0x2013 || out[j] == 0x2014)) {
            ++j;
            while (j < out.size() && is_blank(out[j])) ++j;
            int second = four_digit_year(out, j);
            if (second >= 0 && second + shift >= 1 && second + shift <= 9999) {
                write_year(j, second + shift, thai_out);
                (be_to_ce ? n_ce : n_be)++;
                i = j + 3;
            }
        }
    }
    if (to_ce) *to_ce = n_ce;
    if (to_be) *to_be = n_be;
    return out;
}

}  // namespace detail

std::u32string convert_years(std::u32string_view text, int* converted) {
    int a = 0, b = 0;
    std::u32string out = detail::convert_years_counted(text, &a, &b);
    if (converted) *converted = a + b;
    return out;
}

}  // namespace salap
