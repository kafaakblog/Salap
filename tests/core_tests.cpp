// Tests for the Salap core. No test framework: each check prints on failure
// and the program exits non-zero if any failed.
#include <cstdio>
#include <cstring>
#include <string>

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
    EQ(convert_years(U"2569", &n), U"2026");
    OK(n == 1);
    EQ(convert_years(U"2026"), U"2569");
    EQ(convert_years(U"๒๕๖๙"), U"๒๐๒๖");
    EQ(convert_years(U"พ.ศ. ๒๕๖๙"), U"ค.ศ. ๒๐๒๖");
    EQ(convert_years(U"ค.ศ.2026"), U"พ.ศ.2569");
    EQ(convert_years(U"วันที่ 11 พฤศจิกายน 2569"), U"วันที่ 11 พฤศจิกายน 2026");
    EQ(convert_years(U"2540-2569", &n), U"1997-2026");
    OK(n == 2);
    // Not years.
    EQ(convert_years(U"12345", &n), U"12345");
    OK(n == 0);
    EQ(convert_years(U"3000"), U"3000");
    EQ(convert_years(U"1,2569"), U"1,2569");
    EQ(convert_years(U"2569.50"), U"2569.50");
    EQ(convert_years(U"2,569,000"), U"2,569,000");
    EQ(convert_years(U"256"), U"256");
    EQ(convert_years(U"25๖๙"), U"25๖๙");
    // A full stop after the year is still a year.
    EQ(convert_years(U"ปี 2569."), U"ปี 2026.");
}

static void test_smart_action() {
    OK(smart_action(U"l;ylfu") == Action::FixLanguage);
    OK(smart_action(U"g-hk.0") == Action::FixLanguage);
    OK(smart_action(U"้ำสสน") == Action::FixLanguage);
    OK(smart_action(U"1,234.50") == Action::Numerals);
    OK(smart_action(U"๑๒๓") == Action::Numerals);
    OK(smart_action(U"... ") == Action::None);
    OK(smart_action(U"") == Action::None);
    OK(alternate_action(Action::FixLanguage) == Action::Numerals);
    OK(alternate_action(Action::Numerals) == Action::FixLanguage);
    OK(alternate_action(Action::None) == Action::None);
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
    s = salap_convert_years("2569", &n);
    OK(s && std::strcmp(s, "2026") == 0 && n == 1);
    salap_free(s);
    OK(salap_smart_action("12") == 2);
    OK(salap_smart_action(nullptr) == 0);
    salap_free(nullptr);
}

int main() {
    test_encoding();
    test_layout();
    test_numerals();
    test_baht_text();
    test_years();
    test_smart_action();
    test_c_api();
    if (g_failures) {
        std::printf("%d of %d checks failed\n", g_failures, g_checks);
        return 1;
    }
    std::printf("all %d checks passed\n", g_checks);
    return 0;
}
