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
    // The visible part of most cursors (arrow, hand) fills roughly the
    // top-left three fifths of the image, so tuck the badge in under it
    // instead of clearing the whole, mostly empty, image box.
    int right_reach = (in.cursor_w - in.hot_x) * 3 / 5;
    int down_reach = (in.cursor_h - in.hot_y) * 4 / 5;
    int left_reach = in.hot_x * 3 / 5;
    int up_reach = in.hot_y * 4 / 5;

    int x = in.pointer.x + right_reach + in.gap;
    int y = in.pointer.y + down_reach + in.gap;
    if (x + in.badge_w > in.screen.right) x = in.pointer.x - left_reach - in.gap - in.badge_w;
    if (y + in.badge_h > in.screen.bottom) y = in.pointer.y - up_reach - in.gap - in.badge_h;

    x = clamp(x, in.screen.left, in.screen.right - in.badge_w);
    y = clamp(y, in.screen.top, in.screen.bottom - in.badge_h);
    return {x, y};
}

}  // namespace salap
