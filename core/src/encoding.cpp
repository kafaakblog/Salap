#include "salap/core.h"

namespace salap {

namespace {
const char32_t kReplacement = 0xFFFD;

bool is_scalar(char32_t c) {
    return c <= 0x10FFFF && !(c >= 0xD800 && c <= 0xDFFF);
}
}  // namespace

std::u32string from_utf8(std::string_view s) {
    std::u32string out;
    out.reserve(s.size());
    size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char b = static_cast<unsigned char>(s[i]);
        if (b < 0x80) {
            out.push_back(b);
            ++i;
            continue;
        }
        int len = 0;
        char32_t cp = 0;
        char32_t min = 0;
        if ((b & 0xE0) == 0xC0) { len = 2; cp = b & 0x1F; min = 0x80; }
        else if ((b & 0xF0) == 0xE0) { len = 3; cp = b & 0x0F; min = 0x800; }
        else if ((b & 0xF8) == 0xF0) { len = 4; cp = b & 0x07; min = 0x10000; }
        if (len == 0) {
            out.push_back(kReplacement);
            ++i;
            continue;
        }
        size_t j = 1;
        for (; j < static_cast<size_t>(len) && i + j < n; ++j) {
            unsigned char c = static_cast<unsigned char>(s[i + j]);
            if ((c & 0xC0) != 0x80) break;
            cp = (cp << 6) | (c & 0x3F);
        }
        if (j != static_cast<size_t>(len) || cp < min || !is_scalar(cp)) {
            out.push_back(kReplacement);
            i += (j > 1 ? j : 1);  // skip what was consumed, at least one byte
            continue;
        }
        out.push_back(cp);
        i += len;
    }
    return out;
}

std::string to_utf8(std::u32string_view s) {
    std::string out;
    out.reserve(s.size() * 3);
    for (char32_t c : s) {
        if (!is_scalar(c)) c = kReplacement;
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

std::u32string from_utf16(std::u16string_view s) {
    std::u32string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char32_t c = s[i];
        if (c >= 0xD800 && c <= 0xDBFF) {
            if (i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF) {
                c = 0x10000 + ((c - 0xD800) << 10) + (s[i + 1] - 0xDC00);
                ++i;
            } else {
                c = kReplacement;
            }
        } else if (c >= 0xDC00 && c <= 0xDFFF) {
            c = kReplacement;
        }
        out.push_back(c);
    }
    return out;
}

std::u16string to_utf16(std::u32string_view s) {
    std::u16string out;
    out.reserve(s.size());
    for (char32_t c : s) {
        if (!is_scalar(c)) c = kReplacement;
        if (c < 0x10000) {
            out.push_back(static_cast<char16_t>(c));
        } else {
            c -= 0x10000;
            out.push_back(static_cast<char16_t>(0xD800 + (c >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00 + (c & 0x3FF)));
        }
    }
    return out;
}

}  // namespace salap
