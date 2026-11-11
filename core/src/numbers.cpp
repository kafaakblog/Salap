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

std::u32string convert_years(std::u32string_view text, int* converted) {
    static const std::u32string_view kBE = U"พ.ศ.";
    static const std::u32string_view kCE = U"ค.ศ.";

    std::u32string out(text);
    int count = 0;
    size_t n = out.size();
    bool any_thai_digit = false;
    for (char32_t c : out)
        if (is_thai_digit(c)) any_thai_digit = true;
    size_t i = 0;
    while (i < n) {
        if (!is_digit(out[i])) {
            ++i;
            continue;
        }
        size_t start = i;
        bool thai = is_thai_digit(out[i]);
        bool mixed = false;
        while (i < n && is_digit(out[i])) {
            if (is_thai_digit(out[i]) != thai) mixed = true;
            ++i;
        }
        if (i - start != 4 || mixed) continue;

        // Skip parts of larger numbers: 12,2569 or 1.2569 or 2569.50 or 2569,000.
        auto sep = [](char32_t c) { return c == U',' || c == U'.'; };
        if (start >= 2 && sep(out[start - 1]) && is_digit(out[start - 2])) continue;
        if (i + 1 < n && sep(out[i]) && is_digit(out[i + 1])) continue;

        int value = 0;
        for (size_t k = start; k < i; ++k) value = value * 10 + digit_value(out[k]);

        int result;
        bool be_to_ce;
        if (value >= 2400 && value <= 2700) {
            result = value - 543;
            be_to_ce = true;
        } else if (value >= 1857 && value <= 2157) {
            result = value + 543;
            be_to_ce = false;
        } else {
            continue;
        }

        // ค.ศ. years are always written in Arabic digits. พ.ศ. years follow
        // the text around them: Thai digits if the text uses any.
        bool thai_out = be_to_ce ? false : (thai || any_thai_digit);
        char32_t zero = thai_out ? char32_t(0x0E50) : U'0';
        for (size_t k = i; k-- > start;) {
            out[k] = zero + (result % 10);
            result /= 10;
        }
        ++count;

        // Swap an era label that sits right before the year.
        size_t p = start;
        while (p > 0 && (out[p - 1] == U' ' || out[p - 1] == 0x00A0)) --p;
        if (p >= 4) {
            std::u32string_view label(out.data() + p - 4, 4);
            if (be_to_ce && label == kBE) out.replace(p - 4, 4, kCE);
            else if (!be_to_ce && label == kCE) out.replace(p - 4, 4, kBE);
        }
    }
    if (converted) *converted = count;
    return out;
}

}  // namespace salap
