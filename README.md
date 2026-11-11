# Salap (สลับ)

Pain point killers for Thai users. A small macOS and Windows app that:

- **Fixes text typed in the wrong keyboard language.** Select `l;ylfu`, press a shortcut, get `สวัสดี`. It works the other way too: `้ำสสน` becomes `hello`.
- **Switches numbers between Thai and Arabic digits.** `1,234.50` ⇄ `๑,๒๓๔.๕๐`, for official documents.
- **Shows the current input language** in a small badge at the mouse pointer.

Salap works offline. It has no network access and no telemetry.

## Status

Early development. Version 0.1.0 for Windows is a first test build; there is no macOS app yet and no installer.

| Part | State |
|---|---|
| Conversion engine (`core/`) | Done, tested on Linux, macOS and Windows by CI |
| Windows app (`windows/`) | First test build: tray menu, shortcuts, badge, Settings. Needs hands-on testing |
| Auto-correct while typing | Not started |
| macOS app | Not started |

## Try the Windows test build

1. Open the latest successful run under [Actions](https://github.com/kafaakblog/Salap/actions) and download **Salap-Windows-x64**.
2. Unzip it and run `Salap.exe`. Windows may warn that the file is unsigned; choose **More info → Run anyway**.
3. Settings opens the first time. A blue **ส** icon appears in the tray.
4. In Notepad, type `l;ylfu`, select it, and press **Ctrl+Alt+S**. It becomes `สวัสดี`. Select `1,234` and press it again for `๑,๒๓๔`.

Pressing the shortcut again on the converted text switches to the other conversion, then back to the original. The tray menu has the same commands plus amount-in-words and year conversion.

Salap skips terminals, because it reads the selection with Ctrl+C. Settings are stored in `%APPDATA%\Salap\settings.ini`.

## What the engine does today

| Conversion | Example |
|---|---|
| English keys → Thai | `l;ylfu8iy[` → `สวัสดีครับ` |
| Thai keys → English | `้ำสสน ไนพสก` → `hello world` |
| Arabic → Thai digits | `ข้อ 3 วรรค 2` → `ข้อ ๓ วรรค ๒` |
| Thai → Arabic digits | `๐๐๑,๒๓๔.๕๐` → `001,234.50` |
| Amount in Thai words | `1,234,567.89` → `หนึ่งล้านสองแสนสามหมื่นสี่พันห้าร้อยหกสิบเจ็ดบาทแปดสิบเก้าสตางค์` |
| พ.ศ. → ค.ศ. | `พ.ศ. ๒๕๖๙` → `ค.ศ. 2026` |
| ค.ศ. → พ.ศ. | `ค.ศ. 2026` → `พ.ศ. 2569` (or `๒๕๖๙` when the text around it uses Thai digits) |

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

See [docs/DESIGN.md](docs/DESIGN.md) for how it fits together and [docs/LOG.md](docs/LOG.md) for decisions and progress.

## License

MIT. See [LICENSE](LICENSE).
