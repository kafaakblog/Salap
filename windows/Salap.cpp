// Salap for Windows. One file, Win32 + GDI+, no other dependencies.
//
// What lives here: the tray icon and menu, global shortcuts, reading and
// replacing the selected text through the clipboard, the language badge at
// the pointer, the Settings window and run-at-login. All text conversion is
// in the shared core (core/include/salap/core.h).

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <climits>
#include <string>
#include <vector>

// GDI+ headers use unqualified min/max.
using std::max;
using std::min;
#include <gdiplus.h>

#include "salap/core.h"

#ifdef _MSC_VER
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define SALAP_VERSION L"0.1.0"

namespace {

// ---------------------------------------------------------------------------
// Text shown to the user, in English and Thai.
// ---------------------------------------------------------------------------

enum Str {
    S_FIX, S_NUM, S_BAHT, S_YEAR, S_UNDO, S_SETTINGS, S_QUIT,
    S_SETTINGS_TITLE, S_RUN_LOGIN, S_BADGE, S_EN_FIX, S_EN_NUM, S_KEEP_SEL,
    S_SHORTCUTS, S_HK_SMART, S_HK_FIX, S_HK_NUM, S_HK_BAHT, S_HK_YEAR,
    S_EXCLUDED, S_LANGUAGE, S_LANG_AUTO, S_SAVE, S_CANCEL, S_HK_FAIL,
    S_COUNT
};

const wchar_t* const kEn[S_COUNT] = {
    L"Fix wrong-language text",
    L"Switch Thai / Arabic numerals",
    L"Amount in Thai words",
    L"Convert year (พ.ศ. ⇄ ค.ศ.)",
    L"Undo last conversion",
    L"Settings…",
    L"Quit Salap",
    L"Salap Settings",
    L"Open Salap when I sign in",
    L"Show the current language next to the pointer",
    L"Fix wrong-language text",
    L"Switch Thai / Arabic numerals",
    L"Keep the converted text selected",
    L"Shortcuts (select text first, then press)",
    L"Smart: fix language or numerals",
    L"Fix language only",
    L"Numerals only",
    L"Amount in Thai words",
    L"Convert year",
    L"Do not run in these apps (program names, separated by commas):",
    L"Language / ภาษา",
    L"Same as Windows",
    L"Save",
    L"Cancel",
    L"These shortcuts are already used by Windows or another app, so Salap could not take them:\n\n",
};

const wchar_t* const kTh[S_COUNT] = {
    L"แก้ข้อความที่พิมพ์ผิดภาษา",
    L"สลับเลขไทย / เลขอารบิก",
    L"แปลงจำนวนเงินเป็นตัวอักษร",
    L"แปลงปี (พ.ศ. ⇄ ค.ศ.)",
    L"ย้อนการแปลงล่าสุด",
    L"ตั้งค่า…",
    L"ออกจาก Salap",
    L"ตั้งค่า Salap",
    L"เปิด Salap เมื่อลงชื่อเข้าใช้",
    L"แสดงภาษาปัจจุบันข้างตัวชี้เมาส์",
    L"แก้ข้อความที่พิมพ์ผิดภาษา",
    L"สลับเลขไทย / เลขอารบิก",
    L"เลือกข้อความที่แปลงแล้วค้างไว้",
    L"ปุ่มลัด (เลือกข้อความก่อน แล้วกด)",
    L"อัจฉริยะ: แก้ภาษาหรือสลับตัวเลข",
    L"แก้ภาษาอย่างเดียว",
    L"สลับตัวเลขอย่างเดียว",
    L"แปลงจำนวนเงินเป็นตัวอักษร",
    L"แปลงปี",
    L"ไม่ทำงานในแอปเหล่านี้ (ชื่อโปรแกรม คั่นด้วยจุลภาค):",
    L"Language / ภาษา",
    L"ตามภาษาของ Windows",
    L"บันทึก",
    L"ยกเลิก",
    L"ปุ่มลัดต่อไปนี้ถูก Windows หรือแอปอื่นใช้อยู่ Salap จึงใช้ไม่ได้:\n\n",
};

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

enum HotkeyId { HK_SMART = 1, HK_FIX, HK_NUM, HK_BAHT, HK_YEAR, HK_COUNT };

struct Settings {
    bool runAtLogin = false;
    bool badge = true;
    bool fixEnabled = true;
    bool numEnabled = true;
    bool keepSelected = true;
    int uiLang = 0;  // 0 = same as Windows, 1 = Thai, 2 = English
    // Each shortcut is (MOD_* flags << 16) | virtual key; 0 means none.
    UINT hotkey[HK_COUNT] = {0, (UINT)((MOD_CONTROL | MOD_ALT) << 16) | 'S', 0, 0, 0, 0};
    std::wstring excluded =
        L"WindowsTerminal.exe, cmd.exe, powershell.exe, pwsh.exe, conhost.exe, "
        L"OpenConsole.exe, mintty.exe, putty.exe, alacritty.exe, wezterm-gui.exe";
};

const wchar_t* const kHotkeyIniName[HK_COUNT] = {L"", L"Smart", L"Fix", L"Numerals", L"BahtText", L"Year"};
const wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kRunValue[] = L"Salap";

HINSTANCE g_inst;
HWND g_main;       // hidden window that owns the tray icon, hotkeys and timer
HWND g_badge;
HWND g_settingsWnd;
HWND g_lastForeground;  // last window of another app that had the focus
Settings g_set;
std::wstring g_iniPath;
bool g_firstRun = false;
UINT g_taskbarCreated;

const wchar_t* T(Str id) {
    bool thai;
    if (g_set.uiLang == 1) thai = true;
    else if (g_set.uiLang == 2) thai = false;
    else thai = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_THAI;
    return (thai ? kTh : kEn)[id];
}

std::wstring ExePath() {
    wchar_t buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf));
    return std::wstring(buf, n);
}

bool ReadRunAtLogin() {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    bool found = RegQueryValueExW(key, kRunValue, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    RegCloseKey(key);
    return found;
}

void WriteRunAtLogin(bool on) {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) return;
    if (on) {
        std::wstring cmd = L"\"" + ExePath() + L"\"";
        RegSetValueExW(key, kRunValue, 0, REG_SZ, (const BYTE*)cmd.c_str(),
                       (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kRunValue);
    }
    RegCloseKey(key);
}

void LoadSettings() {
    wchar_t appdata[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) appdata[0] = 0;
    std::wstring dir = std::wstring(appdata) + L"\\Salap";
    CreateDirectoryW(dir.c_str(), nullptr);
    g_iniPath = dir + L"\\settings.ini";
    g_firstRun = GetFileAttributesW(g_iniPath.c_str()) == INVALID_FILE_ATTRIBUTES;

    const wchar_t* ini = g_iniPath.c_str();
    Settings d;
    g_set.badge = GetPrivateProfileIntW(L"General", L"Badge", d.badge, ini) != 0;
    g_set.fixEnabled = GetPrivateProfileIntW(L"General", L"FixLanguage", d.fixEnabled, ini) != 0;
    g_set.numEnabled = GetPrivateProfileIntW(L"General", L"Numerals", d.numEnabled, ini) != 0;
    g_set.keepSelected = GetPrivateProfileIntW(L"General", L"KeepSelected", d.keepSelected, ini) != 0;
    g_set.uiLang = (int)GetPrivateProfileIntW(L"General", L"Language", d.uiLang, ini);
    if (g_set.uiLang < 0 || g_set.uiLang > 2) g_set.uiLang = 0;
    for (int i = HK_SMART; i < HK_COUNT; ++i)
        g_set.hotkey[i] = GetPrivateProfileIntW(L"Shortcuts", kHotkeyIniName[i], d.hotkey[i], ini);
    wchar_t buf[2048];
    GetPrivateProfileStringW(L"General", L"Excluded", d.excluded.c_str(), buf, ARRAYSIZE(buf), ini);
    g_set.excluded = buf;
    g_set.runAtLogin = ReadRunAtLogin();
}

void SaveSettings() {
    const wchar_t* ini = g_iniPath.c_str();
    if (GetFileAttributesW(ini) == INVALID_FILE_ATTRIBUTES) {
        // Start the file with a UTF-16 mark so Thai program names survive.
        HANDLE f = CreateFileW(ini, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            const wchar_t bom = 0xFEFF;
            DWORD written;
            WriteFile(f, &bom, sizeof(bom), &written, nullptr);
            CloseHandle(f);
        }
    }
    auto putInt = [&](const wchar_t* sec, const wchar_t* key, UINT v) {
        WritePrivateProfileStringW(sec, key, std::to_wstring(v).c_str(), ini);
    };
    putInt(L"General", L"Badge", g_set.badge);
    putInt(L"General", L"FixLanguage", g_set.fixEnabled);
    putInt(L"General", L"Numerals", g_set.numEnabled);
    putInt(L"General", L"KeepSelected", g_set.keepSelected);
    putInt(L"General", L"Language", (UINT)g_set.uiLang);
    for (int i = HK_SMART; i < HK_COUNT; ++i) putInt(L"Shortcuts", kHotkeyIniName[i], g_set.hotkey[i]);
    WritePrivateProfileStringW(L"General", L"Excluded", g_set.excluded.c_str(), ini);
    WriteRunAtLogin(g_set.runAtLogin);
}

// ---------------------------------------------------------------------------
// Shortcuts
// ---------------------------------------------------------------------------

std::wstring HotkeyText(UINT hk) {
    if (!hk) return L"";
    UINT mods = hk >> 16, vk = hk & 0xFFFF;
    std::wstring s;
    if (mods & MOD_CONTROL) s += L"Ctrl+";
    if (mods & MOD_ALT) s += L"Alt+";
    if (mods & MOD_SHIFT) s += L"Shift+";
    if (mods & MOD_WIN) s += L"Win+";
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        s += (wchar_t)vk;
    } else if (vk >= VK_F1 && vk <= VK_F24) {
        s += L"F" + std::to_wstring(vk - VK_F1 + 1);
    } else if (vk == VK_SPACE) {
        s += L"Space";
    } else {
        wchar_t name[64] = L"";
        UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        if (!GetKeyNameTextW((LONG)(sc << 16), name, ARRAYSIZE(name)) || !name[0])
            wsprintfW(name, L"0x%02X", vk);
        s += name;
    }
    return s;
}

bool HotkeyWanted(int id) {
    if (!g_set.hotkey[id]) return false;
    if (id == HK_FIX) return g_set.fixEnabled;
    if (id == HK_NUM) return g_set.numEnabled;
    if (id == HK_SMART) return g_set.fixEnabled || g_set.numEnabled;
    return true;
}

void UnregisterHotkeys() {
    for (int i = HK_SMART; i < HK_COUNT; ++i) UnregisterHotKey(g_main, i);
}

// Returns the shortcuts that could not be registered, one per line.
std::wstring RegisterHotkeys() {
    UnregisterHotkeys();
    std::wstring failed;
    for (int i = HK_SMART; i < HK_COUNT; ++i) {
        if (!HotkeyWanted(i)) continue;
        UINT hk = g_set.hotkey[i];
        if (!RegisterHotKey(g_main, i, (hk >> 16) | MOD_NOREPEAT, hk & 0xFFFF))
            failed += HotkeyText(hk) + L"\n";
    }
    return failed;
}

// ---------------------------------------------------------------------------
// Sending keys
// ---------------------------------------------------------------------------

void AddKey(std::vector<INPUT>& v, WORD vk, bool up) {
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
    if (vk == VK_LEFT) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    v.push_back(in);
}

void SendCtrl(WORD vk) {
    std::vector<INPUT> v;
    AddKey(v, VK_CONTROL, false);
    AddKey(v, vk, false);
    AddKey(v, vk, true);
    AddKey(v, VK_CONTROL, true);
    SendInput((UINT)v.size(), v.data(), sizeof(INPUT));
}

void SendShiftLeft(size_t times) {
    std::vector<INPUT> v;
    AddKey(v, VK_SHIFT, false);
    for (size_t i = 0; i < times; ++i) {
        AddKey(v, VK_LEFT, false);
        AddKey(v, VK_LEFT, true);
    }
    AddKey(v, VK_SHIFT, true);
    SendInput((UINT)v.size(), v.data(), sizeof(INPUT));
}

// A shortcut fires while its modifier keys are still down; keys sent then
// would combine with them. Wait for the user to let go.
void WaitForModifiersUp() {
    const int keys[] = {VK_CONTROL, VK_MENU, VK_SHIFT, VK_LWIN, VK_RWIN};
    for (int i = 0; i < 100; ++i) {
        bool down = false;
        for (int k : keys)
            if (GetAsyncKeyState(k) & 0x8000) down = true;
        if (!down) return;
        Sleep(10);
    }
}

// ---------------------------------------------------------------------------
// Clipboard
// ---------------------------------------------------------------------------

struct ClipItem {
    UINT format;
    std::vector<BYTE> data;
};

bool OpenClipboardRetry() {
    for (int i = 0; i < 20; ++i) {
        if (OpenClipboard(g_main)) return true;
        Sleep(10);
    }
    return false;
}

// Copies everything on the clipboard that is plain memory, so it can be put
// back after Salap has borrowed the clipboard.
void BackupClipboard(std::vector<ClipItem>& items) {
    items.clear();
    if (!OpenClipboardRetry()) return;
    size_t total = 0;
    UINT f = 0;
    while ((f = EnumClipboardFormats(f)) != 0) {
        if (f == CF_BITMAP || f == CF_METAFILEPICT || f == CF_ENHMETAFILE || f == CF_PALETTE ||
            f == CF_OWNERDISPLAY || f == CF_DSPBITMAP || f == CF_DSPMETAFILEPICT ||
            f == CF_DSPENHMETAFILE || (f >= CF_PRIVATEFIRST && f <= CF_PRIVATELAST) ||
            (f >= CF_GDIOBJFIRST && f <= CF_GDIOBJLAST))
            continue;  // handles, not memory; Windows rebuilds the common ones
        HANDLE h = GetClipboardData(f);
        if (!h) continue;
        SIZE_T size = GlobalSize(h);
        if (!size || total + size > 64u * 1024 * 1024) continue;
        void* p = GlobalLock(h);
        if (!p) continue;
        ClipItem item;
        item.format = f;
        item.data.assign((BYTE*)p, (BYTE*)p + size);
        GlobalUnlock(h);
        total += size;
        items.push_back(std::move(item));
    }
    CloseClipboard();
}

bool PutData(UINT format, const void* data, size_t size) {
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, size ? size : 1);
    if (!h) return false;
    void* p = GlobalLock(h);
    if (!p) {
        GlobalFree(h);
        return false;
    }
    memcpy(p, data, size);
    GlobalUnlock(h);
    if (!SetClipboardData(format, h)) {
        GlobalFree(h);
        return false;
    }
    return true;
}

void RestoreClipboard(const std::vector<ClipItem>& items) {
    if (!OpenClipboardRetry()) return;
    EmptyClipboard();
    for (const ClipItem& it : items) PutData(it.format, it.data.data(), it.data.size());
    CloseClipboard();
}

bool ReadClipboardText(std::wstring& out) {
    out.clear();
    if (!OpenClipboardRetry()) return false;
    bool ok = false;
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h) {
        const wchar_t* p = (const wchar_t*)GlobalLock(h);
        if (p) {
            size_t cap = GlobalSize(h) / sizeof(wchar_t), len = 0;
            while (len < cap && p[len]) ++len;
            out.assign(p, len);
            GlobalUnlock(h);
            ok = true;
        }
    }
    CloseClipboard();
    return ok;
}

bool WriteClipboardText(const std::wstring& text) {
    if (!OpenClipboardRetry()) return false;
    EmptyClipboard();
    bool ok = PutData(CF_UNICODETEXT, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
    // Keep this temporary text out of clipboard history and cloud sync.
    DWORD zero = 0;
    PutData(RegisterClipboardFormatW(L"ExcludeClipboardContentFromMonitorProcessing"), &zero, sizeof(zero));
    PutData(RegisterClipboardFormatW(L"CanIncludeInClipboardHistory"), &zero, sizeof(zero));
    PutData(RegisterClipboardFormatW(L"CanUploadToCloudClipboard"), &zero, sizeof(zero));
    CloseClipboard();
    return ok;
}

// Presses Ctrl+C in the focused app and returns what it copied. False means
// nothing was selected (the clipboard did not change).
bool CopySelection(std::wstring& out) {
    DWORD before = GetClipboardSequenceNumber();
    SendCtrl('C');
    for (int i = 0; i < 50; ++i) {
        Sleep(10);
        if (GetClipboardSequenceNumber() != before) {
            Sleep(30);  // let the app finish putting all formats
            return ReadClipboardText(out);
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Which app is in front, and is it excluded
// ---------------------------------------------------------------------------

std::wstring Lower(std::wstring s) {
    if (!s.empty()) CharLowerBuffW(&s[0], (DWORD)s.size());
    return s;
}

std::wstring ProcessName(HWND w) {
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    std::wstring name;
    HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (p) {
        wchar_t buf[MAX_PATH * 4];
        DWORD n = ARRAYSIZE(buf);
        if (QueryFullProcessImageNameW(p, 0, buf, &n)) {
            name.assign(buf, n);
            size_t slash = name.find_last_of(L"\\/");
            if (slash != std::wstring::npos) name = name.substr(slash + 1);
        }
        CloseHandle(p);
    }
    return Lower(name);
}

bool IsExcluded(HWND w) {
    if (!w) return true;
    wchar_t cls[64] = L"";
    GetClassNameW(w, cls, ARRAYSIZE(cls));
    if (!lstrcmpW(cls, L"ConsoleWindowClass")) return true;  // Ctrl+C would interrupt a program
    std::wstring name = ProcessName(w);
    if (name.empty()) return false;
    std::wstring list = Lower(g_set.excluded);
    size_t i = 0;
    while (i < list.size()) {
        size_t j = list.find_first_of(L",;\r\n", i);
        if (j == std::wstring::npos) j = list.size();
        size_t a = i, b = j;
        while (a < b && iswspace(list[a])) ++a;
        while (b > a && iswspace(list[b - 1])) --b;
        if (b > a && list.compare(a, b - a, name) == 0) return true;
        i = j + 1;
    }
    return false;
}

void CALLBACK ForegroundChanged(HWINEVENTHOOK, DWORD, HWND w, LONG, LONG, DWORD, DWORD) {
    if (!w) return;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid == GetCurrentProcessId()) return;
    wchar_t cls[64] = L"";
    GetClassNameW(w, cls, ARRAYSIZE(cls));
    // The taskbar and the tray overflow get the focus on the way to our menu.
    if (!lstrcmpW(cls, L"Shell_TrayWnd") || !lstrcmpW(cls, L"Shell_SecondaryTrayWnd") ||
        !lstrcmpW(cls, L"NotifyIconOverflowWindow") ||
        !lstrcmpW(cls, L"TopLevelWindowForOverflowXamlIsland"))
        return;
    g_lastForeground = w;
}

// ---------------------------------------------------------------------------
// Converting the selection
// ---------------------------------------------------------------------------

enum Kind { K_SMART, K_FIX, K_NUM, K_BAHT, K_YEAR, K_UNDO };

struct LastConversion {
    bool valid = false;
    bool smart = false;
    std::wstring original;  // what the user had selected
    std::wstring result;    // what Salap replaced it with
    salap::Action first = salap::Action::None;
    int stage = 0;          // smart cycle: 0 first guess, 1 the other one, 2 original
    ULONGLONG time = 0;
    HWND window = nullptr;
};
LastConversion g_last;

void Fail() { MessageBeep(MB_ICONWARNING); }

std::u32string Apply(salap::Action a, const std::u32string& text) {
    if (a == salap::Action::FixLanguage && g_set.fixEnabled) return salap::convert_layout(text);
    if (a == salap::Action::Numerals && g_set.numEnabled) return salap::convert_numerals(text);
    return text;
}

std::wstring W(const std::u32string& s) {
    std::u16string u = salap::to_utf16(s);
    return std::wstring(u.begin(), u.end());
}

std::u32string U(const std::wstring& s) {
    return salap::from_utf16(std::u16string(s.begin(), s.end()));
}

void RunAction(Kind kind, bool fromMenu) {
    HWND target = fromMenu ? g_lastForeground : GetForegroundWindow();
    if (!target || !IsWindow(target) || IsExcluded(target)) return Fail();
    if (fromMenu) {
        SetForegroundWindow(target);
        Sleep(150);
        if (GetForegroundWindow() != target) return Fail();
    }
    WaitForModifiersUp();

    std::vector<ClipItem> backup;
    BackupClipboard(backup);

    std::wstring sel;
    if (!CopySelection(sel)) return Fail();  // clipboard untouched: nothing was selected
    auto giveUp = [&]() {
        RestoreClipboard(backup);
        Fail();
    };
    // A trailing line break usually means the app copied a whole line or
    // cell because nothing was selected. Leave those alone.
    if (sel.empty() || sel.size() > 5000 || sel.back() == L'\n' || sel.back() == L'\r')
        return giveUp();

    const bool again = g_last.valid && g_last.window == target && sel == g_last.result &&
                       GetTickCount64() - g_last.time < 30000;
    std::wstring original = again ? g_last.original : sel;
    std::u32string src = U(original);
    std::wstring out;
    LastConversion next;
    next.smart = kind == K_SMART;

    switch (kind) {
        case K_SMART: {
            salap::Action first;
            int stage;
            if (again && g_last.smart) {
                first = g_last.first;
                stage = (g_last.stage + 1) % 3;
            } else {
                first = salap::smart_action(src);
                if (first == salap::Action::FixLanguage && !g_set.fixEnabled) first = salap::Action::Numerals;
                else if (first == salap::Action::Numerals && !g_set.numEnabled) first = salap::Action::FixLanguage;
                stage = 0;
            }
            // Walk the cycle until something differs from what is selected now.
            bool found = false;
            for (int tries = 0; tries < 3 && !found; ++tries, stage = (stage + 1) % 3) {
                std::u32string cand = stage == 0   ? Apply(first, src)
                                      : stage == 1 ? Apply(salap::alternate_action(first), src)
                                                   : src;
                std::wstring w = W(cand);
                if (w != sel) {
                    out = w;
                    next.stage = stage;
                    found = true;
                    break;
                }
            }
            if (!found) return giveUp();
            next.first = first;
            break;
        }
        case K_FIX:
            out = W(salap::convert_layout(U(sel)));
            original = sel;
            break;
        case K_NUM:
            out = W(salap::convert_numerals(U(sel)));
            original = sel;
            break;
        case K_BAHT: {
            std::u32string words;
            if (!salap::baht_text(U(sel), words)) return giveUp();
            out = W(words);
            original = sel;
            break;
        }
        case K_YEAR: {
            int n = 0;
            out = W(salap::convert_years(U(sel), &n));
            if (!n) return giveUp();
            original = sel;
            break;
        }
        case K_UNDO:
            if (!again) return giveUp();
            out = g_last.original;
            break;
    }
    if (out == sel) return giveUp();

    if (!WriteClipboardText(out)) return giveUp();
    SendCtrl('V');
    Sleep(250);  // the app reads the clipboard after it sees the keys
    if (g_set.keepSelected) {
        size_t steps = salap::caret_steps(U(out));
        if (steps > 0 && steps <= 400) {
            SendShiftLeft(steps);
            Sleep(30);
        }
    }
    RestoreClipboard(backup);

    next.valid = kind != K_UNDO;
    next.original = original;
    next.result = out;
    next.time = GetTickCount64();
    next.window = target;
    g_last = next;
}

// ---------------------------------------------------------------------------
// Drawing: tray icon and badge
// ---------------------------------------------------------------------------

int g_dpi = 96;
int Dp(int v) { return MulDiv(v, g_dpi, 96); }

void RoundedRect(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float r) {
    float d = r * 2;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();
}

// A rounded tile with centred white text, with per-pixel transparency.
Gdiplus::Bitmap* DrawTile(int w, int h, Gdiplus::Color fill, const wchar_t* text, float fontPx,
                          const wchar_t* fontName) {
    auto* bmp = new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB);
    Gdiplus::Graphics g(bmp);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    g.Clear(Gdiplus::Color(0, 0, 0, 0));
    Gdiplus::GraphicsPath path;
    RoundedRect(path, 0.5f, 0.5f, (float)w - 1, (float)h - 1, (float)min(w, h) * 0.24f);
    Gdiplus::SolidBrush bg(fill);
    g.FillPath(&bg, &path);
    Gdiplus::FontFamily family(fontName);
    Gdiplus::Font font(&family, fontPx, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat fmt;
    fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::SolidBrush white(Gdiplus::Color(255, 255, 255, 255));
    g.DrawString(text, -1, &font, Gdiplus::RectF(0, 0, (float)w, (float)h), &fmt, &white);
    return bmp;
}

HICON MakeTrayIcon() {
    int size = GetSystemMetrics(SM_CXSMICON);
    if (size < 16) size = 16;
    Gdiplus::Bitmap* bmp = DrawTile(size, size, Gdiplus::Color(255, 37, 99, 235), L"ส",
                                    (float)size * 0.72f, L"Leelawadee UI");
    HICON icon = nullptr;
    bmp->GetHICON(&icon);
    delete bmp;
    return icon;
}

struct BadgeState {
    std::wstring code;      // what is drawn now
    int w = 0, h = 0;
    POINT pos = {LONG_MIN, LONG_MIN};
    bool visible = false;
    HCURSOR cursor = nullptr;  // cursor the cached metrics belong to
    int cw = 32, ch = 32, hx = 0, hy = 0;
    int baseSize = 32;         // Windows "mouse pointer size" setting
    int tick = 0;
} g_b;

void ReadPointerSize() {
    DWORD v = 32, size = sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Control Panel\\Cursors", L"CursorBaseSize", RRF_RT_REG_DWORD,
                     nullptr, &v, &size) != ERROR_SUCCESS || v < 16 || v > 512)
        v = 32;
    g_b.baseSize = (int)v;
    g_b.cursor = nullptr;  // re-measure
}

// Size and hotspot of the cursor as it appears on screen.
void MeasureCursor(HCURSOR cur) {
    if (cur == g_b.cursor) return;
    g_b.cursor = cur;
    int w = 32, h = 32, hx = 0, hy = 0;
    ICONINFO ii;
    if (cur && GetIconInfo(cur, &ii)) {
        BITMAP bm = {};
        if (GetObjectW(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm)) {
            w = bm.bmWidth;
            h = ii.hbmColor ? bm.bmHeight : bm.bmHeight / 2;  // a mask holds two images
        }
        hx = (int)ii.xHotspot;
        hy = (int)ii.yHotspot;
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
    }
    if (w < 1) w = 32;
    if (h < 1) h = 32;
    // The enlarged pointer from Settings > Accessibility is drawn bigger
    // than the cursor's own image; scale up to that size if it is larger.
    int shown = max(w, MulDiv(g_b.baseSize, g_dpi, 96));
    g_b.cw = shown;
    g_b.ch = MulDiv(h, shown, w);
    g_b.hx = MulDiv(hx, shown, w);
    g_b.hy = MulDiv(hy, shown, w);
}

std::wstring CurrentLanguageCode(LANGID* langOut) {
    HWND fg = GetForegroundWindow();
    DWORD tid = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
    GUITHREADINFO gi = {};
    gi.cbSize = sizeof(gi);
    if (tid && GetGUIThreadInfo(tid, &gi) && gi.hwndFocus) {
        DWORD t = GetWindowThreadProcessId(gi.hwndFocus, nullptr);
        if (t) tid = t;
    }
    LANGID lang = LOWORD((UINT_PTR)GetKeyboardLayout(tid));
    if (!lang) return L"";  // consoles do not report one; keep what we had
    *langOut = lang;
    if (PRIMARYLANGID(lang) == LANG_THAI) return L"TH";
    if (PRIMARYLANGID(lang) == LANG_ENGLISH) return L"EN";
    wchar_t iso[16] = L"";
    GetLocaleInfoW(MAKELCID(lang, SORT_DEFAULT), LOCALE_SISO639LANGNAME, iso, ARRAYSIZE(iso));
    std::wstring s(iso);
    if (s.size() > 2) s.resize(2);
    if (!s.empty()) CharUpperBuffW(&s[0], (DWORD)s.size());
    return s.empty() ? L"??" : s;
}

void RedrawBadge(const std::wstring& code, LANGID lang) {
    int w = Dp(28), h = Dp(18);
    Gdiplus::Color color = PRIMARYLANGID(lang) == LANG_THAI      ? Gdiplus::Color(235, 37, 99, 235)
                           : PRIMARYLANGID(lang) == LANG_ENGLISH ? Gdiplus::Color(235, 22, 163, 74)
                                                                 : Gdiplus::Color(235, 75, 85, 99);
    Gdiplus::Bitmap* bmp = DrawTile(w, h, color, code.c_str(), (float)Dp(12), L"Segoe UI");
    HBITMAP hbm = nullptr;
    bmp->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hbm);
    delete bmp;
    if (!hbm) return;
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ old = SelectObject(mem, hbm);
    SIZE size = {w, h};
    POINT src = {0, 0};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_badge, screen, nullptr, &size, mem, &src, 0, &blend, ULW_ALPHA);
    SelectObject(mem, old);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    DeleteObject(hbm);
    g_b.code = code;
    g_b.w = w;
    g_b.h = h;
}

void HideBadge() {
    if (g_b.visible) {
        ShowWindow(g_badge, SW_HIDE);
        g_b.visible = false;
    }
}

void UpdateBadge() {
    if (!g_set.badge) return HideBadge();

    CURSORINFO ci = {};
    ci.cbSize = sizeof(ci);
    if (!GetCursorInfo(&ci) || !(ci.flags & CURSOR_SHOWING)) return HideBadge();

    // The language is cheap to read but there is no need to do it 60 times a second.
    if (g_b.tick++ % 6 == 0 || g_b.code.empty()) {
        LANGID lang = 0;
        std::wstring code = CurrentLanguageCode(&lang);
        if (!code.empty() && code != g_b.code) RedrawBadge(code, lang);
    }
    if (g_b.code.empty()) return HideBadge();

    MeasureCursor(ci.hCursor);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(MonitorFromPoint(ci.ptScreenPos, MONITOR_DEFAULTTONEAREST), &mi);

    salap::BadgeInput in = {};
    in.pointer = {ci.ptScreenPos.x, ci.ptScreenPos.y};
    in.cursor_w = g_b.cw;
    in.cursor_h = g_b.ch;
    in.hot_x = g_b.hx;
    in.hot_y = g_b.hy;
    in.badge_w = g_b.w;
    in.badge_h = g_b.h;
    in.gap = Dp(3);
    in.screen = {mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom};
    salap::Point p = salap::place_badge(in);

    if (!g_b.visible || p.x != g_b.pos.x || p.y != g_b.pos.y) {
        SetWindowPos(g_badge, HWND_TOPMOST, p.x, p.y, 0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        g_b.pos = {p.x, p.y};
        g_b.visible = true;
    }
}

// ---------------------------------------------------------------------------
// Tray icon and menu
// ---------------------------------------------------------------------------

const UINT WM_TRAY = WM_APP + 1;
enum MenuId { M_FIX = 1, M_NUM, M_BAHT, M_YEAR, M_UNDO, M_SETTINGS, M_QUIT };

HICON g_trayIcon;

void AddTrayIcon() {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_main;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY;
    nid.hIcon = g_trayIcon;
    lstrcpynW(nid.szTip, L"Salap " SALAP_VERSION, ARRAYSIZE(nid.szTip));
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_main;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void OpenSettings();

void ShowMenu() {
    HMENU m = CreatePopupMenu();
    auto item = [&](UINT id, Str label, int hk, bool enabled) {
        std::wstring s = T(label);
        if (hk && g_set.hotkey[hk] && HotkeyWanted(hk)) s += L"\t" + HotkeyText(g_set.hotkey[hk]);
        AppendMenuW(m, MF_STRING | (enabled ? 0 : MF_GRAYED), id, s.c_str());
    };
    item(M_FIX, S_FIX, g_set.hotkey[HK_FIX] ? HK_FIX : HK_SMART, g_set.fixEnabled);
    item(M_NUM, S_NUM, g_set.hotkey[HK_NUM] ? HK_NUM : 0, g_set.numEnabled);
    item(M_BAHT, S_BAHT, HK_BAHT, true);
    item(M_YEAR, S_YEAR, HK_YEAR, true);
    item(M_UNDO, S_UNDO, 0, g_last.valid);
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    item(M_SETTINGS, S_SETTINGS, 0, true);
    item(M_QUIT, S_QUIT, 0, true);

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_main);  // so the menu closes when the user clicks away
    UINT cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, g_main, nullptr);
    DestroyMenu(m);
    PostMessageW(g_main, WM_NULL, 0, 0);

    switch (cmd) {
        case M_FIX: RunAction(K_FIX, true); break;
        case M_NUM: RunAction(K_NUM, true); break;
        case M_BAHT: RunAction(K_BAHT, true); break;
        case M_YEAR: RunAction(K_YEAR, true); break;
        case M_UNDO: RunAction(K_UNDO, true); break;
        case M_SETTINGS: OpenSettings(); break;
        case M_QUIT: DestroyWindow(g_main); break;
    }
}

// ---------------------------------------------------------------------------
// Settings window
// ---------------------------------------------------------------------------

enum CtrlId {
    C_RUN = 100, C_BADGE, C_ENFIX, C_ENNUM, C_KEEP,
    C_HK_FIRST,  // + HotkeyId
    C_EXCLUDED = 120, C_LANG, C_SAVE, C_CANCEL
};

HFONT g_font;

WORD ToHotkeyCtl(UINT hk) {
    UINT mods = hk >> 16;
    BYTE f = 0;
    if (mods & MOD_SHIFT) f |= HOTKEYF_SHIFT;
    if (mods & MOD_CONTROL) f |= HOTKEYF_CONTROL;
    if (mods & MOD_ALT) f |= HOTKEYF_ALT;
    return MAKEWORD(hk & 0xFF, f);
}

UINT FromHotkeyCtl(WORD v) {
    BYTE vk = LOBYTE(v), f = HIBYTE(v);
    if (!vk) return 0;
    UINT mods = 0;
    if (f & HOTKEYF_SHIFT) mods |= MOD_SHIFT;
    if (f & HOTKEYF_CONTROL) mods |= MOD_CONTROL;
    if (f & HOTKEYF_ALT) mods |= MOD_ALT;
    if (!mods) return 0;  // a bare key would swallow ordinary typing
    return (mods << 16) | vk;
}

HWND Ctl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, DWORD exStyle, int x,
         int y, int w, int h, int id) {
    HWND c = CreateWindowExW(exStyle, cls, text, WS_CHILD | WS_VISIBLE | style, Dp(x), Dp(y), Dp(w),
                             Dp(h), parent, (HMENU)(INT_PTR)id, g_inst, nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

void BuildSettings(HWND w) {
    const int L = 20, W_ = 420;
    int y = 16;
    auto check = [&](int id, Str label, bool on) {
        HWND c = Ctl(w, L"BUTTON", T(label), BS_AUTOCHECKBOX | WS_TABSTOP, 0, L, y, W_, 22, id);
        SendMessageW(c, BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
        y += 26;
    };
    check(C_RUN, S_RUN_LOGIN, g_set.runAtLogin);
    check(C_BADGE, S_BADGE, g_set.badge);
    check(C_ENFIX, S_EN_FIX, g_set.fixEnabled);
    check(C_ENNUM, S_EN_NUM, g_set.numEnabled);
    check(C_KEEP, S_KEEP_SEL, g_set.keepSelected);

    y += 8;
    Ctl(w, L"STATIC", T(S_SHORTCUTS), 0, 0, L, y, W_, 20, -1);
    y += 26;
    const Str labels[HK_COUNT] = {S_COUNT, S_HK_SMART, S_HK_FIX, S_HK_NUM, S_HK_BAHT, S_HK_YEAR};
    for (int i = HK_SMART; i < HK_COUNT; ++i) {
        Ctl(w, L"STATIC", T(labels[i]), 0, 0, L + 12, y + 3, 230, 20, -1);
        HWND h = Ctl(w, HOTKEY_CLASSW, L"", WS_TABSTOP, WS_EX_CLIENTEDGE, L + 250, y, 170, 24, C_HK_FIRST + i);
        SendMessageW(h, HKM_SETHOTKEY, ToHotkeyCtl(g_set.hotkey[i]), 0);
        // Plain keys and Shift+key are for typing; turn them into Ctrl+Alt.
        SendMessageW(h, HKM_SETRULES, HKCOMB_NONE | HKCOMB_S, MAKELPARAM(HOTKEYF_CONTROL | HOTKEYF_ALT, 0));
        y += 30;
    }

    y += 8;
    Ctl(w, L"STATIC", T(S_EXCLUDED), 0, 0, L, y, W_, 20, -1);
    y += 22;
    Ctl(w, L"EDIT", g_set.excluded.c_str(), ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP,
        WS_EX_CLIENTEDGE, L, y, W_, 56, C_EXCLUDED);
    y += 68;

    Ctl(w, L"STATIC", T(S_LANGUAGE), 0, 0, L, y + 3, 150, 20, -1);
    HWND combo = Ctl(w, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0, L + 250, y, 170, 200, C_LANG);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)T(S_LANG_AUTO));
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"ไทย");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(combo, CB_SETCURSEL, g_set.uiLang, 0);
    y += 44;

    Ctl(w, L"BUTTON", T(S_SAVE), BS_DEFPUSHBUTTON | WS_TABSTOP, 0, L + W_ - 200, y, 95, 28, C_SAVE);
    Ctl(w, L"BUTTON", T(S_CANCEL), WS_TABSTOP, 0, L + W_ - 95, y, 95, 28, C_CANCEL);
    y += 28 + 18;

    RECT r = {0, 0, Dp(W_ + 2 * L), Dp(y)};
    AdjustWindowRectEx(&r, (DWORD)GetWindowLongPtrW(w, GWL_STYLE), FALSE, (DWORD)GetWindowLongPtrW(w, GWL_EXSTYLE));
    int ww = r.right - r.left, wh = r.bottom - r.top;
    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    SetWindowPos(w, nullptr, work.left + (work.right - work.left - ww) / 2,
                 work.top + (work.bottom - work.top - wh) / 2, ww, wh, SWP_NOZORDER);
}

void SaveFromSettings(HWND w) {
    auto checked = [&](int id) { return IsDlgButtonChecked(w, id) == BST_CHECKED; };
    g_set.runAtLogin = checked(C_RUN);
    g_set.badge = checked(C_BADGE);
    g_set.fixEnabled = checked(C_ENFIX);
    g_set.numEnabled = checked(C_ENNUM);
    g_set.keepSelected = checked(C_KEEP);
    for (int i = HK_SMART; i < HK_COUNT; ++i)
        g_set.hotkey[i] = FromHotkeyCtl((WORD)SendDlgItemMessageW(w, C_HK_FIRST + i, HKM_GETHOTKEY, 0, 0));
    wchar_t buf[2048];
    GetDlgItemTextW(w, C_EXCLUDED, buf, ARRAYSIZE(buf));
    g_set.excluded = buf;
    for (wchar_t& c : g_set.excluded)
        if (c == L'\r' || c == L'\n') c = L' ';  // one line in the settings file
    int lang = (int)SendDlgItemMessageW(w, C_LANG, CB_GETCURSEL, 0, 0);
    g_set.uiLang = lang < 0 ? 0 : lang;
    SaveSettings();
}

LRESULT CALLBACK SettingsProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wp) == C_SAVE || LOWORD(wp) == IDOK) {
                SaveFromSettings(w);
                DestroyWindow(w);
            } else if (LOWORD(wp) == C_CANCEL || LOWORD(wp) == IDCANCEL) {
                DestroyWindow(w);
            }
            return 0;
        case WM_CTLCOLORSTATIC:
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        case WM_CLOSE:
            DestroyWindow(w);
            return 0;
        case WM_DESTROY: {
            g_settingsWnd = nullptr;
            std::wstring failed = RegisterHotkeys();
            if (!failed.empty())
                MessageBoxW(nullptr, (std::wstring(T(S_HK_FAIL)) + failed).c_str(), L"Salap",
                            MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
            return 0;
        }
    }
    return DefWindowProcW(w, msg, wp, lp);
}

void OpenSettings() {
    if (g_settingsWnd) {
        SetForegroundWindow(g_settingsWnd);
        return;
    }
    UnregisterHotkeys();  // so shortcuts can be typed into the boxes
    g_settingsWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"SalapSettings", T(S_SETTINGS_TITLE),
                                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, 0, 0, 100,
                                    100, nullptr, nullptr, g_inst, nullptr);
    if (!g_settingsWnd) {
        RegisterHotkeys();
        return;
    }
    BuildSettings(g_settingsWnd);
    SendMessageW(g_settingsWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_trayIcon);
    ShowWindow(g_settingsWnd, SW_SHOW);
    SetForegroundWindow(g_settingsWnd);
}

// ---------------------------------------------------------------------------
// Main window
// ---------------------------------------------------------------------------

LRESULT CALLBACK MainProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_HOTKEY:
            switch (wp) {
                case HK_SMART: RunAction(K_SMART, false); break;
                case HK_FIX: RunAction(K_FIX, false); break;
                case HK_NUM: RunAction(K_NUM, false); break;
                case HK_BAHT: RunAction(K_BAHT, false); break;
                case HK_YEAR: RunAction(K_YEAR, false); break;
            }
            return 0;
        case WM_TIMER:
            UpdateBadge();
            return 0;
        case WM_TRAY:
            if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_CONTEXTMENU) ShowMenu();
            else if (LOWORD(lp) == WM_LBUTTONUP) ShowMenu();
            else if (LOWORD(lp) == WM_LBUTTONDBLCLK) OpenSettings();
            return 0;
        case WM_SETTINGCHANGE:
        case WM_DISPLAYCHANGE:
            ReadPointerSize();
            return 0;
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;
    }
    if (msg == g_taskbarCreated && g_taskbarCreated) {
        AddTrayIcon();  // Explorer restarted
        return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
    g_inst = inst;

    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\SalapSingleInstance");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HDC screen = GetDC(nullptr);
    g_dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_HOTKEY_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);
    Gdiplus::GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken = 0;
    Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, nullptr);

    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    g_font = CreateFontIndirectW(&ncm.lfMessageFont);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.hInstance = inst;
    wc.lpfnWndProc = MainProc;
    wc.lpszClassName = L"SalapMain";
    RegisterClassExW(&wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.lpszClassName = L"SalapBadge";
    RegisterClassExW(&wc);
    wc.lpfnWndProc = SettingsProc;
    wc.lpszClassName = L"SalapSettings";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    RegisterClassExW(&wc);

    g_main = CreateWindowExW(0, L"SalapMain", L"Salap", 0, 0, 0, 0, 0, nullptr, nullptr, inst, nullptr);
    g_badge = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                              L"SalapBadge", L"", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, inst, nullptr);
    if (!g_main || !g_badge) return 1;

    LoadSettings();
    ReadPointerSize();
    g_trayIcon = MakeTrayIcon();
    g_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    AddTrayIcon();

    g_lastForeground = GetForegroundWindow();
    HWINEVENTHOOK hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
                                         ForegroundChanged, 0, 0, WINEVENT_OUTOFCONTEXT);
    SetTimer(g_main, 1, 16, nullptr);

    if (g_firstRun) {
        SaveSettings();
        OpenSettings();  // also registers the shortcuts when it closes
    } else {
        std::wstring failed = RegisterHotkeys();
        if (!failed.empty())
            MessageBoxW(nullptr, (std::wstring(T(S_HK_FAIL)) + failed).c_str(), L"Salap",
                        MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (g_settingsWnd && IsDialogMessageW(g_settingsWnd, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hook) UnhookWinEvent(hook);
    UnregisterHotkeys();
    if (g_trayIcon) DestroyIcon(g_trayIcon);
    if (g_font) DeleteObject(g_font);
    Gdiplus::GdiplusShutdown(gdiToken);
    if (mutex) CloseHandle(mutex);
    return 0;
}
