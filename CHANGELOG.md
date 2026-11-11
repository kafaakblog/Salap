# Changelog

All versions so far are Windows test builds.

## 0.4.1 (2026-11-12)

- New **About Salap** window in the tray menu: version number, what the app does, recent changes, and developer links.
- Build: moved the GitHub Actions steps to versions that run on Node.js 24.

## 0.4.0 (2026-11-11)

- The shortcut now does three things only: fix the language, switch numerals, and convert a year.
- Amounts in Thai words moved to the tray menu, as three commands: number → Thai words, Thai words → Arabic number, Thai words → Thai number.

## 0.3.2 (2026-11-11)

- Settings says that conversions taken out of the shortcut stay in the tray menu.

## 0.3.1 (2026-11-11)

- A year converts only when พ.ศ. or ค.ศ. is selected with it. Accepts พ.ศ. พ.ศ พศ. พศ (and the same with ค), with or without a space, and ranges.

## 0.3.0 (2026-11-11)

- English-keyboard text with digits in it is fixed correctly (`8'gf=` → `คงเดช`).
- Tap twice works with Ctrl only; the undo shortcut is always a key combination.
- App icon.

## 0.2.0 (2026-11-11)

- One shortcut for the conversions, pressed again for the next one.
- Shortcuts chosen from lists: Ctrl tapped twice, or two modifiers and a key. Undo shortcut.
- Excluded apps get their own window.
- Badge position, distance and colors.

## 0.1.0 (2026-11-11)

- First test build: tray menu, shortcuts, language badge, Settings.
