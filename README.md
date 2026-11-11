<p align="center"><img src="assets/icon-256.png" width="128" alt="Salap icon: a Thai letter ส and a Latin letter A on two cards, with arrows swapping them, above a keyboard"></p>

# Salap (สลับ)

Pain point killers for Thai users. A small macOS and Windows app that:

- **Fixes text typed in the wrong keyboard language.** Select `l;ylfu`, press a shortcut, get `สวัสดี`. It works the other way too: `้ำสสน` becomes `hello`.
- **Switches numbers between Thai and Arabic digits.** `1,234.50` ⇄ `๑,๒๓๔.๕๐`, for official documents.
- **Shows the current input language** in a small badge at the mouse pointer.

Salap works offline. It has no network access and no telemetry.

## Status

Early development. Version 0.4.1 for Windows is a test build; there is no macOS app yet and no installer.

| Part | State |
|---|---|
| Conversion engine (`core/`) | Done, tested on Linux, macOS and Windows by CI |
| Windows app (`windows/`) | First test build: tray menu, shortcuts, badge, Settings. Needs hands-on testing |
| Auto-correct while typing | Not started |
| macOS app | Not started |

## Try the Windows test build

1. Open the latest successful run under [Actions](https://github.com/kafaakblog/Salap/actions) and download **Salap-Windows-x64**.
2. Unzip it and run `Salap.exe`. Windows may warn that the file is unsigned; choose **More info → Run anyway**.
3. Settings opens the first time. The Salap icon appears in the tray.
4. In Notepad, type `l;ylfu`, select it, and tap **Ctrl twice**. It becomes `สวัสดี`.
5. Select `1,250` and tap Ctrl twice: `๑,๒๕๐`. Tap twice again and `1,250` is back.
6. Select `พ.ศ. 2569` and tap Ctrl twice: `ค.ศ. 2026`.

The shortcut does three things, depending on what is selected: fixes wrong-language text, switches Thai and Arabic digits, and converts a year that has พ.ศ. or ค.ศ. in front of it. Press it again for the next result, then the original text. **Ctrl+Alt+Z** undoes the last conversion.

Amounts in Thai words are in the menu of the Salap icon near the clock: **Number → Thai words**, **Thai words → Arabic number** and **Thai words → Thai number**. The same menu has every other conversion too.

Both shortcuts, which conversions the shortcut uses, the badge's position, distance and colors, and the apps where Salap stays off are all in Settings.

Salap skips terminals, because it reads the selection with Ctrl+C. Settings are stored in `%APPDATA%\Salap\settings.ini`.

## What the engine does today

| Conversion | Example |
|---|---|
| English keys → Thai | `l;ylfu8iy[` → `สวัสดีครับ` |
| Thai keys → English | `้ำสสน ไนพสก` → `hello world` |
| Arabic → Thai digits | `ข้อ 3 วรรค 2` → `ข้อ ๓ วรรค ๒` |
| Thai → Arabic digits | `๐๐๑,๒๓๔.๕๐` → `001,234.50` |
| Amount in Thai words | `1,234,567.89` → `หนึ่งล้านสองแสนสามหมื่นสี่พันห้าร้อยหกสิบเจ็ดบาทแปดสิบเก้าสตางค์` |
| Thai words → number | `หนึ่งพันสองร้อยห้าสิบบาทถ้วน` → `1,250` |
| พ.ศ. → ค.ศ. | `พ.ศ. ๒๕๖๙` → `ค.ศ. 2026` |
| ค.ศ. → พ.ศ. | `ค.ศ. 2026` → `พ.ศ. 2569` (or `๒๕๖๙` when the text around it uses Thai digits) |

A year is converted only when **พ.ศ.** or **ค.ศ.** is selected with it. `2569` on its own is treated as a number. The label can be written `พ.ศ.`, `พ.ศ`, `พศ.` or `พศ`, with or without a space.

## Build and test the engine

Needs CMake 3.16+ and a C++17 compiler.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Project layout

```
core/      conversion engine, C++17, no dependencies
windows/   Windows app, Win32 and GDI+, one file
tests/     engine tests
docs/      design notes and the project log
```

See [CHANGELOG.md](CHANGELOG.md) for what changed in each version, [docs/DESIGN.md](docs/DESIGN.md) for how it fits together and [docs/LOG.md](docs/LOG.md) for decisions and progress.

To see which version you are running, open the Salap icon's menu and choose **About Salap**.

## License

MIT. See [LICENSE](LICENSE).
