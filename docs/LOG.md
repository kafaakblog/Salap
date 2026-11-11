# Salap – Project Log

Running log of decisions made and work done. Newest entries are at the bottom of each table. Dates are Bangkok time.

## What Salap is

A small macOS and Windows app for Thai users that fixes text typed in the wrong keyboard language, toggles Thai ⇄ Arabic numerals, and shows the current input language in a badge at the mouse pointer.

- Repo: https://github.com/kafaakblog/Salap
- Local folder: `AI Personal Projects\Salap` on the Windows PC

## Decisions

| Date | Decision | Why |
|---|---|---|
| 2026-11-11 | Build a new app instead of continuing the CursorLingo fork (kafaakblog/cursor-lingo). | @kafaak wants an app of their own. |
| 2026-11-11 | App name: **Salap** (สลับ, "switch"). | Chosen by @kafaak from the shortlist. |
| 2026-11-11 | Write it **from scratch**; no code copied from CursorLingo. | @kafaak wants to test the limits of Claude Fable 5.2. |
| 2026-11-11 | Platforms: macOS and Windows. | Requirement. |
| 2026-11-11 | Interface languages: Thai and English only. | The app is meant for Thai users. |
| 2026-11-11 | New GitHub repo, separate from the fork: kafaakblog/Salap, public, MIT license. | Requirement; repo created by @kafaak. |
| 2026-11-11 | All suggested extra features are approved (see scope). | "All features are good." |
| 2026-11-11 | Architecture: one shared C++17 engine with no dependencies, a Win32 C++ app for Windows and a Swift/AppKit app for macOS. | Small native apps with direct access to keyboard and pointer APIs; the engine is testable on any machine. Chosen by Claude. |
| 2026-11-11 | Keyboard layouts: US QWERTY ⇄ Thai Kedmanee only. | Kedmanee is the standard Thai layout. Pattachote can be added later. |
| 2026-11-11 | Automatic numeral toggle normalises mixed text to one script (Arabic present → all Thai, otherwise all Arabic). | A document usually wants one script throughout. Chosen by Claude. |
| 2026-11-11 | ~~Year conversion reads 2400–2700 as พ.ศ. and 1857–2157 as ค.ศ.~~ Superseded: see "Year conversion needs the era label". | The ranges cover the same years and cannot be confused. Chosen by Claude. |
| 2026-11-11 | A ค.ศ. year is always written in Arabic digits: พ.ศ. ๒๕๖๙ → ค.ศ. 2026. | Requested by @kafaak. |
| 2026-11-11 | A พ.ศ. year is written in Thai digits when the selected text has any Thai digit, otherwise in Arabic digits. | So the year matches the document around it. Chosen by Claude; to be confirmed by @kafaak. |
| 2026-11-11 | Pushing straight to `main` while the project is being set up. | Solo project, empty repo. Chosen by Claude; @kafaak can ask for branches and pull requests. |
| 2026-11-11 | Windows app reads and replaces the selection through the clipboard (Ctrl+C, Ctrl+V), then restores the clipboard. | Windows has no general API for another app's selection. |
| 2026-11-11 | Default shortcut: **Ctrl+Alt+S** for the smart conversion; the other commands have none by default. | One shortcut to remember; S for Salap. Chosen by Claude. |
| 2026-11-11 | The smart shortcut cycles: first guess → the other conversion → original text. | Gives "press again to switch" and an undo with one key. |
| 2026-11-11 | Terminals are excluded by default; the list is editable. | Ctrl+C interrupts programs in a terminal. |
| 2026-11-11 | The badge follows the pointer with a 16 ms timer, not a mouse hook. | A hook would make the mouse stutter while Salap is busy converting. |
| 2026-11-11 | **One shortcut does every conversion.** Each press pastes the next conversion that fits the selection, then the original. Settings has one shortcut plus a checklist of which conversions it uses. Replaces the five separate shortcuts. | Requested by @kafaak. |
| 2026-11-11 | A shortcut can be a modifier tapped twice (Ctrl, Alt, Shift) or two modifiers plus a key, chosen from drop-down lists instead of a "press the keys" box. | Requested by @kafaak; easier to understand. |
| 2026-11-11 | Defaults: **Ctrl twice** converts, **Ctrl+Alt+Z** undoes. Supersedes Ctrl+Alt+S. | @kafaak asked for Ctrl twice and an undo shortcut. |
| 2026-11-11 | Key combinations always have two modifiers. | A single one would let Salap take over Ctrl+S, Ctrl+C and the like. Chosen by Claude. |
| 2026-11-11 | Wrong-language text gets only the language fix in the cycle; correct text is never turned into gibberish unless nothing else applies. | Otherwise every press on Thai text with digits would pass through nonsense. Chosen by Claude. |
| 2026-11-11 | "Amount in Thai words" is a toggle: the words convert back to a number (`1,250`, or `1,250.50` with satang). | Requested by @kafaak. Number format chosen by Claude. |
| 2026-11-11 | Excluded apps get their own window. Built-in terminals are listed but locked; users add apps from a list of open apps or by browsing. | Requested by @kafaak. |
| 2026-11-11 | Badge: user-chosen corner and distance, placed against the visible cursor shape; 7 preset colors each for Thai and English. | Requested by @kafaak (badge was too far from the pointer). |
| 2026-11-11 | Salap makes no sound when nothing is selected. | A double tap can happen by accident. Chosen by Claude. |
| 2026-11-11 | Text in Latin letters always gets the language fix and never the number conversions, even when it contains digits. | @kafaak found `8'gf=` (คงเดช) became `๘'gf=`. Digits in English-layout text are Thai-letter keys. |
| 2026-11-11 | Tap-twice is offered for **Ctrl only**; Alt and Shift are removed. The undo shortcut is always a key combination. | Requested by @kafaak: Windows already uses Alt and Shift taps. |
| 2026-11-11 | App icon supplied by @kafaak (ส and A cards swapping, on blue). Used for the program file, the tray and Settings. | Provided by @kafaak. |
| 2026-11-11 | **Year conversion needs the era label.** A year converts only when พ.ศ. or ค.ศ. is selected with it; a bare number is never a year. Supersedes the 2400–2700 / 1857–2157 range rule. | Requested by @kafaak after year conversion did not behave as expected. |
| 2026-11-11 | Accepted label spellings: พ.ศ. พ.ศ พศ. พศ (and with ค), with or without a space, Arabic or Thai digits. The label keeps its style when swapped. | Scenarios listed by @kafaak. |
| 2026-11-11 | Keep the year feature. Dotted labels convert on the first press; undotted ones (พศ 2569) switch digits first and convert the year on the next press. | @kafaak offered to drop the feature over cases like licence plates. The dotted form is unambiguous, the undotted form is only one press further away, and the feature can be unticked in Settings. Chosen by Claude. |
| 2026-11-11 | Settings and the tray menu say that พ.ศ. or ค.ศ. must be selected with the year; the tray command explains this when it finds no labelled year. | Requested by @kafaak. |
| 2026-11-11 | Year conversion can be used both ways, by the user's choice: ticked in Settings it is part of the shortcut; unticked it is in the tray menu only. The same holds for every conversion, and Settings says so. | @kafaak: "keep both", after weighing shortcut against menu-only. |

## Approved scope

Core
1. Fix selected wrong-language text (English ⇄ Thai Kedmanee) from the menu or a shortcut.
2. Auto-correct while typing (off by default, undoable).
3. Toggle selected numerals between Thai and Arabic.
4. Language badge at the mouse pointer, positioned from the real pointer size and hotspot.
5. Settings: run at login, turn each feature on or off, custom shortcuts.
6. One shared shortcut for both conversions by default: a selection with letters gets the language fix, a selection with only digits and punctuation gets the numeral toggle, and pressing again switches to the other conversion. Separate shortcuts can be assigned.

Extras
7. Number to Thai words (baht text).
8. พ.ศ. ⇄ ค.ศ. year conversion.
9. Undo last conversion.
10. Excluded apps.
11. Remember language per app.
12. Learned words for auto-correct.
13. Thai and English interface, following the system language.
14. Fully offline: no network, no telemetry.

## Progress

| # | Feature | Engine | Windows | macOS |
|---|---|---|---|---|
| 1 | Fix selected wrong-language text | Done | Built, untested | Not started |
| 2 | Auto-correct while typing | Not started | Not started | Not started |
| 3 | Thai ⇄ Arabic numerals | Done | Built, untested | Not started |
| 4 | Language badge: position, distance, colors | Placement done | Rebuilt in 0.2.0, untested | Not started |
| 5 | Settings: run at login, feature switches, shortcuts | n/a | Rebuilt in 0.2.0, untested | Not started |
| 6 | One shared shortcut for all conversions | Done | Rebuilt in 0.2.0, untested | Not started |
| 7 | Amount in Thai words ⇄ number | Done | Built, untested | Not started |
| 8 | พ.ศ. ⇄ ค.ศ. | Done | Built, untested | Not started |
| 9 | Undo last conversion, with its own shortcut | n/a | Rebuilt in 0.2.0, untested | Not started |
| 10 | Excluded apps, with its own window | n/a | Rebuilt in 0.2.0, untested | Not started |
| 11 | Remember language per app | n/a | Not started | Not started |
| 12 | Learned words | Not started | Not started | Not started |
| 13 | Thai and English interface | n/a | Built, untested | Not started |
| 14 | Fully offline | Done | Done (no network code) | Not started |

"Untested" means the code compiles and is in the test build, but has not been confirmed by hand on Windows. Confirmed by @kafaak so far: the app runs, and the double-tap shortcut converts wrong-language text both ways in 0.2.0.

## Work done

| Date | What |
|---|---|
| 2026-11-11 | Reviewed the CursorLingo fork: `feature/selected-numeral-conversion` is pushed, 8 commits ahead of `main`, not merged; all three core test suites pass; macOS smoke test never done. The fork is now parked. |
| 2026-11-11 | Checked feasibility of every requested feature, including one shared shortcut: all feasible. |
| 2026-11-11 | Proposed extra features and six names; Salap chosen. |
| 2026-11-11 | Started this log. |
| 2026-11-11 | @kafaak created the GitHub repo and the local Salap folder. |
| 2026-11-11 | Wrote the conversion engine (`core/`): layout fix, numerals, baht text, year conversion, shared-shortcut logic, UTF-8/UTF-16 helpers, and a C interface for Swift. |
| 2026-11-11 | Wrote the engine tests (`tests/core_tests.cpp`): 295 checks, all passing on Linux, also under address and undefined-behaviour sanitizers. |
| 2026-11-11 | Added CMake build, GitHub Actions CI for Linux, macOS and Windows, README and design notes. |
| 2026-11-11 | Pushed the first commit (`4e935e1`) to `main`. CI passed on Linux, macOS and Windows. Cloned the repo into the local Salap folder; the tests pass there too. |
| 2026-11-11 | Changed year conversion so ค.ศ. is always in Arabic digits, as @kafaak asked. |
| 2026-11-11 | Added `caret_steps` and `place_badge` to the engine; tests now run 9,023 checks. |
| 2026-11-11 | Wrote the Windows app (`windows/Salap.cpp`, version 0.1.0): tray icon and menu, five shortcuts, clipboard-based selection replace with re-selection and clipboard restore, smart cycle, undo, excluded apps, language badge sized to the pointer, Settings window in Thai and English, run at login. It compiles cleanly with MinGW; it has not been run. |
| 2026-11-11 | CI now builds `Salap.exe` on Windows and publishes it as the artifact **Salap-Windows-x64**. |
| 2026-11-11 | @kafaak tried 0.1.0 and sent feedback: double-tap shortcut, drop-down shortcut picker, one shortcut for everything, friendlier excluded-apps screen, adjustable badge position, badge colors, undo shortcut. |
| 2026-11-11 | Engine: added `conversion_cycle`, `layout_fix_likely`, `parse_baht_text`; badge placement now works from the visible cursor shape, a corner and a gap. Tests: 52,639 checks. |
| 2026-11-11 | Windows 0.2.0: all of the feedback above. Settings rebuilt in two columns with live badge preview; new excluded-apps window; double-tap detection on its own thread. Compiles cleanly; not yet run. |
| 2026-11-11 | @kafaak tested 0.2.0: `4kKk8o` ⇄ `ภาษาคน` works; `8'gf=` wrongly became `๘'gf=`; asked for tap-twice on Ctrl only; supplied the app icon. |
| 2026-11-11 | Windows 0.3.0: fixed the `8'gf=` case in the engine, limited tap-twice to Ctrl, added the icon (`windows/Salap.ico`, `assets/icon.png`) and file version details. Tests: 52,644 checks. Compiles cleanly; not yet run. |
| 2026-11-11 | @kafaak tested 0.3.0: year conversion did not work as expected. Asked that it require the พ.ศ. / ค.ศ. label and listed eight spellings to support. |
| 2026-11-11 | Windows 0.3.1: year conversion rewritten around the label, with ranges; cycle order updated; hint text in Settings and on the tray command. Tests: 52,666 checks. Compiles cleanly; not yet run. |
| 2026-11-11 | Windows 0.3.2: Settings now states that unticked conversions remain in the tray menu. |

## Open items

- **Windows 0.3.2 still needs checking by hand:** year conversion, the number cycle, undo, the Settings window, the excluded-apps window, and badge position, distance and colors with a normal and an enlarged pointer.
- English text with a number in it cannot have its digits switched by the shortcut (tray menu only).
- The icon is detailed and blurs at the 16-pixel tray size; a simplified small version would be sharper.
- Wrong-language detection is a spelling-rules hint only; a stronger check is needed before auto-correct.
- How Windows reports the enlarged pointer size is an assumption (`CursorBaseSize` × screen scaling); to be checked on a real PC.
- Re-selecting the converted text assumes one Left-arrow press per Thai character cluster; apps may differ.
- Name "Salap" not yet checked against existing apps and trademarks.
- macOS Thai layout assumed identical to Windows Kedmanee; to be confirmed on a Mac.
- macOS interface will need hands-on testing on @kafaak's Mac.
- Baht text follows the common reading rules; not yet compared against Excel's BAHTTEXT output.
- No installer or code signing yet; Windows will show an "unknown publisher" warning.
