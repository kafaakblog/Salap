#include "salap/salap_c.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

char* salap_parse_baht_text(const char* words) {
    std::u32string out;
    if (!salap::parse_baht_text(in(words), out)) return nullptr;
    return dup(out);
}

struct salap_cycle {
    std::vector<std::string> items;
};

salap_cycle* salap_conversion_cycle(const char* text, unsigned features) {
    auto* c = new salap_cycle;
    for (const std::u32string& s : salap::conversion_cycle(in(text), features))
        c->items.push_back(salap::to_utf8(s));
    return c;
}

int salap_cycle_count(const salap_cycle* c) { return c ? static_cast<int>(c->items.size()) : 0; }

const char* salap_cycle_item(const salap_cycle* c, int index) {
    if (!c || index < 0 || index >= static_cast<int>(c->items.size())) return nullptr;
    return c->items[index].c_str();
}

void salap_cycle_free(salap_cycle* c) { delete c; }

void salap_free(char* s) { std::free(s); }

}  // extern "C"
