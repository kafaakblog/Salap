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
| `parse_baht_text` | A Thai-words amount back to a number. |
| `conversion_cycle` | The conversions the shared shortcut offers for a selection, in order. |
| `caret_steps`, `place_badge` | Helpers for re-selecting text and placing the badge. |

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

### One shortcut for everything

`conversion_cycle` returns the conversions that fit a selection, in the order to offer them. The app pastes the first; each further press pastes the next, and after the last one the original text comes back.

| Selection | Offered, in order |
|---|---|
| Mostly Latin letters | the language fix only |
| Thai letters that look mistyped | the language fix only |
| An amount in Thai words | the number |
| Contains digits | other digit script, year (if it has one), amount in words (if it is a number) |
| Anything else with letters | the language fix |

Features the user has switched off are left out.

Text in Latin letters is always treated as Thai typed on the English layout, because its digits are Thai-letter keys: `8'gf=` is `คงเดช`, not a number. The cost is that real English with a number in it ("Room 12") cannot have its digits switched with the shortcut; the tray menu's numerals command still does that.

"Look mistyped" is `layout_fix_likely`: it counts Thai spelling impossibilities (a tone mark with no consonant, a leading vowel with nothing after it) and un-English word shapes (punctuation between letters, no vowel), in the text and in its converted form, and says yes when converting makes the count go down. It is a hint, not a dictionary: Thai-layout text that happens to be spellable, like `ไนพสก` for `world`, is not recognised, though it still gets the fix when nothing else applies. Auto-correct will need something stronger.

### Thai words back to a number

`parse_baht_text` reads number words, then checks its answer by converting the number back with `baht_text` and comparing. Only text written exactly the standard way is accepted, so a loosely similar phrase is never turned into a wrong number.

### Badge placement

`place_badge` takes the visible part of the cursor image as a rectangle around the pointer, a corner, and a gap. The badge touches that corner, moved out by the gap (or in, if negative), and flips to the opposite side at a screen edge.

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

**Pressing again.** Salap remembers the last conversion. If the selection it reads is exactly the text it just produced, in the same window, within 30 seconds, the shortcut moves to the next step of the cycle. This relies on step 5 leaving the result selected.

**Undo.** If the converted text is still selected, the undo shortcut pastes the original back. If it is not, Salap sends Ctrl+Z to the app instead, within 30 seconds of the conversion.

**Shortcuts.** The convert shortcut is either Ctrl tapped twice, or two modifiers and a key held together; the undo shortcut is always a combination. Alt and Shift are not offered for tapping, because Windows and apps already react to those keys on their own. Combinations use `RegisterHotKey`. Double taps need a low-level keyboard hook, which runs on its own thread so that it always answers quickly; a mouse hook on the same thread cancels a tap when a button is clicked, so Ctrl+click twice does not count. The hooks are installed only while a double-tap shortcut is chosen, look only at whether a key is Ctrl, and store nothing. Combinations always have two modifiers so that Salap cannot take over Ctrl+S or Alt+F.

**Excluded apps.** Ctrl+C interrupts programs in a terminal, so console windows are always skipped and common terminal programs are built into the exclusion list and cannot be removed. Users add their own from a list of open apps or by browsing for a program.

**Tray menu commands** act on the app that had the focus before the menu opened. Salap tracks that window and gives the focus back before reading the selection.

**Badge.** A click-through, always-on-top layered window drawn with GDI+. A 16 ms timer reads the pointer position; the foreground window's keyboard layout is read every sixth tick. For each new cursor, Salap reads the cursor's image and finds the box around the pixels that are actually drawn, so the badge sits against the arrow or hand itself and not against the mostly empty square around it. The Windows pointer-size setting (`CursorBaseSize`) enlarges the pointer beyond the cursor's own image, so the shape is scaled to the larger of the two. The corner, the distance and the colors for Thai and English are user settings; changes show on the live badge while Settings is open.

**Settings** are in `%APPDATA%\Salap\settings.ini`. Run at login is a value under the current user's `Run` registry key. Shortcuts are released while the Settings window is open so that they can be typed into the shortcut boxes.

Defaults: Ctrl tapped twice converts, Ctrl+Alt+Z undoes.

## Planned next

1. Hands-on testing of the Windows build, then fixes.
2. **Wrong-language detection** in the core: decide whether a typed word is gibberish on its layout and sensible on the other. Needed for auto-correct.
3. Windows extras: language per app, learned words, hiding the badge over full-screen apps, an installer.
4. **macOS app**: the same features, in Swift.
