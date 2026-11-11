#include "salap/core.h"

namespace salap {

namespace {

bool is_zero_width(char32_t c) {
    return c == 0x0E31 || (c >= 0x0E34 && c <= 0x0E3A) || (c >= 0x0E47 && c <= 0x0E4E) ||
           (c >= 0x0300 && c <= 0x036F) || (c >= 0xFE00 && c <= 0xFE0F) || c == 0x200D;
}

int clamp(int v, int lo, int hi) {
    if (hi < lo) return lo;
    return v < lo ? lo : (v > hi ? hi : v);
}

}  // namespace

size_t caret_steps(std::u32string_view text) {
    size_t n = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        char32_t c = text[i];
        if (is_zero_width(c)) continue;
        if (c == U'\n' && i > 0 && text[i - 1] == U'\r') continue;
        ++n;
    }
    return n;
}

Point place_badge(const BadgeInput& in) {
    const bool want_right = in.corner == Corner::BottomRight || in.corner == Corner::TopRight;
    const bool want_below = in.corner == Corner::BottomRight || in.corner == Corner::BottomLeft;

    const int right_x = in.pointer.x + in.cursor.right + in.gap;
    const int left_x = in.pointer.x + in.cursor.left - in.gap - in.badge_w;
    const int below_y = in.pointer.y + in.cursor.bottom + in.gap;
    const int above_y = in.pointer.y + in.cursor.top - in.gap - in.badge_h;

    int x = want_right ? right_x : left_x;
    if (want_right && x + in.badge_w > in.screen.right) x = left_x;
    else if (!want_right && x < in.screen.left) x = right_x;

    int y = want_below ? below_y : above_y;
    if (want_below && y + in.badge_h > in.screen.bottom) y = above_y;
    else if (!want_below && y < in.screen.top) y = below_y;

    x = clamp(x, in.screen.left, in.screen.right - in.badge_w);
    y = clamp(y, in.screen.top, in.screen.bottom - in.badge_h);
    return {x, y};
}

}  // namespace salap
