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
| 2026-11-11 | Year conversion reads 2400–2700 as พ.ศ. and 1857–2157 as ค.ศ. | The ranges cover the same years and cannot be confused. Chosen by Claude. |
| 2026-11-11 | A ค.ศ. year is always written in Arabic digits: พ.ศ. ๒๕๖๙ → ค.ศ. 2026. | Requested by @kafaak. |
| 2026-11-11 | A พ.ศ. year is written in Thai digits when the selected text has any Thai digit, otherwise in Arabic digits. | So the year matches the document around it. Chosen by Claude; to be confirmed by @kafaak. |
| 2026-11-11 | Pushing straight to `main` while the project is being set up. | Solo project, empty repo. Chosen by Claude; @kafaak can ask for branches and pull requests. |
| 2026-11-11 | Windows app reads and replaces the selection through the clipboard (Ctrl+C, Ctrl+V), then restores the clipboard. | Windows has no general API for another app's selection. |
| 2026-11-11 | Default shortcut: **Ctrl+Alt+S** for the smart conversion; the other commands have none by default. | One shortcut to remember; S for Salap. Chosen by Claude. |
| 2026-11-11 | The smart shortcut cycles: first guess → the other conversion → original text. | Gives "press again to switch" and an undo with one key. |
| 2026-11-11 | Terminals are excluded by default; the list is editable. | Ctrl+C interrupts programs in a terminal. |
| 2026-11-11 | The badge follows the pointer with a 16 ms timer, not a mouse hook. | A hook would make the mouse stutter while Salap is busy converting. |

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
| 4 | Language badge at the pointer | Placement done | Built, untested | Not started |
| 5 | Settings: run at login, feature switches, shortcuts | n/a | Built, untested | Not started |
| 6 | One shared shortcut | Done | Built, untested | Not started |
| 7 | Amount in Thai words | Done | Built, untested | Not started |
| 8 | พ.ศ. ⇄ ค.ศ. | Done | Built, untested | Not started |
| 9 | Undo last conversion | n/a | Built, untested | Not started |
| 10 | Excluded apps | n/a | Built, untested | Not started |
| 11 | Remember language per app | n/a | Not started | Not started |
| 12 | Learned words | Not started | Not started | Not started |
| 13 | Thai and English interface | n/a | Built, untested | Not started |
| 14 | Fully offline | Done | Done (no network code) | Not started |

"Built, untested" means the code compiles and is in the test build, but nobody has run it on Windows yet.

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

## Open items

- **The Windows app has never been run.** It needs a hands-on test: conversion in Notepad, Word and a browser; the badge with a normal and an enlarged pointer; Settings; run at login.
- How Windows reports the enlarged pointer size is an assumption (`CursorBaseSize` × screen scaling); to be checked on a real PC.
- Re-selecting the converted text assumes one Left-arrow press per Thai character cluster; apps may differ.
- Name "Salap" not yet checked against existing apps and trademarks.
- macOS Thai layout assumed identical to Windows Kedmanee; to be confirmed on a Mac.
- macOS interface will need hands-on testing on @kafaak's Mac.
- Baht text follows the common reading rules; not yet compared against Excel's BAHTTEXT output.
- No app icon, installer or code signing yet; Windows will show an "unknown publisher" warning.
