// Salap core: platform-independent text conversions.
// All functions are pure, allocate only their result, and never touch the
// network, the clipboard or the keyboard. The macOS and Windows apps call
// these and handle everything that talks to the system.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

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
// Converts a four-digit year that has its era label in front of it. The
// label may be written พ.ศ. พ.ศ พศ. or พศ (and the same with ค), with or
// without a space before the year; the label is swapped and keeps its style.
// A year with no label is left alone: 2569 on its own is just a number.
//   พ.ศ. ๒๕๖๙ -> ค.ศ. 2026     พศ2569 -> คศ2026     ค.ศ. 2026 -> พ.ศ. 2569
// A ค.ศ. result is always in Arabic digits; a พ.ศ. result is in Thai digits
// if the text has any Thai digit, otherwise Arabic. In a range such as
// พ.ศ. 2540-2569 both years are converted.
// `converted`, if given, receives how many years were changed.
std::u32string convert_years(std::u32string_view text, int* converted = nullptr);

// ---- Amount in Thai words, back to a number ---------------------------------
// "หนึ่งพันสองร้อยห้าสิบบาทถ้วน" -> "1,250"; with satang -> "1,250.50".
// Only accepts text exactly as baht_text writes it. Returns false otherwise.
bool parse_baht_text(std::u32string_view words, std::u32string& number_out);

// ---- One shortcut for everything ---------------------------------------------
enum Feature : unsigned {
    FixLanguage = 1,
    Numerals = 2,
    Years = 4,
    AllFeatures = 7,
};

// True when the text looks wrong on the layout it was typed on and right on
// the other one, judged by Thai spelling rules and the shape of English
// words. A hint, not a dictionary check.
bool layout_fix_likely(std::u32string_view text);

// The conversions that make sense for a selection, in the order the shared
// shortcut should offer them. The app pastes the first, and on each further
// press the next, then the original text again. Empty if nothing applies.
//   text in Latin letters      -> the language fix only (digits in it are keys)
//   mistyped-looking Thai text -> the language fix only
//   a year with พ.ศ. / ค.ศ.     -> the other era, and the other digit script
//   text with digits           -> the other digit script
// Amounts in Thai words are deliberately not part of this: they are separate
// commands in the apps' menus (baht_text, parse_baht_text).
std::vector<std::u32string> conversion_cycle(std::u32string_view text,
                                             unsigned features = AllFeatures);

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
enum class Corner { BottomRight, BottomLeft, TopRight, TopLeft };

struct BadgeInput {
    Point pointer;  // pointer position (the hotspot) on screen
    Rect cursor;    // the visible part of the cursor image, relative to the pointer
    int badge_w, badge_h;
    Corner corner;  // which corner of the cursor the badge sits at
    int gap;        // space between cursor and badge; may be negative to overlap
    Rect screen;    // the monitor the pointer is on
};

// Top-left corner for the language badge: at the chosen corner of the cursor,
// flipped to the opposite side where it would leave the screen, and always
// inside the screen when it fits.
Point place_badge(const BadgeInput& in);

}  // namespace salap
