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
#include <commdlg.h>

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

#define SALAP_VERSION L"0.4.0"

namespace {

// ---------------------------------------------------------------------------
// Text shown to the user, in English and Thai.
// ---------------------------------------------------------------------------

enum Str {
    S_CONVERT, S_FIX, S_NUM, S_YEAR, S_TO_WORDS, S_WORDS_AR, S_WORDS_TH, S_UNDO, S_SETTINGS, S_QUIT,
    S_SETTINGS_TITLE, S_SEC_GENERAL, S_RUN_LOGIN, S_LANGUAGE, S_LANG_AUTO,
    S_SEC_CONVERT, S_SC_MAIN, S_SC_UNDO, S_TAP_TWICE, S_COMBO,
    S_FEATURES, S_F_FIX, S_F_NUM, S_F_YEAR, S_KEEP_SEL, S_EXCLUDED_BTN,
    S_SEC_BADGE, S_BADGE, S_POSITION, S_POS_BR, S_POS_BL, S_POS_TR, S_POS_TL,
    S_DISTANCE, S_COLOR_TH, S_COLOR_EN,
    S_SAVE, S_CANCEL, S_HK_FAIL,
    S_EX_TITLE, S_EX_INFO, S_EX_BUILTIN, S_EX_ADD_RUNNING, S_EX_BROWSE, S_EX_REMOVE,
    S_EX_CLOSE, S_EX_NONE_RUNNING, S_YEAR_HINT, S_FEATURES_NOTE,
    S_HINT_NUMBER, S_HINT_WORDS,
    S_COUNT
};

const wchar_t* const kEn[] = {
    L"Convert selected text",
    L"Fix wrong-language text",
    L"Switch Thai / Arabic numerals",
    L"Convert year (พ.ศ. ⇄ ค.ศ.)",
    L"Number → Thai words",
    L"Thai words → Arabic number",
    L"Thai words → Thai number",
    L"Undo last conversion",
    L"Settings…",
    L"Quit Salap",
    L"Salap Settings",
    L"General",
    L"Open Salap when I sign in",
    L"Language / ภาษา",
    L"Same as Windows",
    L"Convert selected text",
    L"Shortcut to convert",
    L"Shortcut to undo",
    L"Tap Ctrl twice",
    L"Keys together:",
    L"The shortcut does these. Press it again for the next one:",
    L"Fix text typed in the wrong language",
    L"Switch Thai / Arabic numerals",
    L"Convert year: select พ.ศ. or ค.ศ. together with the year",
    L"Keep the converted text selected",
    L"Apps where Salap is off…",
    L"Language badge",
    L"Show the current language next to the pointer",
    L"Position",
    L"Below, right",
    L"Below, left",
    L"Above, right",
    L"Above, left",
    L"Distance from the pointer",
    L"Color for Thai",
    L"Color for English",
    L"Save",
    L"Cancel",
    L"These shortcuts are already used by Windows or another app, so Salap could not take them:\n\n",
    L"Apps where Salap is off",
    L"Salap does not convert text in these apps.",
    L"  (built in)",
    L"Add an open app…",
    L"Browse…",
    L"Remove",
    L"Close",
    L"(no other apps are open)",
    L"To convert a year, select it together with พ.ศ. or ค.ศ.\n\nFor example: select “พ.ศ. 2569”, not just “2569”.",
    L"These, and amounts in Thai words, are also in the menu of the Salap icon near the clock.",
    L"Select a number first, such as 1,250 or ๑,๒๕๐.50.",
    L"Select an amount written in Thai words first, such as หนึ่งพันสองร้อยห้าสิบบาทถ้วน.",
};

const wchar_t* const kTh[] = {
    L"แปลงข้อความที่เลือก",
    L"แก้ข้อความที่พิมพ์ผิดภาษา",
    L"สลับเลขไทย / เลขอารบิก",
    L"แปลงปี (พ.ศ. ⇄ ค.ศ.)",
    L"ตัวเลข → ตัวอักษร (บาท)",
    L"ตัวอักษร (บาท) → เลขอารบิก",
    L"ตัวอักษร (บาท) → เลขไทย",
    L"ย้อนการแปลงล่าสุด",
    L"ตั้งค่า…",
    L"ออกจาก Salap",
    L"ตั้งค่า Salap",
    L"ทั่วไป",
    L"เปิด Salap เมื่อลงชื่อเข้าใช้",
    L"Language / ภาษา",
    L"ตามภาษาของ Windows",
    L"แปลงข้อความที่เลือก",
    L"ปุ่มลัดสำหรับแปลง",
    L"ปุ่มลัดสำหรับย้อนกลับ",
    L"กด Ctrl สองครั้ง",
    L"กดพร้อมกัน:",
    L"ปุ่มลัดจะทำสิ่งเหล่านี้ กดซ้ำเพื่อเปลี่ยนเป็นแบบถัดไป:",
    L"แก้ข้อความที่พิมพ์ผิดภาษา",
    L"สลับเลขไทย / เลขอารบิก",
    L"แปลงปี: ต้องเลือกคำว่า พ.ศ. หรือ ค.ศ. พร้อมกับปี",
    L"เลือกข้อความที่แปลงแล้วค้างไว้",
    L"แอปที่ไม่ให้ Salap ทำงาน…",
    L"ป้ายบอกภาษา",
    L"แสดงภาษาปัจจุบันข้างตัวชี้เมาส์",
    L"ตำแหน่ง",
    L"ล่างขวา",
    L"ล่างซ้าย",
    L"บนขวา",
    L"บนซ้าย",
    L"ระยะห่างจากตัวชี้เมาส์",
    L"สีสำหรับภาษาไทย",
    L"สีสำหรับภาษาอังกฤษ",
    L"บันทึก",
    L"ยกเลิก",
    L"ปุ่มลัดต่อไปนี้ถูก Windows หรือแอปอื่นใช้อยู่ Salap จึงใช้ไม่ได้:\n\n",
    L"แอปที่ไม่ให้ Salap ทำงาน",
    L"Salap จะไม่แปลงข้อความในแอปเหล่านี้",
    L"  (มีมาให้)",
    L"เพิ่มแอปที่เปิดอยู่…",
    L"เลือกไฟล์…",
    L"ลบ",
    L"ปิด",
    L"(ไม่มีแอปอื่นเปิดอยู่)",
    L"การแปลงปีต้องเลือกคำว่า พ.ศ. หรือ ค.ศ. พร้อมกับปีด้วย\n\nเช่น เลือก “พ.ศ. 2569” ไม่ใช่เลือกแค่ “2569”",
    L"รายการเหล่านี้ และการแปลงจำนวนเงินเป็นตัวอักษร ใช้ได้จากเมนูของไอคอน Salap ข้างนาฬิกาด้วย",
    L"กรุณาเลือกตัวเลขก่อน เช่น 1,250 หรือ ๑,๒๕๐.50",
    L"กรุณาเลือกจำนวนเงินที่เขียนเป็นตัวอักษรก่อน เช่น หนึ่งพันสองร้อยห้าสิบบาทถ้วน",
};

static_assert(ARRAYSIZE(kEn) == S_COUNT, "English strings out of step with Str");
static_assert(ARRAYSIZE(kTh) == S_COUNT, "Thai strings out of step with Str");

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

// A shortcut is either Ctrl tapped twice, or keys held together. Alt and Shift
// are not offered for tapping: Windows and apps already react to those keys
// on their own (menus, language switching, Sticky Keys).
enum { SC_DOUBLE = 1, SC_COMBO = 2 };
struct Shortcut {
    int type;
    UINT mods;  // MOD_* flags, for SC_COMBO
    UINT vk;    // the key; for SC_DOUBLE always VK_CONTROL
};

struct Color {
    BYTE r, g, b;
};
const Color kPalette[] = {
    {37, 99, 235},   // blue
    {22, 163, 74},   // green
    {220, 38, 38},   // red
    {234, 88, 12},   // orange
    {147, 51, 234},  // purple
    {219, 39, 119},  // pink
    {55, 65, 81},    // dark grey
};
const int kPaletteCount = ARRAYSIZE(kPalette);

// Terminals: Salap reads the selection with Ctrl+C, which stops a running
// program there. These cannot be removed from the list.
const wchar_t* const kBuiltInExcluded[] = {
    L"WindowsTerminal.exe", L"cmd.exe", L"powershell.exe", L"pwsh.exe", L"conhost.exe",
    L"OpenConsole.exe",     L"mintty.exe", L"putty.exe",   L"alacritty.exe", L"wezterm-gui.exe",
};

struct Settings {
    bool runAtLogin = false;
    int uiLang = 0;  // 0 = same as Windows, 1 = Thai, 2 = English
    Shortcut main = {SC_DOUBLE, 0, VK_CONTROL};
    Shortcut undo = {SC_COMBO, MOD_CONTROL | MOD_ALT, 'Z'};
    unsigned features = salap::AllFeatures;
    bool keepSelected = true;
    std::vector<std::wstring> excluded;  // added by the user
    bool badge = true;
    int badgeCorner = 0;  // salap::Corner
    int badgeGap = 2;
    int colorTh = 0;
    int colorEn = 1;
};

const wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kRunValue[] = L"Salap";

const UINT WM_TRAY = WM_APP + 1;
const UINT WM_DOUBLETAP = WM_APP + 2;   // wParam: the modifier key that was tapped twice
const UINT TM_SYNC_HOOKS = WM_APP + 10;  // to the hook thread; wParam: hooks wanted
enum { HK_MAIN = 1, HK_UNDO = 2 };

HINSTANCE g_inst;
HWND g_main;       // hidden window that owns the tray icon, hotkeys and timer
HWND g_badge;
HWND g_settingsWnd;
HWND g_excludedWnd;
HWND g_lastForeground;  // last window of another app that had the focus
Settings g_set;
std::wstring g_iniPath;
bool g_firstRun = false;
UINT g_taskbarCreated;
DWORD g_hookThreadId;

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

std::vector<std::wstring> SplitList(const std::wstring& list) {
    std::vector<std::wstring> out;
    size_t i = 0;
    while (i < list.size()) {
        size_t j = list.find_first_of(L",;\r\n", i);
        if (j == std::wstring::npos) j = list.size();
        size_t a = i, b = j;
        while (a < b && iswspace(list[a])) ++a;
        while (b > a && iswspace(list[b - 1])) --b;
        if (b > a) out.push_back(list.substr(a, b - a));
        i = j + 1;
    }
    return out;
}

bool ValidShortcut(const Shortcut& s) {
    if (s.type == SC_DOUBLE) return s.vk == VK_CONTROL;
    if (s.type == SC_COMBO) return s.vk != 0 && (s.mods & (MOD_CONTROL | MOD_ALT | MOD_WIN)) != 0;
    return false;
}

void LoadSettings() {
    wchar_t appdata[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) appdata[0] = 0;
    std::wstring dir = std::wstring(appdata) + L"\\Salap";
    CreateDirectoryW(dir.c_str(), nullptr);
    g_iniPath = dir + L"\\settings.ini";
    g_firstRun = GetFileAttributesW(g_iniPath.c_str()) == INVALID_FILE_ATTRIBUTES;

    const wchar_t* ini = g_iniPath.c_str();
    const Settings d;
    auto getInt = [&](const wchar_t* sec, const wchar_t* key, int def) {
        return (int)GetPrivateProfileIntW(sec, key, def, ini);
    };
    auto getShortcut = [&](const wchar_t* name, const Shortcut& def) {
        std::wstring n(name);
        Shortcut s;
        s.type = getInt(L"Shortcuts", (n + L"Type").c_str(), def.type);
        s.mods = (UINT)getInt(L"Shortcuts", (n + L"Mods").c_str(), (int)def.mods);
        s.vk = (UINT)getInt(L"Shortcuts", (n + L"Key").c_str(), (int)def.vk);
        return ValidShortcut(s) ? s : def;
    };
    auto inRange = [](int v, int lo, int hi, int def) { return v < lo || v > hi ? def : v; };

    g_set.uiLang = inRange(getInt(L"General", L"Language", d.uiLang), 0, 2, 0);
    g_set.main = getShortcut(L"Convert", d.main);
    g_set.undo = getShortcut(L"Undo", d.undo);
    if (g_set.undo.type != SC_COMBO) g_set.undo = d.undo;  // undo is always a key combination
    g_set.features = (unsigned)getInt(L"Convert", L"Features", (int)d.features) & salap::AllFeatures;
    g_set.keepSelected = getInt(L"Convert", L"KeepSelected", d.keepSelected) != 0;
    wchar_t buf[4096];
    GetPrivateProfileStringW(L"Convert", L"ExcludedApps", L"", buf, ARRAYSIZE(buf), ini);
    g_set.excluded = SplitList(buf);
    g_set.badge = getInt(L"Badge", L"Show", d.badge) != 0;
    g_set.badgeCorner = inRange(getInt(L"Badge", L"Corner", d.badgeCorner), 0, 3, 0);
    g_set.badgeGap = inRange(getInt(L"Badge", L"Gap", d.badgeGap + 100) - 100, -8, 30, d.badgeGap);
    g_set.colorTh = inRange(getInt(L"Badge", L"ColorThai", d.colorTh), 0, kPaletteCount - 1, d.colorTh);
    g_set.colorEn = inRange(getInt(L"Badge", L"ColorEnglish", d.colorEn), 0, kPaletteCount - 1, d.colorEn);
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
    auto putInt = [&](const wchar_t* sec, const wchar_t* key, int v) {
        WritePrivateProfileStringW(sec, key, std::to_wstring(v).c_str(), ini);
    };
    auto putShortcut = [&](const wchar_t* name, const Shortcut& s) {
        std::wstring n(name);
        putInt(L"Shortcuts", (n + L"Type").c_str(), s.type);
        putInt(L"Shortcuts", (n + L"Mods").c_str(), (int)s.mods);
        putInt(L"Shortcuts", (n + L"Key").c_str(), (int)s.vk);
    };
    putInt(L"General", L"Language", g_set.uiLang);
    putShortcut(L"Convert", g_set.main);
    putShortcut(L"Undo", g_set.undo);
    putInt(L"Convert", L"Features", (int)g_set.features);
    putInt(L"Convert", L"KeepSelected", g_set.keepSelected);
    std::wstring list;
    for (const std::wstring& e : g_set.excluded) list += (list.empty() ? L"" : L", ") + e;
    WritePrivateProfileStringW(L"Convert", L"ExcludedApps", list.c_str(), ini);
    putInt(L"Badge", L"Show", g_set.badge);
    putInt(L"Badge", L"Corner", g_set.badgeCorner);
    putInt(L"Badge", L"Gap", g_set.badgeGap + 100);  // stored offset so it is never negative
    putInt(L"Badge", L"ColorThai", g_set.colorTh);
    putInt(L"Badge", L"ColorEnglish", g_set.colorEn);
    WriteRunAtLogin(g_set.runAtLogin);
}

// ---------------------------------------------------------------------------
// Shortcuts
// ---------------------------------------------------------------------------

std::wstring KeyName(UINT vk) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::wstring(1, (wchar_t)vk);
    if (vk >= VK_F1 && vk <= VK_F24) return L"F" + std::to_wstring(vk - VK_F1 + 1);
    if (vk == VK_SPACE) return L"Space";
    if (vk == VK_OEM_3) return L"`";
    if (vk == VK_CONTROL) return L"Ctrl";
    if (vk == VK_MENU) return L"Alt";
    if (vk == VK_SHIFT) return L"Shift";
    wchar_t name[64];
    wsprintfW(name, L"0x%02X", vk);
    return name;
}

std::wstring ShortcutText(const Shortcut& s) {
    if (s.type == SC_DOUBLE) return KeyName(s.vk) + L" ×2";
    std::wstring t;
    if (s.mods & MOD_CONTROL) t += L"Ctrl+";
    if (s.mods & MOD_ALT) t += L"Alt+";
    if (s.mods & MOD_SHIFT) t += L"Shift+";
    if (s.mods & MOD_WIN) t += L"Win+";
    return t + KeyName(s.vk);
}

// Tells the hook thread whether anything needs to watch for double taps.
void SyncHooks() {
    bool want = !g_settingsWnd && g_set.main.type == SC_DOUBLE;
    PostThreadMessageW(g_hookThreadId, TM_SYNC_HOOKS, want, 0);
}

void UnregisterHotkeys() {
    UnregisterHotKey(g_main, HK_MAIN);
    UnregisterHotKey(g_main, HK_UNDO);
}

// Returns the shortcuts that could not be registered, one per line.
std::wstring RegisterHotkeys() {
    UnregisterHotkeys();
    std::wstring failed;
    auto reg = [&](int id, const Shortcut& s) {
        if (s.type != SC_COMBO) return;
        if (!RegisterHotKey(g_main, id, s.mods | MOD_NOREPEAT, s.vk)) failed += ShortcutText(s) + L"\n";
    };
    reg(HK_MAIN, g_set.main);
    reg(HK_UNDO, g_set.undo);
    SyncHooks();
    return failed;
}

// ---------------------------------------------------------------------------
// Double-tap detection
//
// Low-level hooks must answer quickly or Windows drops them, and the main
// thread sleeps while it converts text. So the hooks live on their own thread
// that does nothing but watch for "Ctrl tapped twice" and tell the main
// window. They are installed only while the double-tap shortcut is in use.
// No key is looked at beyond "it was not Ctrl", and nothing is stored.
// ---------------------------------------------------------------------------

struct TapState {
    UINT mod = 0;
    int stage = 0;  // 0 idle, 1 first press, 2 first release, 3 second press, 4 held too long
    DWORD time = 0;
} g_tap;
HHOOK g_kbHook, g_mouseHook;
const DWORD kTapHoldMs = 300;  // a tap is shorter than this
const DWORD kTapGapMs = 400;   // the second tap starts within this

UINT ModifierOf(DWORD vk) {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ? VK_CONTROL : 0;
}

LRESULT CALLBACK KeyboardHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION) {
        const KBDLLHOOKSTRUCT* k = (const KBDLLHOOKSTRUCT*)lp;
        if (!(k->flags & LLKHF_INJECTED)) {
            const bool down = wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN;
            const UINT mod = ModifierOf(k->vkCode);
            TapState& t = g_tap;
            if (!mod) {
                t.stage = 0;
            } else if (down) {
                if (t.stage == 0 || mod != t.mod) {
                    t = {mod, 1, k->time};
                } else if (t.stage == 2) {
                    if (k->time - t.time <= kTapGapMs) t = {mod, 3, k->time};
                    else t = {mod, 1, k->time};
                } else if ((t.stage == 1 || t.stage == 3) && k->time - t.time > kTapHoldMs) {
                    t.stage = 4;  // key repeat: it is being held, not tapped
                }
            } else {
                if (mod != t.mod) {
                    t.stage = 0;
                } else if (t.stage == 1 && k->time - t.time <= kTapHoldMs) {
                    t.stage = 2;
                    t.time = k->time;
                } else {
                    if (t.stage == 3 && k->time - t.time <= kTapHoldMs)
                        PostMessageW(g_main, WM_DOUBLETAP, mod, 0);
                    t.stage = 0;
                }
            }
        }
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}

// Ctrl+click, twice, is not a double tap.
LRESULT CALLBACK MouseHook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && wp != WM_MOUSEMOVE) g_tap.stage = 0;
    return CallNextHookEx(nullptr, code, wp, lp);
}

DWORD WINAPI HookThread(LPVOID ready) {
    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);  // create the message queue
    SetEvent((HANDLE)ready);
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr && msg.message == TM_SYNC_HOOKS) {
            if (msg.wParam && !g_kbHook) {
                g_tap = TapState();
                g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHook, g_inst, 0);
                g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHook, g_inst, 0);
            } else if (!msg.wParam && g_kbHook) {
                UnhookWindowsHookEx(g_kbHook);
                if (g_mouseHook) UnhookWindowsHookEx(g_mouseHook);
                g_kbHook = g_mouseHook = nullptr;
            }
        }
        DispatchMessageW(&msg);
    }
    return 0;
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
    return name;
}

bool IsBuiltInExcluded(const std::wstring& name) {
    for (const wchar_t* b : kBuiltInExcluded)
        if (!lstrcmpiW(b, name.c_str())) return true;
    return false;
}

bool InList(const std::vector<std::wstring>& list, const std::wstring& name) {
    for (const std::wstring& e : list)
        if (!lstrcmpiW(e.c_str(), name.c_str())) return true;
    return false;
}

bool IsExcluded(HWND w) {
    if (!w) return true;
    wchar_t cls[64] = L"";
    GetClassNameW(w, cls, ARRAYSIZE(cls));
    if (!lstrcmpW(cls, L"ConsoleWindowClass")) return true;
    std::wstring name = ProcessName(w);
    if (name.empty()) return false;
    return IsBuiltInExcluded(name) || InList(g_set.excluded, name);
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

enum Kind { K_CYCLE, K_FIX, K_NUM, K_YEAR, K_TO_WORDS, K_WORDS_AR, K_WORDS_TH, K_UNDO };

struct LastConversion {
    bool valid = false;
    bool cycle = false;
    std::wstring original;  // what the user had selected
    std::wstring result;    // what Salap replaced it with
    size_t index = 0;       // position in the cycle
    ULONGLONG time = 0;
    HWND window = nullptr;
};
LastConversion g_last;

void Fail() { MessageBeep(MB_ICONWARNING); }

std::wstring W(const std::u32string& s) {
    std::u16string u = salap::to_utf16(s);
    return std::wstring(u.begin(), u.end());
}

std::u32string U(const std::wstring& s) {
    return salap::from_utf16(std::u16string(s.begin(), s.end()));
}

// Replaces the selection with `text` and, if wanted, selects it again.
bool PasteOver(const std::wstring& text) {
    if (!WriteClipboardText(text)) return false;
    SendCtrl('V');
    Sleep(250);  // the app reads the clipboard after it sees the keys
    if (g_set.keepSelected) {
        size_t steps = salap::caret_steps(U(text));
        if (steps > 0 && steps <= 400) {
            SendShiftLeft(steps);
            Sleep(30);
        }
    }
    return true;
}

void RunAction(Kind kind, bool fromMenu) {
    HWND target = fromMenu ? g_lastForeground : GetForegroundWindow();
    if (!target || !IsWindow(target) || IsExcluded(target)) return;
    if (fromMenu) {
        SetForegroundWindow(target);
        Sleep(150);
        if (GetForegroundWindow() != target) return Fail();
    }
    WaitForModifiersUp();

    const ULONGLONG now = GetTickCount64();
    const bool recent = g_last.valid && g_last.window == target && now - g_last.time < 30000;

    std::vector<ClipItem> backup;
    BackupClipboard(backup);
    std::wstring sel;
    const bool copied = CopySelection(sel);
    const bool again = copied && recent && sel == g_last.result;

    if (kind == K_UNDO) {
        if (again) {
            PasteOver(g_last.original);
            RestoreClipboard(backup);
        } else {
            // The converted text is no longer selected; fall back on the app's own undo.
            if (copied) RestoreClipboard(backup);
            if (!recent) return;
            SendCtrl('Z');
        }
        g_last.valid = false;
        return;
    }

    if (!copied) return;  // nothing selected; the clipboard was not touched
    auto giveUp = [&]() {
        RestoreClipboard(backup);
        Fail();
    };
    // The menu commands say what they need when the selection is not it.
    auto explain = [&](Str hint) {
        RestoreClipboard(backup);
        MessageBoxW(nullptr, T(hint), L"Salap", MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
    };
    // A trailing line break usually means the app copied a whole line or
    // cell because nothing was selected. Leave those alone.
    if (sel.empty() || sel.size() > 5000 || sel.back() == L'\n' || sel.back() == L'\r')
        return giveUp();

    std::wstring original = sel;
    std::wstring out;
    LastConversion next;

    switch (kind) {
        case K_CYCLE: {
            const bool continuing = again && g_last.cycle;
            if (continuing) original = g_last.original;
            std::vector<std::u32string> steps = salap::conversion_cycle(U(original), g_set.features);
            if (steps.empty()) return giveUp();
            steps.push_back(U(original));  // the last press brings the original back
            size_t idx = continuing ? (g_last.index + 1) % steps.size() : 0;
            out = W(steps[idx]);
            if (out == sel) {
                idx = (idx + 1) % steps.size();
                out = W(steps[idx]);
            }
            next.cycle = true;
            next.index = idx;
            break;
        }
        case K_FIX:
            out = W(salap::convert_layout(U(sel)));
            break;
        case K_NUM:
            out = W(salap::convert_numerals(U(sel)));
            break;
        case K_YEAR: {
            int n = 0;
            out = W(salap::convert_years(U(sel), &n));
            if (!n) return explain(S_YEAR_HINT);
            break;
        }
        case K_TO_WORDS: {
            std::u32string r;
            if (!salap::baht_text(U(sel), r)) return explain(S_HINT_NUMBER);
            out = W(r);
            break;
        }
        case K_WORDS_AR:
        case K_WORDS_TH: {
            std::u32string r;
            if (!salap::parse_baht_text(U(sel), r)) return explain(S_HINT_WORDS);
            if (kind == K_WORDS_TH) r = salap::convert_numerals(r, salap::NumeralDirection::ToThai);
            out = W(r);
            break;
        }
        case K_UNDO:
            break;
    }
    if (out == sel) return giveUp();
    if (!PasteOver(out)) return giveUp();
    RestoreClipboard(backup);

    next.valid = true;
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
    std::wstring code;      // what is drawn now; empty forces a redraw
    int w = 0, h = 0;
    POINT pos = {LONG_MIN, LONG_MIN};
    bool visible = false;
    HCURSOR cursor = nullptr;      // cursor the cached shape belongs to
    salap::Rect shape = {0, 0, 12, 19};  // visible part of the cursor, relative to the pointer
    int baseSize = 32;             // Windows "mouse pointer size" setting
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

bool ReadPixels(HBITMAP bmp, int w, int h, std::vector<DWORD>& px) {
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;  // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    px.assign((size_t)w * h, 0);
    HDC dc = GetDC(nullptr);
    int got = GetDIBits(dc, bmp, 0, (UINT)h, px.data(), &bi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, dc);
    return got == h;
}

// Finds the part of the cursor image that is actually drawn, so the badge can
// sit right next to it whatever the cursor's shape or size.
void MeasureCursor(HCURSOR cur) {
    if (cur == g_b.cursor) return;
    g_b.cursor = cur;

    int w = 32, h = 32, hx = 0, hy = 0;
    int l = 0, t = 0, r = 11, b = 18;  // a plain arrow, if the image cannot be read
    ICONINFO ii;
    if (cur && GetIconInfo(cur, &ii)) {
        BITMAP bm = {};
        if (GetObjectW(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm) && bm.bmWidth > 0 &&
            bm.bmWidth <= 1024 && bm.bmHeight > 0 && bm.bmHeight <= 2048) {
            w = bm.bmWidth;
            h = ii.hbmColor ? bm.bmHeight : bm.bmHeight / 2;  // a mask holds two images
            if (h < 1) h = 1;
            hx = (int)ii.xHotspot;
            hy = (int)ii.yHotspot;

            std::vector<DWORD> color, mask;
            std::vector<bool> drawn;
            const size_t n = (size_t)w * h;
            if (ii.hbmColor && ReadPixels(ii.hbmColor, w, h, color)) {
                bool hasAlpha = false;
                for (DWORD p : color)
                    if (p >> 24) hasAlpha = true;
                if (hasAlpha) {
                    drawn.resize(n);
                    for (size_t k = 0; k < n; ++k) drawn[k] = (color[k] >> 24) > 32;
                } else if (ii.hbmMask && ReadPixels(ii.hbmMask, w, h, mask)) {
                    drawn.resize(n);
                    for (size_t k = 0; k < n; ++k) drawn[k] = (mask[k] & 0xFFFFFF) == 0;
                }
            } else if (!ii.hbmColor && ii.hbmMask && ReadPixels(ii.hbmMask, w, h * 2, mask)) {
                // Black-and-white cursor: drawn where the top half is black
                // or the bottom half is white (which inverts the screen).
                drawn.resize(n);
                for (size_t k = 0; k < n; ++k)
                    drawn[k] = (mask[k] & 0xFFFFFF) == 0 || (mask[n + k] & 0xFFFFFF) != 0;
            }
            int bl = w, bt = h, br = -1, bb = -1;
            for (size_t k = 0; k < drawn.size(); ++k) {
                if (!drawn[k]) continue;
                int x = (int)(k % w), y = (int)(k / w);
                if (x < bl) bl = x;
                if (x > br) br = x;
                if (y < bt) bt = y;
                if (y > bb) bb = y;
            }
            if (br >= bl && bb >= bt) {
                l = bl; t = bt; r = br; b = bb;
            } else {
                l = 0; t = 0; r = w - 1; b = h - 1;
            }
        }
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
    }
    // The enlarged pointer from Settings > Accessibility is drawn bigger
    // than the cursor's own image; scale up to that size if it is larger.
    int shown = max(w, MulDiv(g_b.baseSize, g_dpi, 96));
    g_b.shape = {MulDiv(l - hx, shown, w), MulDiv(t - hy, shown, w), MulDiv(r + 1 - hx, shown, w),
                 MulDiv(b + 1 - hy, shown, w)};
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
    Color c = PRIMARYLANGID(lang) == LANG_THAI      ? kPalette[g_set.colorTh]
              : PRIMARYLANGID(lang) == LANG_ENGLISH ? kPalette[g_set.colorEn]
                                                    : Color{75, 85, 99};
    Gdiplus::Bitmap* bmp = DrawTile(w, h, Gdiplus::Color(235, c.r, c.g, c.b), code.c_str(), (float)Dp(12), L"Segoe UI");
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
    in.cursor = g_b.shape;
    in.badge_w = g_b.w;
    in.badge_h = g_b.h;
    in.corner = (salap::Corner)g_set.badgeCorner;
    in.gap = Dp(g_set.badgeGap);
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

enum MenuId { M_CYCLE = 1, M_FIX, M_NUM, M_YEAR, M_TO_WORDS, M_WORDS_AR, M_WORDS_TH, M_UNDO, M_SETTINGS, M_QUIT };

HICON g_trayIcon, g_bigIcon;

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
    auto item = [&](UINT id, Str label, const Shortcut* sc, bool enabled) {
        std::wstring s = T(label);
        if (sc) s += L"\t" + ShortcutText(*sc);
        AppendMenuW(m, MF_STRING | (enabled ? 0 : MF_GRAYED), id, s.c_str());
    };
    item(M_CYCLE, S_CONVERT, &g_set.main, g_set.features != 0);
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    item(M_FIX, S_FIX, nullptr, true);
    item(M_NUM, S_NUM, nullptr, true);
    item(M_YEAR, S_YEAR, nullptr, true);
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    item(M_TO_WORDS, S_TO_WORDS, nullptr, true);
    item(M_WORDS_AR, S_WORDS_AR, nullptr, true);
    item(M_WORDS_TH, S_WORDS_TH, nullptr, true);
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    item(M_UNDO, S_UNDO, &g_set.undo, g_last.valid);
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    item(M_SETTINGS, S_SETTINGS, nullptr, true);
    item(M_QUIT, S_QUIT, nullptr, true);

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_main);  // so the menu closes when the user clicks away
    UINT cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, g_main, nullptr);
    DestroyMenu(m);
    PostMessageW(g_main, WM_NULL, 0, 0);

    switch (cmd) {
        case M_CYCLE: RunAction(K_CYCLE, true); break;
        case M_FIX: RunAction(K_FIX, true); break;
        case M_NUM: RunAction(K_NUM, true); break;
        case M_YEAR: RunAction(K_YEAR, true); break;
        case M_TO_WORDS: RunAction(K_TO_WORDS, true); break;
        case M_WORDS_AR: RunAction(K_WORDS_AR, true); break;
        case M_WORDS_TH: RunAction(K_WORDS_TH, true); break;
        case M_UNDO: RunAction(K_UNDO, true); break;
        case M_SETTINGS: OpenSettings(); break;
        case M_QUIT: DestroyWindow(g_main); break;
    }
}

// ---------------------------------------------------------------------------
// Settings window
// ---------------------------------------------------------------------------

enum CtrlId {
    C_RUN = 100, C_LANG,
    C_MAIN,       // +0 tap radio, +2 combination radio, +3 modifier, +4 modifier, +5 key
    C_UNDO = C_MAIN + 6,
    C_F_FIX = C_UNDO + 6, C_F_NUM, C_F_YEAR, C_KEEP, C_EXCLUDED,
    C_BADGE, C_POS, C_GAP, C_SAVE, C_CANCEL,
    C_SW_TH = 200,  // + palette index
    C_SW_EN = 220,
    C_EX_LIST = 300, C_EX_ADD, C_EX_BROWSE, C_EX_REMOVE, C_EX_CLOSE
};

HFONT g_font, g_fontBold;
Settings g_before;                         // to put back if Settings is cancelled
std::vector<std::wstring> g_pendingExcluded;  // edited by the excluded-apps window

struct NamedValue {
    const wchar_t* name;
    UINT value;
};
const NamedValue kMod1[] = {{L"Ctrl", MOD_CONTROL}, {L"Alt", MOD_ALT}, {L"Win", MOD_WIN}};
// Always two modifiers: Ctrl+S or Alt+F alone would take over shortcuts every app uses.
const NamedValue kMod2[] = {{L"Shift", MOD_SHIFT}, {L"Alt", MOD_ALT}, {L"Ctrl", MOD_CONTROL}};

std::vector<UINT> ComboKeys() {
    std::vector<UINT> v;
    for (UINT k = 'A'; k <= 'Z'; ++k) v.push_back(k);
    for (UINT k = '0'; k <= '9'; ++k) v.push_back(k);
    for (UINT k = VK_F1; k <= VK_F12; ++k) v.push_back(k);
    v.push_back(VK_SPACE);
    v.push_back(VK_OEM_3);
    return v;
}

HWND Ctl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, DWORD exStyle, int x,
         int y, int w, int h, int id, HFONT font = nullptr) {
    HWND c = CreateWindowExW(exStyle, cls, text, WS_CHILD | WS_VISIBLE | style, Dp(x), Dp(y), Dp(w),
                             Dp(h), parent, (HMENU)(INT_PTR)id, g_inst, nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)(font ? font : g_font), TRUE);
    return c;
}

template <size_t N>
HWND Combo(HWND w, int x, int y, int width, int id, const NamedValue (&items)[N], UINT selected) {
    HWND c = Ctl(w, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, x, y, width, 300, id);
    int sel = 0;
    for (size_t i = 0; i < N; ++i) {
        SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)items[i].name);
        if (items[i].value == selected) sel = (int)i;
    }
    SendMessageW(c, CB_SETCURSEL, sel, 0);
    return c;
}

// The modifier, modifier and key boxes of a key combination, at ids base+3..5.
void BuildCombo(HWND w, int x, int y, int base, const Shortcut& sc) {
    const bool combo = sc.type == SC_COMBO;
    UINT m1 = MOD_CONTROL, m2 = MOD_ALT;
    if (combo) {
        // Split the modifier set over the two boxes.
        m1 = (sc.mods & MOD_CONTROL) ? MOD_CONTROL : (sc.mods & MOD_ALT) ? MOD_ALT : MOD_WIN;
        UINT rest = sc.mods & ~m1;
        m2 = (rest & MOD_SHIFT) ? MOD_SHIFT : (rest & MOD_ALT) ? MOD_ALT : MOD_CONTROL;
    }
    Combo(w, x, y, 76, base + 3, kMod1, m1);
    Ctl(w, L"STATIC", L"+", SS_CENTER, 0, x + 78, y + 3, 14, 20, -1);
    Combo(w, x + 94, y, 76, base + 4, kMod2, m2);
    Ctl(w, L"STATIC", L"+", SS_CENTER, 0, x + 172, y + 3, 14, 20, -1);
    HWND key = Ctl(w, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, x + 188, y, 62, 300, base + 5);
    std::vector<UINT> keys = ComboKeys();
    int sel = 18;  // S
    for (size_t i = 0; i < keys.size(); ++i) {
        SendMessageW(key, CB_ADDSTRING, 0, (LPARAM)KeyName(keys[i]).c_str());
        if (combo && keys[i] == sc.vk) sel = (int)i;
    }
    SendMessageW(key, CB_SETCURSEL, sel, 0);
}

// The convert shortcut: (•) Tap Ctrl twice   ( ) Keys together: [Ctrl] + [Alt] + [S]
void BuildMainShortcut(HWND w, int x, int& y) {
    Ctl(w, L"STATIC", T(S_SC_MAIN), 0, 0, x, y, 380, 20, -1);
    y += 24;
    HWND r1 = Ctl(w, L"BUTTON", T(S_TAP_TWICE), BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 0, x + 14, y, 380, 24, C_MAIN);
    y += 28;
    HWND r2 = Ctl(w, L"BUTTON", T(S_COMBO), BS_AUTORADIOBUTTON, 0, x + 14, y, 140, 24, C_MAIN + 2);
    BuildCombo(w, x + 160, y, C_MAIN, g_set.main);
    SendMessageW(g_set.main.type == SC_COMBO ? r2 : r1, BM_SETCHECK, BST_CHECKED, 0);
    y += 36;
}

// The undo shortcut is always a key combination.
void BuildUndoShortcut(HWND w, int x, int& y) {
    Ctl(w, L"STATIC", T(S_SC_UNDO), WS_GROUP, 0, x, y + 3, 156, 20, -1);
    BuildCombo(w, x + 160, y, C_UNDO, g_set.undo);
    y += 40;
}

Shortcut ReadShortcut(HWND w, int base, bool canTap) {
    auto sel = [&](int id) { return (int)SendDlgItemMessageW(w, id, CB_GETCURSEL, 0, 0); };
    Shortcut s = {};
    if (canTap && IsDlgButtonChecked(w, base) == BST_CHECKED) {
        s.type = SC_DOUBLE;
        s.vk = VK_CONTROL;
    } else {
        s.type = SC_COMBO;
        int a = sel(base + 3), b = sel(base + 4), k = sel(base + 5);
        UINT m1 = kMod1[a < 0 || a >= (int)ARRAYSIZE(kMod1) ? 0 : a].value;
        UINT m2 = kMod2[b < 0 || b >= (int)ARRAYSIZE(kMod2) ? 0 : b].value;
        if (m2 == m1) m2 = m1 == MOD_ALT ? MOD_SHIFT : MOD_ALT;  // the same key twice is not a combination
        s.mods = m1 | m2;
        std::vector<UINT> keys = ComboKeys();
        s.vk = keys[k < 0 || k >= (int)keys.size() ? 0 : k];
    }
    return s;
}

void BuildSettings(HWND w) {
    const int L = 20, LW = 420;        // left column
    const int R = L + LW + 30, RW = 300;  // right column
    int y = 16;
    auto header = [&](int x, Str label) {
        Ctl(w, L"STATIC", T(label), 0, 0, x, y, 300, 20, -1, g_fontBold);
        y += 26;
    };
    auto check = [&](int x, int width, int id, Str label, bool on, DWORD extra = 0) {
        HWND c = Ctl(w, L"BUTTON", T(label), BS_AUTOCHECKBOX | WS_TABSTOP | extra, 0, x, y, width, 22, id);
        SendMessageW(c, BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
        y += 26;
    };

    // ---- Left column: converting text
    header(L, S_SEC_CONVERT);
    BuildMainShortcut(w, L, y);
    BuildUndoShortcut(w, L, y);
    Ctl(w, L"STATIC", T(S_FEATURES), 0, 0, L, y, LW, 20, -1);
    y += 24;
    check(L + 14, LW - 14, C_F_FIX, S_F_FIX, (g_set.features & salap::FixLanguage) != 0, WS_GROUP);
    check(L + 14, LW - 14, C_F_NUM, S_F_NUM, (g_set.features & salap::Numerals) != 0);
    check(L + 14, LW - 14, C_F_YEAR, S_F_YEAR, (g_set.features & salap::Years) != 0);
    Ctl(w, L"STATIC", T(S_FEATURES_NOTE), 0, 0, L + 14, y + 2, LW - 14, 38, -1);
    y += 50;
    check(L, LW, C_KEEP, S_KEEP_SEL, g_set.keepSelected);
    y += 4;
    Ctl(w, L"BUTTON", T(S_EXCLUDED_BTN), WS_TABSTOP, 0, L, y, 230, 28, C_EXCLUDED);
    int leftBottom = y + 28;

    // ---- Right column: badge, then general
    y = 16;
    header(R, S_SEC_BADGE);
    check(R, RW, C_BADGE, S_BADGE, g_set.badge);
    y += 4;
    Ctl(w, L"STATIC", T(S_POSITION), 0, 0, R, y + 3, 120, 20, -1);
    HWND pos = Ctl(w, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0, R + 130, y, 170, 200, C_POS);
    for (Str sId : {S_POS_BR, S_POS_BL, S_POS_TR, S_POS_TL}) SendMessageW(pos, CB_ADDSTRING, 0, (LPARAM)T(sId));
    SendMessageW(pos, CB_SETCURSEL, g_set.badgeCorner, 0);
    y += 34;
    Ctl(w, L"STATIC", T(S_DISTANCE), 0, 0, R, y, RW, 20, -1);
    y += 20;
    HWND gap = Ctl(w, TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_AUTOTICKS | WS_TABSTOP, 0, R - 6, y, RW + 12, 30, C_GAP);
    SendMessageW(gap, TBM_SETRANGEMIN, FALSE, -8);
    SendMessageW(gap, TBM_SETRANGEMAX, FALSE, 30);
    SendMessageW(gap, TBM_SETTICFREQ, 2, 0);
    SendMessageW(gap, TBM_SETPOS, TRUE, g_set.badgeGap);
    y += 40;
    auto swatches = [&](Str label, int base) {
        Ctl(w, L"STATIC", T(label), 0, 0, R, y, RW, 20, -1);
        y += 22;
        for (int i = 0; i < kPaletteCount; ++i)
            Ctl(w, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, R + i * 34, y, 28, 28, base + i);
        y += 40;
    };
    swatches(S_COLOR_TH, C_SW_TH);
    swatches(S_COLOR_EN, C_SW_EN);

    y += 6;
    header(R, S_SEC_GENERAL);
    check(R, RW, C_RUN, S_RUN_LOGIN, g_set.runAtLogin);
    y += 4;
    Ctl(w, L"STATIC", T(S_LANGUAGE), 0, 0, R, y + 3, 120, 20, -1);
    HWND combo = Ctl(w, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0, R + 130, y, 170, 200, C_LANG);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)T(S_LANG_AUTO));
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"ไทย");
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(combo, CB_SETCURSEL, g_set.uiLang, 0);
    y += 30;

    y = max(y, leftBottom) + 20;
    const int total = R + RW;
    Ctl(w, L"BUTTON", T(S_SAVE), BS_DEFPUSHBUTTON | WS_TABSTOP, 0, total - 200, y, 95, 28, C_SAVE);
    Ctl(w, L"BUTTON", T(S_CANCEL), WS_TABSTOP, 0, total - 95, y, 95, 28, C_CANCEL);
    y += 28 + 18;

    RECT r = {0, 0, Dp(total + L), Dp(y)};
    AdjustWindowRectEx(&r, (DWORD)GetWindowLongPtrW(w, GWL_STYLE), FALSE, (DWORD)GetWindowLongPtrW(w, GWL_EXSTYLE));
    int ww = r.right - r.left, wh = r.bottom - r.top;
    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    SetWindowPos(w, nullptr, work.left + (work.right - work.left - ww) / 2,
                 work.top + max(0, (int)(work.bottom - work.top - wh) / 2), ww, wh, SWP_NOZORDER);
}

void SaveFromSettings(HWND w) {
    auto checked = [&](int id) { return IsDlgButtonChecked(w, id) == BST_CHECKED; };
    g_set.runAtLogin = checked(C_RUN);
    int lang = (int)SendDlgItemMessageW(w, C_LANG, CB_GETCURSEL, 0, 0);
    g_set.uiLang = lang < 0 ? 0 : lang;
    g_set.main = ReadShortcut(w, C_MAIN, true);
    g_set.undo = ReadShortcut(w, C_UNDO, false);
    unsigned features = 0;
    if (checked(C_F_FIX)) features |= salap::FixLanguage;
    if (checked(C_F_NUM)) features |= salap::Numerals;
    if (checked(C_F_YEAR)) features |= salap::Years;
    g_set.features = features;
    g_set.keepSelected = checked(C_KEEP);
    g_set.excluded = g_pendingExcluded;
    // The badge settings were applied as they were changed.
    SaveSettings();
}

void DrawSwatch(const DRAWITEMSTRUCT* d) {
    const bool thai = d->CtlID < (UINT)C_SW_EN;
    const int index = (int)d->CtlID - (thai ? C_SW_TH : C_SW_EN);
    if (index < 0 || index >= kPaletteCount) return;
    const bool chosen = index == (thai ? g_set.colorTh : g_set.colorEn);
    const Color c = kPalette[index];
    RECT r = d->rcItem;
    FillRect(d->hDC, &r, GetSysColorBrush(COLOR_WINDOW));
    HBRUSH fill = CreateSolidBrush(RGB(c.r, c.g, c.b));
    HPEN pen = CreatePen(PS_SOLID, chosen ? Dp(3) : 1, chosen ? GetSysColor(COLOR_WINDOWTEXT) : RGB(c.r, c.g, c.b));
    HGDIOBJ oldB = SelectObject(d->hDC, fill), oldP = SelectObject(d->hDC, pen);
    int inset = chosen ? Dp(2) : Dp(4);
    RoundRect(d->hDC, r.left + inset, r.top + inset, r.right - inset, r.bottom - inset, Dp(8), Dp(8));
    SelectObject(d->hDC, oldB);
    SelectObject(d->hDC, oldP);
    DeleteObject(fill);
    DeleteObject(pen);
    if (d->itemState & ODS_FOCUS) DrawFocusRect(d->hDC, &r);
}

void OpenExcluded(HWND owner);

LRESULT CALLBACK SettingsProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            if (id == C_SAVE || id == IDOK) {
                SaveFromSettings(w);
                DestroyWindow(w);
            } else if (id == C_CANCEL || id == IDCANCEL) {
                g_set = g_before;
                DestroyWindow(w);
            } else if (id == C_EXCLUDED) {
                OpenExcluded(w);
            } else if (id == C_BADGE) {
                g_set.badge = IsDlgButtonChecked(w, C_BADGE) == BST_CHECKED;
            } else if (id == C_POS && HIWORD(wp) == CBN_SELCHANGE) {
                int i = (int)SendDlgItemMessageW(w, C_POS, CB_GETCURSEL, 0, 0);
                if (i >= 0 && i <= 3) g_set.badgeCorner = i;
            } else if (id >= C_SW_TH && id < C_SW_TH + kPaletteCount) {
                g_set.colorTh = id - C_SW_TH;
            } else if (id >= C_SW_EN && id < C_SW_EN + kPaletteCount) {
                g_set.colorEn = id - C_SW_EN;
            } else {
                return 0;
            }
            if (id >= C_SW_TH) {
                for (int i = 0; i < kPaletteCount; ++i) {
                    InvalidateRect(GetDlgItem(w, C_SW_TH + i), nullptr, TRUE);
                    InvalidateRect(GetDlgItem(w, C_SW_EN + i), nullptr, TRUE);
                }
                g_b.code.clear();  // repaint the badge in the new colour
            }
            return 0;
        }
        case WM_HSCROLL:
            if ((HWND)lp == GetDlgItem(w, C_GAP)) g_set.badgeGap = (int)SendMessageW((HWND)lp, TBM_GETPOS, 0, 0);
            return 0;
        case WM_DRAWITEM:
            DrawSwatch((const DRAWITEMSTRUCT*)lp);
            return TRUE;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        case WM_CLOSE:
            g_set = g_before;
            DestroyWindow(w);
            return 0;
        case WM_DESTROY: {
            g_settingsWnd = nullptr;
            g_b.code.clear();
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
        SetForegroundWindow(g_excludedWnd ? g_excludedWnd : g_settingsWnd);
        return;
    }
    g_before = g_set;
    g_pendingExcluded = g_set.excluded;
    g_settingsWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"SalapSettings", T(S_SETTINGS_TITLE),
                                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, 0, 0, 100,
                                    100, nullptr, nullptr, g_inst, nullptr);
    if (!g_settingsWnd) return;
    // No shortcuts while the window is open, so that choosing one cannot set it off.
    UnregisterHotkeys();
    SyncHooks();
    BuildSettings(g_settingsWnd);
    SendMessageW(g_settingsWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_trayIcon);
    if (g_bigIcon) SendMessageW(g_settingsWnd, WM_SETICON, ICON_BIG, (LPARAM)g_bigIcon);
    ShowWindow(g_settingsWnd, SW_SHOW);
    SetForegroundWindow(g_settingsWnd);
}

// ---------------------------------------------------------------------------
// "Apps where Salap is off" window
// ---------------------------------------------------------------------------

void FillExcludedList(HWND w) {
    HWND list = GetDlgItem(w, C_EX_LIST);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const wchar_t* b : kBuiltInExcluded)
        SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)(std::wstring(b) + T(S_EX_BUILTIN)).c_str());
    for (const std::wstring& e : g_pendingExcluded) SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)e.c_str());
    EnableWindow(GetDlgItem(w, C_EX_REMOVE), FALSE);
}

void AddExcluded(HWND w, const std::wstring& name) {
    if (name.empty() || IsBuiltInExcluded(name) || InList(g_pendingExcluded, name)) return;
    g_pendingExcluded.push_back(name);
    FillExcludedList(w);
    SendDlgItemMessageW(w, C_EX_LIST, LB_SETCURSEL, ARRAYSIZE(kBuiltInExcluded) + g_pendingExcluded.size() - 1, 0);
    EnableWindow(GetDlgItem(w, C_EX_REMOVE), TRUE);
}

struct RunningApp {
    std::wstring exe, title;
};

BOOL CALLBACK CollectApps(HWND w, LPARAM lp) {
    auto* apps = (std::vector<RunningApp>*)lp;
    if (!IsWindowVisible(w) || GetWindow(w, GW_OWNER)) return TRUE;
    if (GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
    wchar_t title[80];
    if (!GetWindowTextW(w, title, ARRAYSIZE(title))) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid == GetCurrentProcessId()) return TRUE;
    std::wstring exe = ProcessName(w);
    if (exe.empty() || IsBuiltInExcluded(exe) || InList(g_pendingExcluded, exe)) return TRUE;
    for (const RunningApp& a : *apps)
        if (!lstrcmpiW(a.exe.c_str(), exe.c_str())) return TRUE;
    std::wstring t(title);
    if (t.size() > 40) t = t.substr(0, 40) + L"…";
    apps->push_back({exe, t});
    return TRUE;
}

void AddRunningApp(HWND w) {
    std::vector<RunningApp> apps;
    EnumWindows(CollectApps, (LPARAM)&apps);
    HMENU m = CreatePopupMenu();
    if (apps.empty()) AppendMenuW(m, MF_STRING | MF_GRAYED, 0, T(S_EX_NONE_RUNNING));
    for (size_t i = 0; i < apps.size() && i < 40; ++i)
        AppendMenuW(m, MF_STRING, i + 1, (apps[i].exe + L"   —   " + apps[i].title).c_str());
    RECT r;
    GetWindowRect(GetDlgItem(w, C_EX_ADD), &r);
    UINT cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_NONOTIFY, r.left, r.bottom, 0, w, nullptr);
    DestroyMenu(m);
    if (cmd >= 1 && cmd <= apps.size()) AddExcluded(w, apps[cmd - 1].exe);
}

void BrowseForApp(HWND w) {
    wchar_t file[MAX_PATH * 2] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = w;
    ofn.lpstrFilter = L"Programs (*.exe)\0*.exe\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = ARRAYSIZE(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return;
    std::wstring name(file);
    size_t slash = name.find_last_of(L"\\/");
    if (slash != std::wstring::npos) name = name.substr(slash + 1);
    AddExcluded(w, name);
}

LRESULT CALLBACK ExcludedProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            const int builtIn = (int)ARRAYSIZE(kBuiltInExcluded);
            int sel = (int)SendDlgItemMessageW(w, C_EX_LIST, LB_GETCURSEL, 0, 0);
            if (id == C_EX_LIST && HIWORD(wp) == LBN_SELCHANGE) {
                EnableWindow(GetDlgItem(w, C_EX_REMOVE), sel >= builtIn);
            } else if (id == C_EX_ADD) {
                AddRunningApp(w);
            } else if (id == C_EX_BROWSE) {
                BrowseForApp(w);
            } else if (id == C_EX_REMOVE && sel >= builtIn && sel - builtIn < (int)g_pendingExcluded.size()) {
                g_pendingExcluded.erase(g_pendingExcluded.begin() + (sel - builtIn));
                FillExcludedList(w);
            } else if (id == C_EX_CLOSE || id == IDOK || id == IDCANCEL) {
                DestroyWindow(w);
            }
            return 0;
        }
        case WM_CTLCOLORSTATIC:
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        case WM_CLOSE:
            DestroyWindow(w);
            return 0;
        case WM_DESTROY:
            g_excludedWnd = nullptr;
            if (g_settingsWnd) {
                EnableWindow(g_settingsWnd, TRUE);
                SetForegroundWindow(g_settingsWnd);
            }
            return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}

void OpenExcluded(HWND owner) {
    if (g_excludedWnd) return;
    HWND w = CreateWindowExW(WS_EX_DLGMODALFRAME, L"SalapExcluded", T(S_EX_TITLE),
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, 0, 0, 100, 100,
                             owner, nullptr, g_inst, nullptr);
    if (!w) return;
    g_excludedWnd = w;
    const int L = 20, W_ = 400;
    int y = 16;
    Ctl(w, L"STATIC", T(S_EX_INFO), 0, 0, L, y, W_, 20, -1);
    y += 28;
    Ctl(w, L"LISTBOX", L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE, L, y, W_, 260, C_EX_LIST);
    y += 272;
    Ctl(w, L"BUTTON", T(S_EX_ADD_RUNNING), WS_TABSTOP, 0, L, y, 170, 28, C_EX_ADD);
    Ctl(w, L"BUTTON", T(S_EX_BROWSE), WS_TABSTOP, 0, L + 178, y, 100, 28, C_EX_BROWSE);
    Ctl(w, L"BUTTON", T(S_EX_REMOVE), WS_TABSTOP, 0, L + W_ - 100, y, 100, 28, C_EX_REMOVE);
    y += 44;
    Ctl(w, L"BUTTON", T(S_EX_CLOSE), BS_DEFPUSHBUTTON | WS_TABSTOP, 0, L + W_ - 100, y, 100, 28, C_EX_CLOSE);
    y += 28 + 18;
    FillExcludedList(w);

    RECT r = {0, 0, Dp(W_ + 2 * L), Dp(y)};
    AdjustWindowRectEx(&r, (DWORD)GetWindowLongPtrW(w, GWL_STYLE), FALSE, (DWORD)GetWindowLongPtrW(w, GWL_EXSTYLE));
    RECT o;
    GetWindowRect(owner, &o);
    SetWindowPos(w, nullptr, o.left + Dp(40), o.top + Dp(40), r.right - r.left, r.bottom - r.top, SWP_NOZORDER);
    EnableWindow(owner, FALSE);
    ShowWindow(w, SW_SHOW);
}

// ---------------------------------------------------------------------------
// Main window
// ---------------------------------------------------------------------------

LRESULT CALLBACK MainProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_HOTKEY:
            if (wp == HK_MAIN) RunAction(K_CYCLE, false);
            else if (wp == HK_UNDO) RunAction(K_UNDO, false);
            return 0;
        case WM_DOUBLETAP:
            if (g_settingsWnd) return 0;
            if (g_set.main.type == SC_DOUBLE && g_set.main.vk == wp) RunAction(K_CYCLE, false);
            return 0;
        case WM_TIMER:
            UpdateBadge();
            return 0;
        case WM_TRAY:
            if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_CONTEXTMENU || LOWORD(lp) == WM_LBUTTONUP) ShowMenu();
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

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);
    Gdiplus::GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken = 0;
    Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, nullptr);

    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    g_font = CreateFontIndirectW(&ncm.lfMessageFont);
    ncm.lfMessageFont.lfWeight = FW_BOLD;
    g_fontBold = CreateFontIndirectW(&ncm.lfMessageFont);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.hInstance = inst;
    wc.lpfnWndProc = MainProc;
    wc.lpszClassName = L"SalapMain";
    RegisterClassExW(&wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.lpszClassName = L"SalapBadge";
    RegisterClassExW(&wc);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpfnWndProc = SettingsProc;
    wc.lpszClassName = L"SalapSettings";
    RegisterClassExW(&wc);
    wc.lpfnWndProc = ExcludedProc;
    wc.lpszClassName = L"SalapExcluded";
    RegisterClassExW(&wc);

    g_main = CreateWindowExW(0, L"SalapMain", L"Salap", 0, 0, 0, 0, 0, nullptr, nullptr, inst, nullptr);
    g_badge = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                              L"SalapBadge", L"", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, inst, nullptr);
    if (!g_main || !g_badge) return 1;

    HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE hookThread = CreateThread(nullptr, 0, HookThread, ready, 0, &g_hookThreadId);
    if (ready) {
        WaitForSingleObject(ready, 2000);
        CloseHandle(ready);
    }

    LoadSettings();
    ReadPointerSize();
    g_trayIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), 0);
    g_bigIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                                  GetSystemMetrics(SM_CYICON), 0);
    if (!g_trayIcon) g_trayIcon = MakeTrayIcon();  // built without the icon resource
    g_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    AddTrayIcon();

    g_lastForeground = GetForegroundWindow();
    HWINEVENTHOOK hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
                                         ForegroundChanged, 0, 0, WINEVENT_OUTOFCONTEXT);
    SetTimer(g_main, 1, 16, nullptr);

    if (g_firstRun) {
        SaveSettings();
        OpenSettings();  // registers the shortcuts when it closes
    } else {
        std::wstring failed = RegisterHotkeys();
        if (!failed.empty())
            MessageBoxW(nullptr, (std::wstring(T(S_HK_FAIL)) + failed).c_str(), L"Salap",
                        MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (g_excludedWnd && IsDialogMessageW(g_excludedWnd, &msg)) continue;
        if (g_settingsWnd && IsDialogMessageW(g_settingsWnd, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hook) UnhookWinEvent(hook);
    UnregisterHotkeys();
    if (hookThread) {
        PostThreadMessageW(g_hookThreadId, WM_QUIT, 0, 0);
        WaitForSingleObject(hookThread, 1000);
        CloseHandle(hookThread);
    }
    if (g_trayIcon) DestroyIcon(g_trayIcon);
    if (g_font) DeleteObject(g_font);
    if (g_fontBold) DeleteObject(g_fontBold);
    Gdiplus::GdiplusShutdown(gdiToken);
    if (mutex) CloseHandle(mutex);
    return 0;
}
