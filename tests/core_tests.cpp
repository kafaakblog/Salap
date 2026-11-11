// Tests for the Salap core. No test framework: each check prints on failure
// and the program exits non-zero if any failed.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "salap/core.h"
#include "salap/salap_c.h"

using namespace salap;

static int g_checks = 0;
static int g_failures = 0;

static void expect_eq(const std::u32string& got, const std::u32string& want, const char* what,
                      int line) {
    ++g_checks;
    if (got != want) {
        ++g_failures;
        std::printf("FAIL line %d: %s\n  got:  %s\n  want: %s\n", line, what,
                    to_utf8(got).c_str(), to_utf8(want).c_str());
    }
}

static void expect_true(bool ok, const char* what, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("FAIL line %d: %s\n", line, what);
    }
}

#define EQ(got, want) expect_eq((got), (want), #got, __LINE__)
#define OK(cond) expect_true((cond), #cond, __LINE__)

static std::u32string baht(const char32_t* s) {
    std::u32string out;
    return baht_text(s, out) ? out : U"<not a number>";
}

static void test_encoding() {
    const std::u32string s = U"Salap สลับ ๑๒๓ 😀";
    EQ(from_utf8(to_utf8(s)), s);
    EQ(from_utf16(to_utf16(s)), s);
    OK(to_utf16(U"😀").size() == 2);
    OK(to_utf8(U"ก").size() == 3);
    // Broken input becomes U+FFFD instead of crashing or vanishing.
    EQ(from_utf8("a\xFF" "b"), U"a�b");
    EQ(from_utf8("\xE0\xB8"), U"�");
    EQ(from_utf8("\xC0\x80"), U"�");  // overlong
    const char16_t lone[] = {u'a', 0xD800, u'b', 0};
    EQ(from_utf16(lone), U"a�b");
}

static void test_layout() {
    // Typed on the English layout, meant Thai.
    EQ(convert_layout(U"l;ylfu"), U"สวัสดี");
    EQ(convert_layout(U"l;ylfu8iy["), U"สวัสดีครับ");
    EQ(convert_layout(U"g-hk.0"), U"เข้าใจ");  // number-row keys are letters in Thai
    EQ(convert_layout(U"4kKkwmp"), U"ภาษาไทย");
    EQ(convert_layout(U"l;ylfu ;yoouh"), U"สวัสดี วันนี้");  // spaces survive

    // Typed on the Thai layout, meant English.
    EQ(convert_layout(U"้ำสสน"), U"hello");
    EQ(convert_layout(U"ะ้รห"), U"this");
    EQ(convert_layout(U"ธ้ฟร"), U"Thai");
    EQ(convert_layout(U"้ำสสน ไนพสก"), U"hello world");
    EQ(convert_layout(U"ำทฟรส๑ำปฟทยสำใแนท"), U"email@example.com");

    // Forced direction.
    EQ(convert_layout(U"8", LayoutDirection::EnToTh), U"ค");
    EQ(convert_layout(U"ค", LayoutDirection::ThToEn), U"8");
    EQ(convert_layout(U"hello", LayoutDirection::ThToEn), U"hello");  // not Thai-layout output

    // Nothing to go on: unchanged.
    EQ(convert_layout(U"12 34"), U"12 34");
    EQ(convert_layout(U""), U"");

    // Every key round-trips, which also proves the four tables line up.
    for (char32_t c = 0x21; c < 0x7F; ++c) {
        std::u32string en(1, c);
        std::u32string th = convert_layout(en, LayoutDirection::EnToTh);
        OK(th != en);  // no key types the same character on both layouts
        EQ(convert_layout(th, LayoutDirection::ThToEn), en);
    }

    OK(detect_layout_direction(U"l;ylfu") == LayoutDirection::EnToTh);
    OK(detect_layout_direction(U"้ำสสน") == LayoutDirection::ThToEn);
    OK(detect_layout_direction(U"123 ...") == LayoutDirection::None);
}

static void test_numerals() {
    EQ(convert_numerals(U"001,234.50"), U"๐๐๑,๒๓๔.๕๐");
    EQ(convert_numerals(U"๐๐๑,๒๓๔.๕๐"), U"001,234.50");
    EQ(convert_numerals(U"ข้อ 3 วรรค 2"), U"ข้อ ๓ วรรค ๒");
    EQ(convert_numerals(U"ข้อ ๓ วรรค ๒"), U"ข้อ 3 วรรค 2");
    // Mixed input is normalised rather than swapped digit by digit.
    EQ(convert_numerals(U"12 / ๓๔"), U"๑๒ / ๓๔");
    EQ(convert_numerals(U"1 / ๒๓๔"), U"1 / 234");
    EQ(convert_numerals(U"12 / ๓๔", NumeralDirection::ToArabic), U"12 / 34");
    EQ(convert_numerals(U"12 / ๓๔", NumeralDirection::ToThai), U"๑๒ / ๓๔");
    EQ(convert_numerals(U"no digits"), U"no digits");
}

static void test_baht_text() {
    EQ(baht(U"0"), U"ศูนย์บาทถ้วน");
    EQ(baht(U"1"), U"หนึ่งบาทถ้วน");
    EQ(baht(U"10"), U"สิบบาทถ้วน");
    EQ(baht(U"11"), U"สิบเอ็ดบาทถ้วน");
    EQ(baht(U"20"), U"ยี่สิบบาทถ้วน");
    EQ(baht(U"21"), U"ยี่สิบเอ็ดบาทถ้วน");
    EQ(baht(U"101"), U"หนึ่งร้อยเอ็ดบาทถ้วน");
    EQ(baht(U"110"), U"หนึ่งร้อยสิบบาทถ้วน");
    EQ(baht(U"1,250"), U"หนึ่งพันสองร้อยห้าสิบบาทถ้วน");
    EQ(baht(U"1000000"), U"หนึ่งล้านบาทถ้วน");
    EQ(baht(U"2,000,000"), U"สองล้านบาทถ้วน");
    EQ(baht(U"1000001"), U"หนึ่งล้านเอ็ดบาทถ้วน");
    EQ(baht(U"11,000,000"), U"สิบเอ็ดล้านบาทถ้วน");
    EQ(baht(U"1,234,567.89"),
       U"หนึ่งล้านสองแสนสามหมื่นสี่พันห้าร้อยหกสิบเจ็ดบาทแปดสิบเก้าสตางค์");
    EQ(baht(U"1000000000000"), U"หนึ่งล้านล้านบาทถ้วน");
    EQ(baht(U"0.50"), U"ห้าสิบสตางค์");
    EQ(baht(U".5"), U"ห้าสิบสตางค์");
    EQ(baht(U"0.01"), U"หนึ่งสตางค์");
    EQ(baht(U"1.01"), U"หนึ่งบาทหนึ่งสตางค์");
    EQ(baht(U"1.21"), U"หนึ่งบาทยี่สิบเอ็ดสตางค์");
    EQ(baht(U"100.5"), U"หนึ่งร้อยบาทห้าสิบสตางค์");
    // Rounding to whole satang.
    EQ(baht(U"1.005"), U"หนึ่งบาทหนึ่งสตางค์");
    EQ(baht(U"1.004"), U"หนึ่งบาทถ้วน");
    EQ(baht(U"0.999"), U"หนึ่งบาทถ้วน");
    EQ(baht(U"999.995"), U"หนึ่งพันบาทถ้วน");
    // Other accepted spellings.
    EQ(baht(U"๑๒๐"), U"หนึ่งร้อยยี่สิบบาทถ้วน");
    EQ(baht(U"  ฿ 1,250.00 "), U"หนึ่งพันสองร้อยห้าสิบบาทถ้วน");
    EQ(baht(U"-5"), U"ลบห้าบาทถ้วน");
    EQ(baht(U"-0"), U"ศูนย์บาทถ้วน");
    EQ(baht(U"007"), U"เจ็ดบาทถ้วน");
    // Not numbers.
    EQ(baht(U""), U"<not a number>");
    EQ(baht(U"abc"), U"<not a number>");
    EQ(baht(U"1.2.3"), U"<not a number>");
    EQ(baht(U"12 บาท"), U"<not a number>");
    EQ(baht(U","), U"<not a number>");
    EQ(baht(U"1,"), U"<not a number>");
    EQ(baht(U"."), U"<not a number>");
}

static void test_years() {
    int n = -1;
    // Every way of writing the label, with Arabic and Thai digits.
    EQ(convert_years(U"พ.ศ. 2569", &n), U"ค.ศ. 2026");
    OK(n == 1);
    EQ(convert_years(U"พ.ศ.2569"), U"ค.ศ.2026");
    EQ(convert_years(U"พศ2569"), U"คศ2026");
    EQ(convert_years(U"พศ 2569"), U"คศ 2026");
    EQ(convert_years(U"พ.ศ. ๒๕๖๙"), U"ค.ศ. 2026");
    EQ(convert_years(U"พ.ศ.๒๕๖๙"), U"ค.ศ.2026");
    EQ(convert_years(U"พศ๒๕๖๙"), U"คศ2026");
    EQ(convert_years(U"พศ ๒๕๖๙"), U"คศ 2026");
    EQ(convert_years(U"พ.ศ 2569"), U"ค.ศ 2026");
    EQ(convert_years(U"พศ. 2569"), U"คศ. 2026");
    // And back.
    EQ(convert_years(U"ค.ศ. 2026"), U"พ.ศ. 2569");
    EQ(convert_years(U"ค.ศ.2026"), U"พ.ศ.2569");
    EQ(convert_years(U"คศ2026"), U"พศ2569");
    EQ(convert_years(U"คศ 2026"), U"พศ 2569");
    EQ(convert_years(U"ค.ศ. ๒๐๒๖"), U"พ.ศ. ๒๕๖๙");
    // พ.ศ. follows the surrounding text: Thai digits if it uses any.
    EQ(convert_years(U"วันที่ ๑๑ พฤศจิกายน ค.ศ. 2026"), U"วันที่ ๑๑ พฤศจิกายน พ.ศ. ๒๕๖๙");
    EQ(convert_years(U"วันที่ 11 พฤศจิกายน ค.ศ. 2026"), U"วันที่ 11 พฤศจิกายน พ.ศ. 2569");
    // ค.ศ. is always in Arabic digits.
    EQ(convert_years(U"วันที่ ๑๑ พฤศจิกายน พ.ศ. ๒๕๖๙"), U"วันที่ ๑๑ พฤศจิกายน ค.ศ. 2026");
    // Ranges and several years.
    EQ(convert_years(U"พ.ศ. 2540-2569", &n), U"ค.ศ. 1997-2026");
    OK(n == 2);
    EQ(convert_years(U"พ.ศ. 2540 – 2569"), U"ค.ศ. 1997 – 2026");
    EQ(convert_years(U"พ.ศ. 2475 และ พ.ศ. 2569", &n), U"ค.ศ. 1932 และ ค.ศ. 2026");
    OK(n == 2);
    EQ(convert_years(U"ปี พ.ศ. 2569."), U"ปี ค.ศ. 2026.");
    // No label, no conversion: the number alone could be anything.
    EQ(convert_years(U"2569", &n), U"2569");
    OK(n == 0);
    EQ(convert_years(U"๒๕๖๙"), U"๒๕๖๙");
    EQ(convert_years(U"ปี 2569"), U"ปี 2569");
    EQ(convert_years(U"วันที่ 11 พฤศจิกายน 2569"), U"วันที่ 11 พฤศจิกายน 2569");
    // A label, but not a year after it.
    EQ(convert_years(U"พ.ศ. 12345"), U"พ.ศ. 12345");
    EQ(convert_years(U"พ.ศ. 256"), U"พ.ศ. 256");
    EQ(convert_years(U"พ.ศ. 2569.50"), U"พ.ศ. 2569.50");
    EQ(convert_years(U"พ.ศ. 2,569"), U"พ.ศ. 2,569");
    EQ(convert_years(U"พ.ศ. 25๖๙"), U"พ.ศ. 25๖๙");
    EQ(convert_years(U"พ.ศ. 0100"), U"พ.ศ. 0100");
    // Not a label.
    EQ(convert_years(U"เทพศ 2569"), U"เทพศ 2569");
    EQ(convert_years(U"ศ. 2569"), U"ศ. 2569");
    EQ(convert_years(U"พ.ศ."), U"พ.ศ.");
}

static std::u32string unbaht(const char32_t* s) {
    std::u32string out;
    return parse_baht_text(s, out) ? out : U"<not an amount>";
}

static void test_parse_baht_text() {
    EQ(unbaht(U"หนึ่งพันสองร้อยห้าสิบบาทถ้วน"), U"1,250");
    EQ(unbaht(U"ศูนย์บาทถ้วน"), U"0");
    EQ(unbaht(U"สิบเอ็ดบาทถ้วน"), U"11");
    EQ(unbaht(U"ยี่สิบเอ็ดบาทห้าสิบสตางค์"), U"21.50");
    EQ(unbaht(U"ห้าสิบสตางค์"), U"0.50");
    EQ(unbaht(U"หนึ่งสตางค์"), U"0.01");
    EQ(unbaht(U"หนึ่งล้านเอ็ดบาทถ้วน"), U"1,000,001");
    EQ(unbaht(U"หนึ่งล้านล้านบาทถ้วน"), U"1,000,000,000,000");
    EQ(unbaht(U"ลบห้าบาทถ้วน"), U"-5");
    EQ(unbaht(U"  หนึ่งร้อยบาทถ้วน "), U"100");
    // Every amount that baht_text writes reads back as the same number.
    for (const char32_t* n : {U"0", U"7", U"10", U"19", U"20", U"99", U"100", U"101", U"111",
                              U"1,000", U"10,001", U"110,011", U"999,999", U"1,000,000",
                              U"12,345,678.90", U"999,999,999,999,999,999", U"0.25", U"3.07"}) {
        EQ(unbaht(baht(n).c_str()), n);
    }
    // Not amounts, or not written the standard way.
    EQ(unbaht(U"หนึ่งร้อย"), U"<not an amount>");
    EQ(unbaht(U"สิบหนึ่งบาทถ้วน"), U"<not an amount>");
    EQ(unbaht(U"หนึ่งบาท"), U"<not an amount>");
    EQ(unbaht(U"บาทถ้วน"), U"<not an amount>");
    EQ(unbaht(U"สวัสดี"), U"<not an amount>");
    EQ(unbaht(U""), U"<not an amount>");
}

static void test_cycle() {
    typedef std::vector<std::u32string> V;
    // Wrong-language text gets the fix and nothing else.
    OK(conversion_cycle(U"l;ylfu") == V{U"สวัสดี"});
    OK(conversion_cycle(U"g-hk.0") == V{U"เข้าใจ"});
    OK(conversion_cycle(U"้ำสสน") == V{U"hello"});
    OK(conversion_cycle(U"ะ้รห รห") == V{U"this is"});
    // Latin letters with digits: the digits are Thai-letter keys, not numbers.
    OK(conversion_cycle(U"8'gf=") == V{U"คงเดช"});
    OK(conversion_cycle(U"4kKk8o") == V{U"ภาษาคน"});
    OK(conversion_cycle(U"8o") == V{U"คน"});
    // Mostly Thai with a Latin word in it is still Thai text.
    OK(conversion_cycle(U"ประชุมทาง Zoom 3 ครั้ง") == V{U"ประชุมทาง Zoom ๓ ครั้ง"});
    // With the language fix switched off, Latin text falls back to the rest.
    OK(conversion_cycle(U"Room 12", Numerals) == V{U"Room ๑๒"});
    // Numbers: a plain switch between the two digit scripts.
    OK(conversion_cycle(U"1,250") == V{U"๑,๒๕๐"});
    OK(conversion_cycle(U"๑,๒๕๐") == V{U"1,250"});
    OK(conversion_cycle(U"2569") == V{U"๒๕๖๙"});
    // A year with its label: the year conversion comes first.
    OK(conversion_cycle(U"พ.ศ. 2569") == (V{U"ค.ศ. 2026", U"พ.ศ. ๒๕๖๙"}));
    OK(conversion_cycle(U"พ.ศ. ๒๕๖๙") == (V{U"ค.ศ. 2026", U"พ.ศ. 2569"}));
    // Without dots it might not be a year (a licence plate, say): digits first.
    OK(conversion_cycle(U"พศ2569") == (V{U"พศ๒๕๖๙", U"คศ2026"}));
    OK(conversion_cycle(U"พศ ๒๕๖๙") == (V{U"พศ 2569", U"คศ 2026"}));
    OK(conversion_cycle(U"ค.ศ. 2026") == (V{U"พ.ศ. 2569", U"พ.ศ. ๒๕๖๙", U"ค.ศ. ๒๐๒๖"}));
    // Correct Thai with digits is not turned into gibberish.
    OK(conversion_cycle(U"ข้อ 3 วรรค 2") == V{U"ข้อ ๓ วรรค ๒"});
    OK(conversion_cycle(U"วันที่ ๑๑ พ.ศ. ๒๕๖๙") == (V{U"วันที่ ๑๑ ค.ศ. 2026", U"วันที่ 11 พ.ศ. 2569"}));
    // Amounts in words are not the shortcut's business; with nothing else to
    // offer, plain Thai text gets the language fix like any other.
    OK(conversion_cycle(U"หนึ่งพันสองร้อยห้าสิบบาทถ้วน").size() == 1);
    // Correct text with nothing else to do: the fix is still offered.
    OK(conversion_cycle(U"hello") == V{U"้ำสสน"});
    OK(conversion_cycle(U"ไนพสก") == V{U"world"});
    // Nothing to do.
    OK(conversion_cycle(U"...").empty());
    OK(conversion_cycle(U"").empty());
    // Switched-off features are skipped.
    OK(conversion_cycle(U"1,250", Numerals) == V{U"๑,๒๕๐"});
    OK(conversion_cycle(U"1,250", FixLanguage | Years).empty());
    OK(conversion_cycle(U"l;ylfu", Numerals | Years).empty());
    OK(conversion_cycle(U"พ.ศ. 2569", FixLanguage | Years) == V{U"ค.ศ. 2026"});
    OK(conversion_cycle(U"พ.ศ. 2569", Numerals) == V{U"พ.ศ. ๒๕๖๙"});

    OK(layout_fix_likely(U"l;ylfu"));
    OK(layout_fix_likely(U"้ำสสน"));
    OK(!layout_fix_likely(U"hello world"));
    OK(!layout_fix_likely(U"สวัสดีครับ วันนี้อากาศดี"));
    OK(!layout_fix_likely(U"น้ำ เพราะ สิทธิ์ ไม่ ก็ ฤๅ"));
    OK(!layout_fix_likely(U"12345"));
}

static void test_caret_steps() {
    OK(caret_steps(U"") == 0);
    OK(caret_steps(U"hello") == 5);
    OK(caret_steps(U"สวัสดี") == 4);      // ส ว(ั) ส ด(ี)
    OK(caret_steps(U"เข้าใจ") == 5);      // เ ข(้) า ใ จ
    OK(caret_steps(U"น้ำ") == 2);         // น(้) ำ
    OK(caret_steps(U"a\r\nb") == 3);
    OK(caret_steps(U"a\nb") == 3);
    OK(caret_steps(U"๑๒๓") == 3);
}

static void test_badge() {
    const Rect screen{0, 0, 1920, 1080};
    const Rect second{-1280, 200, 0, 1224};  // a monitor to the left, offset down
    // Visible cursor shapes relative to the pointer: arrow, hand, I-beam, at three sizes.
    const Rect shapes[] = {{0, 0, 12, 19},   {-6, 0, 16, 22},   {-4, -9, 5, 10},
                           {0, 0, 36, 57},   {-18, 0, 48, 66},  {-12, -27, 15, 30},
                           {0, 0, 96, 152},  {-48, 0, 128, 176}, {-32, -72, 40, 80}};
    const Corner corners[] = {Corner::BottomRight, Corner::BottomLeft, Corner::TopRight,
                              Corner::TopLeft};
    int cases = 0;
    for (const Rect& sc : {screen, second}) {
        for (const Rect& shape : shapes) {
            for (Corner corner : corners) {
                for (int gap : {0, 2, 12}) {
                    for (int fx = 0; fx <= 10; ++fx) {
                        for (int fy = 0; fy <= 10; ++fy) {
                            BadgeInput in{};
                            in.pointer = {sc.left + (sc.right - sc.left - 1) * fx / 10,
                                          sc.top + (sc.bottom - sc.top - 1) * fy / 10};
                            in.cursor = shape;
                            in.badge_w = 30;
                            in.badge_h = 20;
                            in.corner = corner;
                            in.gap = gap;
                            in.screen = sc;
                            Point p = place_badge(in);
                            ++cases;
                            bool inside = p.x >= sc.left && p.y >= sc.top &&
                                          p.x + in.badge_w <= sc.right &&
                                          p.y + in.badge_h <= sc.bottom;
                            bool covers = in.pointer.x >= p.x && in.pointer.x < p.x + in.badge_w &&
                                          in.pointer.y >= p.y && in.pointer.y < p.y + in.badge_h;
                            if (!inside || covers) {
                                std::printf("badge case: at %d,%d -> %d,%d\n", in.pointer.x,
                                            in.pointer.y, p.x, p.y);
                            }
                            OK(inside);
                            OK(!covers);
                        }
                    }
                }
            }
        }
    }
    OK(cases == 2 * 9 * 4 * 3 * 121);

    // Away from the edges the badge touches the chosen corner of the cursor.
    BadgeInput in{{500, 500}, {0, 0, 12, 19}, 30, 20, Corner::BottomRight, 0, screen};
    Point p = place_badge(in);
    OK(p.x == 512 && p.y == 519);
    in.corner = Corner::TopLeft;
    p = place_badge(in);
    OK(p.x == 470 && p.y == 480);
    in.corner = Corner::BottomLeft;
    p = place_badge(in);
    OK(p.x == 470 && p.y == 519);
    // The gap moves it away; a negative gap tucks it under the cursor.
    in.corner = Corner::BottomRight;
    in.gap = 5;
    p = place_badge(in);
    OK(p.x == 517 && p.y == 524);
    in.gap = -4;
    p = place_badge(in);
    OK(p.x == 508 && p.y == 515);
    // A larger pointer pushes it further out.
    in.gap = 0;
    in.cursor = {0, 0, 48, 76};
    p = place_badge(in);
    OK(p.x == 548 && p.y == 576);
    // At the bottom-right corner of the screen it flips to the top-left.
    in.cursor = {0, 0, 12, 19};
    in.pointer = {1915, 1075};
    p = place_badge(in);
    OK(p.x + 30 <= 1915 && p.y + 20 <= 1075);
}

static void test_c_api() {
    char* s = salap_convert_layout("l;ylfu", 0);
    OK(s && std::strcmp(s, to_utf8(U"สวัสดี").c_str()) == 0);
    salap_free(s);
    s = salap_convert_numerals("123", 1);
    OK(s && std::strcmp(s, to_utf8(U"๑๒๓").c_str()) == 0);
    salap_free(s);
    s = salap_baht_text("21");
    OK(s && std::strcmp(s, to_utf8(U"ยี่สิบเอ็ดบาทถ้วน").c_str()) == 0);
    salap_free(s);
    OK(salap_baht_text("x") == nullptr);
    int n = 0;
    s = salap_convert_years(to_utf8(U"พศ2569").c_str(), &n);
    OK(s && std::strcmp(s, to_utf8(U"คศ2026").c_str()) == 0 && n == 1);
    salap_free(s);
    s = salap_parse_baht_text(to_utf8(U"ยี่สิบเอ็ดบาทถ้วน").c_str());
    OK(s && std::strcmp(s, "21") == 0);
    salap_free(s);
    OK(salap_parse_baht_text("21") == nullptr);
    salap_cycle* c = salap_conversion_cycle("1,250", 7);
    OK(salap_cycle_count(c) == 1);
    OK(salap_cycle_item(c, 0) && std::strcmp(salap_cycle_item(c, 0), to_utf8(U"๑,๒๕๐").c_str()) == 0);
    OK(salap_cycle_item(c, 1) == nullptr);
    salap_cycle_free(c);
    c = salap_conversion_cycle(nullptr, 7);
    OK(salap_cycle_count(c) == 0);
    salap_cycle_free(c);
    salap_free(nullptr);
}

int main() {
    test_encoding();
    test_layout();
    test_numerals();
    test_baht_text();
    test_years();
    test_parse_baht_text();
    test_cycle();
    test_caret_steps();
    test_badge();
    test_c_api();
    if (g_failures) {
        std::printf("%d of %d checks failed\n", g_failures, g_checks);
        return 1;
    }
    std::printf("all %d checks passed\n", g_checks);
    return 0;
}
