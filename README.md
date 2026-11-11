# Salap (สลับ)

Pain point killers for Thai users. A small macOS and Windows app that:

- **Fixes text typed in the wrong keyboard language.** Select `l;ylfu`, press a shortcut, get `สวัสดี`. It works the other way too: `้ำสสน` becomes `hello`.
- **Switches numbers between Thai and Arabic digits.** `1,234.50` ⇄ `๑,๒๓๔.๕๐`, for official documents.
- **Shows the current input language** in a small badge at the mouse pointer.

Salap works offline. It has no network access and no telemetry.

## Status

Early development. The conversion engine is written and tested; the macOS and Windows apps are not built yet, so there is nothing to install.

| Part | State |
|---|---|
| Conversion engine (`core/`) | Done, tested on Linux, macOS and Windows by CI |
| Wrong-language detection for auto-correct | Not started |
| Windows app | Not started |
| macOS app | Not started |

## What the engine does today

| Conversion | Example |
|---|---|
| English keys → Thai | `l;ylfu8iy[` → `สวัสดีครับ` |
| Thai keys → English | `้ำสสน ไนพสก` → `hello world` |
| Arabic → Thai digits | `ข้อ 3 วรรค 2` → `ข้อ ๓ วรรค ๒` |
| Thai → Arabic digits | `๐๐๑,๒๓๔.๕๐` → `001,234.50` |
| Amount in Thai words | `1,234,567.89` → `หนึ่งล้านสองแสนสามหมื่นสี่พันห้าร้อยหกสิบเจ็ดบาทแปดสิบเก้าสตางค์` |
| พ.ศ. ⇄ ค.ศ. | `พ.ศ. ๒๕๖๙` ⇄ `ค.ศ. ๒๐๒๖` |

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
tests/     engine tests
docs/      design notes and the project log
```

See [docs/DESIGN.md](docs/DESIGN.md) for how it fits together and [docs/LOG.md](docs/LOG.md) for decisions and progress.

## License

MIT. See [LICENSE](LICENSE).
