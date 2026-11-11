// Salap core: platform-independent text conversions.
// All functions are pure, allocate only their result, and never touch the
// network, the clipboard or the keyboard. The macOS and Windows apps call
// these and handle everything that talks to the system.
#pragma once

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
// becomes ค.ศ.; 1857-2157 is read as ค.ศ. and becomes พ.ศ. The digit script
// is kept, and a "พ.ศ." / "ค.ศ." label right before the year is swapped too.
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

}  // namespace salap
