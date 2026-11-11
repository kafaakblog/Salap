// Salap core: platform-independent text conversions.
// All functions are pure, allocate only their result, and never touch the
// network, the clipboard or the keyboard. The macOS and Windows apps call
// these and handle everything that talks to the system.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace salap {

// ---- Encoding helpers -----------------------------------------------------
// Invalid input never throws; bad sequences become U+FFFD.
std::u32string from_utf8(std::string_view s);
std::string to_utf8(std::u32string_view s);
std::u32string from_utf16(std::u16string_view s);
std::u16string to_utf16(std::u32string_view s);

// ---- Wrong-language fix (US QWERTY <-> Thai Kedmanee) ---------------------
enum class LayoutDirection {
    None,    // nothing to go on (no letters)
    EnToTh,  // typed on the English layout, meant Thai:  l;ylfu -> สวัสดี
    ThToEn,  // typed on the Thai layout, meant English:  ้ำสสน -> hello
};

// Guesses which way a piece of text should be converted, from whether it is
// mostly Latin letters or mostly Thai characters.
LayoutDirection detect_layout_direction(std::u32string_view text);

// Re-maps every character as the key that produced it. Characters that do
// not exist on the source layout are left unchanged. With None, the
// direction is detected; if that also gives None the text is returned as is.
std::u32string convert_layout(std::u32string_view text,
                              LayoutDirection direction = LayoutDirection::None);

// ---- Thai <-> Arabic numerals ---------------------------------------------
enum class NumeralDirection {
    Auto,      // Arabic digits present (and not outnumbered) -> Thai, else -> Arabic
    ToThai,    // 123 -> ๑๒๓
    ToArabic,  // ๑๒๓ -> 123
};

// Converts digits only; every other character is kept.
std::u32string convert_numerals(std::u32string_view text,
                                NumeralDirection direction = NumeralDirection::Auto);

// ---- Amount in Thai words --------------------------------------------------
// "1,250" -> "หนึ่งพันสองร้อยห้าสิบบาทถ้วน". Accepts Arabic or Thai digits,
// thousands commas, one decimal point, a leading minus and a leading ฿.
// Rounds to whole satang. Returns false if the text is not a number.
bool baht_text(std::u32string_view number, std::u32string& out);

// ---- Buddhist Era <-> Common Era years ------------------------------------
// Converts every standalone four-digit year: 2400-2700 is read as พ.ศ. and
// becomes ค.ศ.; 1857-2157 is read as ค.ศ. and becomes พ.ศ. A ค.ศ. result is
// always in Arabic digits; a พ.ศ. result is in Thai digits if the text has any
// Thai digit, otherwise Arabic. A "พ.ศ." / "ค.ศ." label right before the year
// is swapped too.
// `converted`, if given, receives how many years were changed.
std::u32string convert_years(std::u32string_view text, int* converted = nullptr);

// ---- One shortcut, two conversions ----------------------------------------
enum class Action {
    None,         // nothing convertible selected
    FixLanguage,  // selection contains letters
    Numerals,     // selection contains digits but no letters
};

// What the shared shortcut should do first for this selection.
Action smart_action(std::u32string_view text);

// What pressing the shortcut again should switch to.
Action alternate_action(Action a);

// ---- Helpers for the apps --------------------------------------------------

// How many times the caret has to move to cross `text`: Thai vowels and tone
// marks that sit above or below a consonant, other combining marks, and the
// second half of a CR LF pair do not count. Used to re-select converted text.
size_t caret_steps(std::u32string_view text);

struct Rect {
    int left, top, right, bottom;  // right and bottom are exclusive
};
struct Point {
    int x, y;
};
struct BadgeInput {
    Point pointer;         // pointer position (the hotspot) on screen
    int cursor_w, cursor_h;  // size of the cursor image as displayed
    int hot_x, hot_y;        // hotspot inside that image
    int badge_w, badge_h;
    int gap;                 // space to leave between cursor and badge
    Rect screen;             // the monitor the pointer is on
};

// Top-left corner for the language badge: below and to the right of the
// cursor image, flipped to the other side at the right and bottom edges, and
// always inside the screen when it fits.
Point place_badge(const BadgeInput& in);

}  // namespace salap
