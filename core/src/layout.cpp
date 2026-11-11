#include "salap/core.h"

#include <unordered_map>

#include "chars.h"

namespace salap {

namespace {

// The 47 character keys of a US keyboard, row by row, and what the same keys
// type on the Thai Kedmanee layout (TIS 820-2538, as shipped by Windows and
// macOS). Index i in an English string is the same physical key as index i
// in the Thai string next to it.
const char32_t* const kEnPlain = U"`1234567890-=" U"qwertyuiop[]\\" U"asdfghjkl;'" U"zxcvbnm,./";
const char32_t* const kThPlain = U"_ๅ/-ภถุึคตจขช" U"ๆไำพะัีรนยบลฃ" U"ฟหกดเ้่าสวง" U"ผปแอิืทมใฝ";
const char32_t* const kEnShift = U"~!@#$%^&*()_+" U"QWERTYUIOP{}|" U"ASDFGHJKL:\"" U"ZXCVBNM<>?";
const char32_t* const kThShift = U"%+๑๒๓๔ู฿๕๖๗๘๙" U"๐\"ฎฑธํ๊ณฯญฐ,ฅ" U"ฤฆฏโฌ็๋ษศซ." U"()ฉฮฺ์?ฒฬฦ";

struct Maps {
    std::unordered_map<char32_t, char32_t> en_to_th;
    std::unordered_map<char32_t, char32_t> th_to_en;
    Maps() {
        add(kEnPlain, kThPlain);
        add(kEnShift, kThShift);
    }
    void add(const char32_t* en, const char32_t* th) {
        std::u32string_view e(en), t(th);
        size_t n = e.size() < t.size() ? e.size() : t.size();
        for (size_t i = 0; i < n; ++i) {
            en_to_th.emplace(e[i], t[i]);
            th_to_en.emplace(t[i], e[i]);
        }
    }
};

const Maps& maps() {
    static const Maps m;
    return m;
}

}  // namespace

LayoutDirection detect_layout_direction(std::u32string_view text) {
    size_t latin = 0, thai = 0;
    for (char32_t c : text) {
        if (detail::is_ascii_letter(c)) ++latin;
        else if (detail::is_thai_letter(c)) ++thai;
    }
    if (latin == 0 && thai == 0) return LayoutDirection::None;
    return thai > latin ? LayoutDirection::ThToEn : LayoutDirection::EnToTh;
}

std::u32string convert_layout(std::u32string_view text, LayoutDirection direction) {
    if (direction == LayoutDirection::None) direction = detect_layout_direction(text);
    std::u32string out(text);
    if (direction == LayoutDirection::None) return out;
    const auto& table =
        direction == LayoutDirection::EnToTh ? maps().en_to_th : maps().th_to_en;
    for (char32_t& c : out) {
        auto it = table.find(c);
        if (it != table.end()) c = it->second;
    }
    return out;
}

Action smart_action(std::u32string_view text) {
    bool digits = false;
    for (char32_t c : text) {
        if (detail::is_ascii_letter(c) || detail::is_thai_letter(c)) return Action::FixLanguage;
        if (detail::is_digit(c)) digits = true;
    }
    return digits ? Action::Numerals : Action::None;
}

Action alternate_action(Action a) {
    switch (a) {
        case Action::FixLanguage: return Action::Numerals;
        case Action::Numerals: return Action::FixLanguage;
        default: return Action::None;
    }
}

}  // namespace salap
