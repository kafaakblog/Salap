#include "salap/core.h"

#include "chars.h"

namespace salap {

namespace {

using detail::is_ascii_letter;
using detail::is_thai_letter;

enum class ThaiKind { None, Consonant, Leading, AboveBelow, Mark, Following, Other };

ThaiKind thai_kind(char32_t c) {
    if (c >= 0x0E01 && c <= 0x0E2E) return ThaiKind::Consonant;
    if (c >= 0x0E40 && c <= 0x0E44) return ThaiKind::Leading;       // เ แ โ ใ ไ
    if (c == 0x0E31 || (c >= 0x0E34 && c <= 0x0E3A)) return ThaiKind::AboveBelow;
    if (c >= 0x0E47 && c <= 0x0E4E) return ThaiKind::Mark;          // tone marks and friends
    if (c == 0x0E30 || c == 0x0E32 || c == 0x0E33 || c == 0x0E45) return ThaiKind::Following;
    if (c >= 0x0E00 && c <= 0x0E7F) return ThaiKind::Other;
    return ThaiKind::None;
}

// Counts places where Thai characters sit where Thai spelling never puts
// them: a tone mark with nothing to sit on, a vowel with no consonant.
int thai_violations(std::u32string_view s) {
    int bad = 0;
    ThaiKind prev = ThaiKind::None;
    char32_t prev_c = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char32_t c = s[i];
        ThaiKind k = thai_kind(c);
        switch (k) {
            case ThaiKind::AboveBelow:
                if (prev != ThaiKind::Consonant) ++bad;
                break;
            case ThaiKind::Mark:
                if (prev != ThaiKind::Consonant && prev != ThaiKind::AboveBelow) ++bad;
                break;
            case ThaiKind::Following:
                if (c == 0x0E45) {  // ๅ only follows ฤ or ฦ
                    if (prev_c != 0x0E24 && prev_c != 0x0E26) ++bad;
                } else if (prev != ThaiKind::Consonant && prev != ThaiKind::AboveBelow &&
                           prev != ThaiKind::Mark) {
                    ++bad;
                }
                break;
            case ThaiKind::Leading:
                if (i + 1 >= s.size() || thai_kind(s[i + 1]) != ThaiKind::Consonant) ++bad;
                break;
            case ThaiKind::Consonant:
                if (c == 0x0E03 || c == 0x0E05) ++bad;  // ฃ ฅ are no longer used
                break;
            default:
                break;
        }
        prev = k;
        prev_c = c;
    }
    return bad;
}

bool is_vowel(char32_t c) {
    switch (c | 0x20) {
        case U'a': case U'e': case U'i': case U'o': case U'u': case U'y': return true;
        default: return false;
    }
}

// Counts words that do not have the shape of an English word: something
// other than a letter between two letters (l;ylfu), or no vowel at all.
int english_violations(std::u32string_view s) {
    int bad = 0;
    size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && (s[i] == U' ' || s[i] == U'\t' || s[i] == U'\n' || s[i] == U'\r')) ++i;
        size_t start = i;
        while (i < n && !(s[i] == U' ' || s[i] == U'\t' || s[i] == U'\n' || s[i] == U'\r')) ++i;
        if (i == start) continue;
        size_t first = n, last = n;
        int letters = 0;
        bool vowel = false;
        for (size_t k = start; k < i; ++k) {
            if (is_ascii_letter(s[k])) {
                if (first == n) first = k;
                last = k;
                ++letters;
                if (is_vowel(s[k])) vowel = true;
            }
        }
        if (!letters) continue;
        bool odd_inside = false;
        for (size_t k = first; k <= last; ++k) {
            char32_t c = s[k];
            if (!is_ascii_letter(c) && c != U'\'' && c != U'-') odd_inside = true;
        }
        if (odd_inside || (letters >= 3 && !vowel)) ++bad;
    }
    return bad;
}

bool has_letters(std::u32string_view s) {
    for (char32_t c : s)
        if (is_ascii_letter(c) || is_thai_letter(c)) return true;
    return false;
}

}  // namespace

bool layout_fix_likely(std::u32string_view text) {
    LayoutDirection dir = detect_layout_direction(text);
    if (dir == LayoutDirection::None) return false;
    std::u32string fixed = convert_layout(text, dir);
    if (dir == LayoutDirection::EnToTh) return thai_violations(fixed) < english_violations(text);
    return english_violations(fixed) < thai_violations(text);
}

std::vector<std::u32string> conversion_cycle(std::u32string_view text, unsigned features) {
    std::vector<std::u32string> out;
    auto add = [&](const std::u32string& s) {
        if (s == text) return;
        for (const auto& have : out)
            if (have == s) return;
        out.push_back(s);
    };

    const bool can_fix = (features & FixLanguage) && has_letters(text);
    // Text in Latin letters is never a candidate for Thai numerals, years or
    // amounts: 8'gf= is คงเดช typed on the English layout, not a number to
    // convert. Thai-letter text gets the fix only when it looks mistyped.
    // Either way the other conversions would only turn gibberish into more
    // gibberish, so the fix is the whole cycle.
    if (can_fix && (detect_layout_direction(text) == LayoutDirection::EnToTh ||
                    layout_fix_likely(text))) {
        out.push_back(convert_layout(text));
        return out;
    }

    // A year written with a dotted label (พ.ศ. 2569) is a clear request, so it
    // goes first. Without the dots (พศ 2569) it could be something else, such
    // as a licence plate, so there the digit switch goes first and the year
    // conversion is the next press.
    const bool dotted = text.find(U"พ.ศ") != std::u32string_view::npos ||
                        text.find(U"ค.ศ") != std::u32string_view::npos;
    auto add_years = [&]() {
        if (!(features & Years)) return;
        int to_ce = 0, to_be = 0;
        std::u32string y = detail::convert_years_counted(text, &to_ce, &to_be);
        if (to_ce + to_be) add(y);
        // A new พ.ศ. year is often wanted in Thai digits; offer that next.
        if (to_be && !to_ce) add(convert_numerals(y, NumeralDirection::ToThai));
    };
    if (dotted) add_years();
    if (features & Numerals) add(convert_numerals(text));
    if (!dotted) add_years();

    // Nothing else applies: trust the user and offer the language fix anyway.
    if (out.empty() && can_fix) add(convert_layout(text));
    return out;
}

}  // namespace salap
