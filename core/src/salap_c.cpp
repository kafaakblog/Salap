#include "salap/salap_c.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "salap/core.h"

namespace {

char* dup(const std::u32string& s) {
    std::string u = salap::to_utf8(s);
    char* p = static_cast<char*>(std::malloc(u.size() + 1));
    if (!p) return nullptr;
    std::memcpy(p, u.c_str(), u.size() + 1);
    return p;
}

std::u32string in(const char* s) { return salap::from_utf8(s ? s : ""); }

}  // namespace

extern "C" {

char* salap_convert_layout(const char* text, int direction) {
    salap::LayoutDirection d = direction == 1   ? salap::LayoutDirection::EnToTh
                               : direction == 2 ? salap::LayoutDirection::ThToEn
                                                : salap::LayoutDirection::None;
    return dup(salap::convert_layout(in(text), d));
}

char* salap_convert_numerals(const char* text, int direction) {
    salap::NumeralDirection d = direction == 1   ? salap::NumeralDirection::ToThai
                                : direction == 2 ? salap::NumeralDirection::ToArabic
                                                 : salap::NumeralDirection::Auto;
    return dup(salap::convert_numerals(in(text), d));
}

char* salap_baht_text(const char* number) {
    std::u32string out;
    if (!salap::baht_text(in(number), out)) return nullptr;
    return dup(out);
}

char* salap_convert_years(const char* text, int* converted) {
    return dup(salap::convert_years(in(text), converted));
}

int salap_smart_action(const char* text) {
    switch (salap::smart_action(in(text))) {
        case salap::Action::FixLanguage: return 1;
        case salap::Action::Numerals: return 2;
        default: return 0;
    }
}

void salap_free(char* s) { std::free(s); }

}  // extern "C"
