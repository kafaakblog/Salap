# Salap design notes

## Shape of the project

Salap is three pieces:

1. **Core** (`core/`): C++17, no dependencies. Pure text-in, text-out functions. It never touches the keyboard, the clipboard, the screen or the network, so it can be tested anywhere.
2. **Windows app** (`windows/Salap.cpp`): C++ and Win32. Calls the core directly.
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

A ค.ศ. result is always written in Arabic digits, whatever the พ.ศ. year used. A พ.ศ. result is written in Thai digits when the selected text contains any Thai digit, and in Arabic digits otherwise, so it matches the document it sits in.

### One shortcut for two conversions

`smart_action` looks at the selection:

- any letter, Latin or Thai: fix the language
- digits but no letters: convert numerals
- neither: do nothing

Pressing the shortcut again undoes that and applies the other conversion (`alternate_action`). Once wrong-language detection exists, the first guess will use it, so that correct Thai text containing digits gets the numeral conversion first.

## Windows app

One source file, Win32 and GDI+, statically linked, no installer yet.

**Reading and replacing the selection.** Windows has no general way to ask another app for its selected text, so Salap borrows the clipboard:

1. Save what is on the clipboard.
2. Wait for the shortcut's modifier keys to be released, then send Ctrl+C.
3. If the clipboard did not change, nothing was selected: stop.
4. Convert, put the result on the clipboard, send Ctrl+V.
5. Send Shift+Left once per caret step to select the new text again.
6. Put the saved clipboard back.

The temporary text is marked so that clipboard history and cloud clipboard ignore it. Selections over 5,000 characters, or ending in a line break (which usually means the app copied a whole line or cell because nothing was selected), are left alone.

**Pressing again.** Salap remembers the last conversion. If the selection it reads is exactly the text it just produced, in the same window, within 30 seconds, the smart shortcut moves to the next step of a cycle: first guess, the other conversion, the original text. This relies on step 5 leaving the result selected.

**Excluded apps.** Ctrl+C interrupts programs in a terminal, so console windows are always skipped and common terminal programs are on the default exclusion list, which is editable in Settings.

**Tray menu commands** act on the app that had the focus before the menu opened. Salap tracks that window and gives the focus back before reading the selection.

**Badge.** A click-through, always-on-top layered window drawn with GDI+. A 16 ms timer reads the pointer position and the cursor's image size and hotspot; the foreground window's keyboard layout is read every sixth tick. The Windows pointer-size setting (`CursorBaseSize`) enlarges the pointer beyond the cursor's own image, so the larger of the two is used. Placement is `place_badge` in the core, which is covered by tests.

**Settings** are in `%APPDATA%\Salap\settings.ini`. Run at login is a value under the current user's `Run` registry key. Shortcuts are released while the Settings window is open so that they can be typed into the shortcut boxes.

Default shortcut: Ctrl+Alt+S for the smart conversion. The other four commands have no shortcut until one is set.

## Planned next

1. Hands-on testing of the Windows build, then fixes.
2. **Wrong-language detection** in the core: decide whether a typed word is gibberish on its layout and sensible on the other. Needed for auto-correct.
3. Windows extras: language per app, learned words, hiding the badge over full-screen apps, an app icon, an installer.
4. **macOS app**: the same features, in Swift.
