# Salap design notes

## Shape of the project

Salap is three pieces:

1. **Core** (`core/`): C++17, no dependencies. Pure text-in, text-out functions. It never touches the keyboard, the clipboard, the screen or the network, so it can be tested anywhere.
2. **Windows app** (planned, `windows/`): C++ and Win32. Calls the core directly.
3. **macOS app** (planned, `macos/`): Swift and AppKit. Calls the core through the C interface in `salap_c.h`.

Each app is responsible for the parts that talk to the system: reading and replacing the selection, global shortcuts, the pointer badge, the menu, Settings and run-at-login.

Native code on both systems keeps the app small and lets it use the low-level keyboard and pointer APIs that these features need.

## Core functions

| Function | What it does |
|---|---|
| `convert_layout` | Re-maps each character as the key that produced it, between US QWERTY and Thai Kedmanee. |
| `detect_layout_direction` | Mostly Latin letters means English keys to Thai; mostly Thai means Thai keys to English. |
| `convert_numerals` | Thai ⇄ Arabic digits. Everything else is kept. |
| `baht_text` | A number as a Thai-words amount, rounded to whole satang. |
| `convert_years` | Standalone four-digit years between พ.ศ. and ค.ศ. |
| `smart_action` | Decides what the shared shortcut does for a selection. |

### Layout conversion

The table is the 47 character keys, plain and shifted, 94 pairs. Conversion is by key, so number-row keys become Thai letters (`8` is `ค`) and the few ASCII characters the Thai layout can type map back to their keys (`/` on the Thai layout is the `2` key).

A Thai keyboard cannot type Arabic digits from the main rows, so Arabic digits in Thai-typed text are left alone.

Only the Kedmanee layout is supported. Pattachote is not.

### Numerals

In automatic mode, a selection with Arabic digits (that are not outnumbered by Thai digits) goes to Thai; otherwise it goes to Arabic. Mixed input is normalised to one script instead of swapping each digit, because a document with both scripts usually wants one.

### Baht text

Follows the usual reading rules: `เอ็ด` for a final 1 after any higher digit, `ยี่สิบ` for twenty, `ล้าน` repeated for each further six digits, `ถ้วน` when there are no satang. Amounts under one baht are read as satang only.

### Years

A four-digit number from 2400 to 2700 is read as พ.ศ.; from 1857 to 2157 as ค.ศ. The two ranges are the same years and do not overlap, so the direction is never ambiguous. Numbers that are part of a larger figure (`2,569,000`, `2569.50`) are skipped.

### One shortcut for two conversions

`smart_action` looks at the selection:

- any letter, Latin or Thai: fix the language
- digits but no letters: convert numerals
- neither: do nothing

Pressing the shortcut again undoes that and applies the other conversion (`alternate_action`). Once wrong-language detection exists, the first guess will use it, so that correct Thai text containing digits gets the numeral conversion first.

## Planned next

1. **Wrong-language detection** in the core: decide whether a typed word is gibberish on its layout and sensible on the other. Needed for auto-correct.
2. **Windows app**: tray icon, global shortcut, selection replace, badge, Settings.
3. **macOS app**: the same, in Swift.
4. Extras: undo, excluded apps, language per app, learned words.
