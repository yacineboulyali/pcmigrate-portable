// =====================================================================
//  PC Migrate Portable  -  a small PCmover-like migration tool
//  Pure Win32 / C++ , statically linked : no .NET, no runtime, no install.
//  Runs on normal Windows 10/11 and inside WinPE (Hiren's BootCD PE).
//
//  Backup  (old PC, can run offline from WinPE):
//     - user files (Desktop, Documents, Pictures, ...)
//     - browser profiles (Chrome, Edge, Brave, Opera, Firefox)
//     - Windows settings (mouse, keyboard, regional, theme, wallpaper, Wi-Fi)
//     - list of installed programs
//  Restore (new PC):
//     - copies everything back into the chosen profile
//     - re-installs programs with winget
// =====================================================================
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <knownfolders.h>
#include <cstdarg>
#include <cwchar>
#include <cwctype>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

using std::wstring;
using std::vector;

static const wchar_t* APP_TITLE = L"PC Migrate Portable 1.1";

// ---------------------------------------------------------------------
//  Control IDs / messages
// ---------------------------------------------------------------------
enum {
    IDC_TAB = 100, IDC_LOG, IDC_PROGRESS, IDC_STATUS, IDC_CANCEL,
    IDC_B_SRC = 200, IDC_B_REFRESH, IDC_B_USERS, IDC_B_FILES, IDC_B_BROWSERS,
    IDC_B_SETTINGS, IDC_B_PROGRAMS, IDC_B_DEST, IDC_B_BROWSE, IDC_B_START, IDC_B_GROUP,
    IDC_R_PKG = 300, IDC_R_BROWSE, IDC_R_LOAD, IDC_R_SRCUSER, IDC_R_TGT, IDC_R_FILES,
    IDC_R_BROWSERS, IDC_R_SETTINGS, IDC_R_START, IDC_R_PROGS, IDC_R_CHECKALL,
    IDC_R_UNCHECK, IDC_R_INSTALL, IDC_R_PREFWINGET,
    IDC_B_APPS = 400, IDC_B_CHOOSE
};
#define WM_APP_LOG      (WM_APP + 1)
#define WM_APP_PROGRESS (WM_APP + 2)
#define WM_APP_DONE     (WM_APP + 3)

// ---------------------------------------------------------------------
//  Globals
// ---------------------------------------------------------------------
static HWND g_hwnd, g_tab, g_log, g_progress, g_status, g_cancelBtn;
static HWND hBSrc, hBRefresh, hBUsers, hBFiles, hBBrowsers, hBSettings, hBPrograms, hBDest, hBBrowse, hBStart, hBGroup;
static HWND hRPkg, hRBrowse, hRLoad, hRSrcUser, hRTgt, hRFiles, hRBrowsers, hRSettings, hRStart, hRProgs, hRCheckAll, hRUncheck, hRInstall, hRPrefWinget;
static HWND hBApps, hBChoose;
static vector<HWND> g_pageB, g_pageR;
static HFONT g_font, g_fontBold;
static double g_scale = 1.0;
static volatile LONG g_busy = 0;
static BOOL g_cancel = FALSE;
static bool g_isPE = false;
static wstring g_sysDrive;           // e.g. "C:" (or "X:" inside WinPE)
static CRITICAL_SECTION g_logCs;
static HANDLE g_logFile = INVALID_HANDLE_VALUE;

struct WinInstall { wstring drive; wstring label; bool live; };
static vector<WinInstall> g_installs;
static vector<wstring> g_tgtProfiles;

struct ProgItem { wstring name, version, publisher, id, pkgDir; };
static vector<ProgItem> g_progs;

// a program that can be transferred with its files + registry + data
struct AppInfo {
    wstring name, version, publisher, scope;   // scope: machine / machine-x86 / user
    wstring uninstallSub;                     // uninstall key, relative to SOFTWARE (machine) or user hive (user)
    wstring owner;                            // profile path (user-scope programs)
    wstring installDir;                       // logical path as seen by the old Windows (C:\...)
    wstring srcDir;                           // real path on the source disk now
    wstring note, pkgName;
    DWORD sizeKB = 0; bool selected = false;
};
static vector<AppInfo> g_srcApps;
static wstring g_srcAppsKey;

// ---------------------------------------------------------------------
//  Static data
// ---------------------------------------------------------------------
struct FolderDef { const wchar_t* name; const KNOWNFOLDERID* id; };
static const FolderDef kFolders[] = {
    {L"Desktop", &FOLDERID_Desktop},     {L"Documents", &FOLDERID_Documents},
    {L"Downloads", &FOLDERID_Downloads}, {L"Pictures", &FOLDERID_Pictures},
    {L"Music", &FOLDERID_Music},         {L"Videos", &FOLDERID_Videos},
    {L"Favorites", &FOLDERID_Favorites}, {L"Contacts", &FOLDERID_Contacts},
    {L"Links", &FOLDERID_Links},         {L"Saved Games", &FOLDERID_SavedGames},
};
// folders that OneDrive may redirect
static const wchar_t* kOneDriveFolders[] = { L"Desktop", L"Documents", L"Pictures" };

struct BrowserDef { const wchar_t* name; const wchar_t* rel; const wchar_t* exe; };
static const BrowserDef kBrowsers[] = {
    {L"Chrome",  L"AppData\\Local\\Google\\Chrome\\User Data",               L"chrome.exe"},
    {L"Edge",    L"AppData\\Local\\Microsoft\\Edge\\User Data",              L"msedge.exe"},
    {L"Brave",   L"AppData\\Local\\BraveSoftware\\Brave-Browser\\User Data", L"brave.exe"},
    {L"Opera",   L"AppData\\Roaming\\Opera Software\\Opera Stable",          L"opera.exe"},
    {L"OperaGX", L"AppData\\Roaming\\Opera Software\\Opera GX Stable",       L"opera.exe"},
    {L"Firefox", L"AppData\\Roaming\\Mozilla\\Firefox",                      L"firefox.exe"},
};
// cache folders skipped inside browser profiles (saves GBs, rebuilt automatically)
static const wchar_t* kCacheDirs[] = {
    L"cache", L"code cache", L"gpucache", L"shadercache", L"grshadercache", L"graphitedawncache",
    L"dawncache", L"dawngraphitecache", L"dawnwebgpucache", L"cachestorage", L"scriptcache",
    L"crashpad", L"component_crx_cache", L"cache2", L"startupcache", L"jumplistcache",
    L"offlinecache", L"optimization_guide_model_store", L"browsermetrics", L"crash reports",
    L"minidumps", L"saved telemetry pings", L"datareporting"
};

static const wchar_t* kUserRegKeys[] = {
    L"Control Panel\\Mouse", L"Control Panel\\Keyboard", L"Control Panel\\International",
    L"Control Panel\\Colors", L"Control Panel\\Cursors", L"Control Panel\\Accessibility",
    L"Keyboard Layout\\Preload", L"Keyboard Layout\\Substitutes", L"Keyboard Layout\\Toggle",
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
    L"Software\\Microsoft\\Windows\\DWM",
    L"Console",
    L"Software\\Microsoft\\Notepad",
};

static const wchar_t* QUICK_ACCESS_REL =
    L"AppData\\Roaming\\Microsoft\\Windows\\Recent\\AutomaticDestinations\\f01b4d95cf55d32a.automaticDestinations-ms";
static const wchar_t* WALLPAPER_REL = L"AppData\\Roaming\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";

// program name -> winget id   (exact = whole name must match, otherwise prefix match)
struct WingetMap { const wchar_t* pattern; const wchar_t* id; bool exact; };
static const WingetMap kWinget[] = {
    {L"Google Chrome", L"Google.Chrome", false},
    {L"Mozilla Firefox", L"Mozilla.Firefox", false},
    {L"Mozilla Thunderbird", L"Mozilla.Thunderbird", false},
    {L"Brave", L"Brave.Brave", false},
    {L"Opera GX", L"Opera.OperaGX", false},
    {L"Opera Stable", L"Opera.Opera", false},
    {L"Opera", L"Opera.Opera", true},
    {L"Vivaldi", L"Vivaldi.Vivaldi", false},
    {L"7-Zip", L"7zip.7zip", false},
    {L"WinRAR", L"RARLab.WinRAR", false},
    {L"PeaZip", L"Giorgiotani.Peazip", false},
    {L"VLC media player", L"VideoLAN.VLC", false},
    {L"K-Lite Codec Pack", L"CodecGuide.K-LiteCodecPack.Standard", false},
    {L"PotPlayer", L"Daum.PotPlayer", false},
    {L"Notepad++", L"Notepad++.Notepad++", false},
    {L"Microsoft Visual Studio Code", L"Microsoft.VisualStudioCode", false},
    {L"GitHub Desktop", L"GitHub.GitHubDesktop", false},
    {L"Git", L"Git.Git", true},
    {L"Node.js", L"OpenJS.NodeJS.LTS", false},
    {L"Postman", L"Postman.Postman", false},
    {L"Docker Desktop", L"Docker.DockerDesktop", false},
    {L"Android Studio", L"Google.AndroidStudio", false},
    {L"IntelliJ IDEA Community", L"JetBrains.IntelliJIDEA.Community", false},
    {L"PyCharm Community", L"JetBrains.PyCharm.Community", false},
    {L"MySQL Workbench", L"Oracle.MySQLWorkbench", false},
    {L"Oracle VirtualBox", L"Oracle.VirtualBox", false},
    {L"Oracle VM VirtualBox", L"Oracle.VirtualBox", false},
    {L"Microsoft SQL Server Management Studio", L"Microsoft.SQLServerManagementStudio", false},
    {L"Arduino IDE", L"ArduinoSA.IDE.stable", false},
    {L"FileZilla", L"TimKosse.FileZilla.Client", false},
    {L"WinSCP", L"WinSCP.WinSCP", false},
    {L"PuTTY", L"PuTTY.PuTTY", false},
    {L"Zoom", L"Zoom.Zoom", false},
    {L"Microsoft Teams", L"Microsoft.Teams", false},
    {L"Discord", L"Discord.Discord", false},
    {L"Slack", L"SlackTechnologies.Slack", false},
    {L"Telegram Desktop", L"Telegram.TelegramDesktop", false},
    {L"Signal", L"OpenWhisperSystems.Signal", false},
    {L"Spotify", L"Spotify.Spotify", false},
    {L"Steam", L"Valve.Steam", true},
    {L"Epic Games Launcher", L"EpicGames.EpicGamesLauncher", false},
    {L"Adobe Acrobat Reader", L"Adobe.Acrobat.Reader.64-bit", false},
    {L"Adobe Acrobat (64-bit)", L"Adobe.Acrobat.Reader.64-bit", false},
    {L"Foxit PDF Reader", L"Foxit.FoxitReader", false},
    {L"SumatraPDF", L"SumatraPDF.SumatraPDF", false},
    {L"LibreOffice", L"TheDocumentFoundation.LibreOffice", false},
    {L"Microsoft 365", L"Microsoft.Office", false},
    {L"Microsoft Office", L"Microsoft.Office", false},
    {L"OBS Studio", L"OBSProject.OBSStudio", false},
    {L"Audacity", L"Audacity.Audacity", false},
    {L"GIMP", L"GIMP.GIMP", false},
    {L"Inkscape", L"Inkscape.Inkscape", false},
    {L"paint.net", L"dotPDN.PaintDotNet", false},
    {L"Figma", L"Figma.Figma", false},
    {L"ShareX", L"ShareX.ShareX", false},
    {L"Greenshot", L"Greenshot.Greenshot", false},
    {L"qBittorrent", L"qBittorrent.qBittorrent", false},
    {L"AnyDesk", L"AnyDeskSoftwareGmbH.AnyDesk", false},
    {L"TeamViewer", L"TeamViewer.TeamViewer", false},
    {L"Dropbox", L"Dropbox.Dropbox", false},
    {L"Google Drive", L"Google.GoogleDrive", false},
    {L"Everything", L"voidtools.Everything", false},
    {L"PowerToys", L"Microsoft.PowerToys", false},
    {L"CCleaner", L"Piriform.CCleaner", false},
    {L"Malwarebytes", L"Malwarebytes.Malwarebytes", false},
    {L"CPUID CPU-Z", L"CPUID.CPU-Z", false},
    {L"HWiNFO", L"REALiX.HWiNFO", false},
    {L"CrystalDiskInfo", L"CrystalDewWorld.CrystalDiskInfo", false},
    {L"CrystalDiskMark", L"CrystalDewWorld.CrystalDiskMark", false},
    {L"Rufus", L"Rufus.Rufus", false},
    {L"XAMPP", L"ApacheFriends.Xampp.8.2", false},
    {L"Java", L"Oracle.JavaRuntimeEnvironment", false},
};

// ---------------------------------------------------------------------
//  String helpers
// ---------------------------------------------------------------------
static wstring Fmt(const wchar_t* f, ...) {
    wchar_t b[4096]; va_list a; va_start(a, f);
    _vsnwprintf(b, 4095, f, a); va_end(a); b[4095] = 0; return b;
}
static wstring Lower(wstring s) { for (auto& c : s) c = (wchar_t)towlower(c); return s; }
static bool IEquals(const wstring& a, const wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) == 0; }
static bool IStarts(const wstring& s, const wstring& p) { return s.size() >= p.size() && _wcsnicmp(s.c_str(), p.c_str(), p.size()) == 0; }
static wstring Trim(const wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n"), b = s.find_last_not_of(L" \t\r\n");
    return a == wstring::npos ? L"" : s.substr(a, b - a + 1);
}
static wstring StripSlash(wstring p) { while (p.size() > 3 && (p.back() == L'\\' || p.back() == L'/')) p.pop_back(); return p; }
static wstring Join(const wstring& a, const wstring& b) {
    if (a.empty()) return b;
    if (a.back() == L'\\') return a + b;
    return a + L"\\" + b;
}
static wstring FileName(const wstring& p) { size_t i = p.find_last_of(L'\\'); return i == wstring::npos ? p : p.substr(i + 1); }
static wstring ReplaceAll(wstring s, const wstring& from, const wstring& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while (true) {
        // case-insensitive search
        size_t found = wstring::npos;
        for (size_t i = pos; i + from.size() <= s.size(); ++i)
            if (_wcsnicmp(s.c_str() + i, from.c_str(), from.size()) == 0) { found = i; break; }
        if (found == wstring::npos) break;
        s.replace(found, from.size(), to);
        pos = found + to.size();
    }
    return s;
}
static std::string ToUtf8(const wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}
static wstring FromBytes(const std::string& b, UINT cp) {
    if (b.empty()) return {};
    int n = MultiByteToWideChar(cp, 0, b.data(), (int)b.size(), nullptr, 0);
    wstring w(n, 0);
    MultiByteToWideChar(cp, 0, b.data(), (int)b.size(), &w[0], n);
    return w;
}
static wstring SizeStr(ULONGLONG v) {
    const wchar_t* u[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    double d = (double)v; int i = 0;
    while (d >= 1024 && i < 4) { d /= 1024; ++i; }
    return Fmt(i == 0 ? L"%.0f %ls" : L"%.2f %ls", d, u[i]);
}
static wstring ErrText(DWORD e) {
    wchar_t* buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, e, 0, (LPWSTR)&buf, 0, nullptr);
    wstring s = buf ? Trim(buf) : L"";
    if (buf) LocalFree(buf);
    return Fmt(L"error %lu: %ls", e, s.c_str());
}
static wstring TimeStamp(bool forFile) {
    SYSTEMTIME st; GetLocalTime(&st);
    if (forFile) return Fmt(L"%04d%02d%02d_%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return Fmt(L"%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}
static wstring Env(const wchar_t* n) {
    wchar_t b[MAX_PATH * 2]; DWORD r = GetEnvironmentVariableW(n, b, MAX_PATH * 2);
    return (r && r < MAX_PATH * 2) ? wstring(b) : wstring();
}

// ---------------------------------------------------------------------
//  Logging (thread-safe, to UI + file)
// ---------------------------------------------------------------------
static void Log(const wstring& s) {
    SYSTEMTIME st; GetLocalTime(&st);
    wstring line = Fmt(L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond) + s;
    EnterCriticalSection(&g_logCs);
    if (g_logFile != INVALID_HANDLE_VALUE) {
        std::string u = ToUtf8(line + L"\r\n"); DWORD w;
        WriteFile(g_logFile, u.data(), (DWORD)u.size(), &w, nullptr);
    }
    LeaveCriticalSection(&g_logCs);
    PostMessageW(g_hwnd, WM_APP_LOG, 0, (LPARAM)new wstring(line + L"\r\n"));
}
static void OpenLogFile(const wstring& path) {
    EnterCriticalSection(&g_logCs);
    g_logFile = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (g_logFile != INVALID_HANDLE_VALUE && GetLastError() != ERROR_ALREADY_EXISTS) {
        DWORD w; const unsigned char bom[3] = {0xEF, 0xBB, 0xBF}; WriteFile(g_logFile, bom, 3, &w, nullptr);
    }
    LeaveCriticalSection(&g_logCs);
}
static void CloseLogFile() {
    EnterCriticalSection(&g_logCs);
    if (g_logFile != INVALID_HANDLE_VALUE) CloseHandle(g_logFile);
    g_logFile = INVALID_HANDLE_VALUE;
    LeaveCriticalSection(&g_logCs);
}
static ULONGLONG g_lastReport = 0;
static void Report(int permille, const wstring& status, bool force = false) {
    ULONGLONG now = GetTickCount64();
    if (!force && now - g_lastReport < 120) return;
    g_lastReport = now;
    PostMessageW(g_hwnd, WM_APP_PROGRESS, (WPARAM)permille, (LPARAM)new wstring(status));
}

// ---------------------------------------------------------------------
//  File-system helpers (long-path aware)
// ---------------------------------------------------------------------
static wstring LongP(const wstring& p) {
    if (IStarts(p, L"\\\\?\\")) return p;
    if (p.size() >= 2 && p[1] == L':') return L"\\\\?\\" + p;
    if (IStarts(p, L"\\\\")) return L"\\\\?\\UNC\\" + p.substr(2);
    return p;
}
static DWORD Attr(const wstring& p) { return GetFileAttributesW(LongP(p).c_str()); }
static bool IsDir(const wstring& p) { DWORD a = Attr(p); return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY); }
static bool IsFile(const wstring& p) { DWORD a = Attr(p); return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY); }
static bool MakeDirs(wstring p) {
    p = StripSlash(p);
    if (p.empty()) return false;
    if (IsDir(p)) return true;
    size_t pos = p.find_last_of(L'\\');
    if (pos != wstring::npos && pos > 2) MakeDirs(p.substr(0, pos));
    return CreateDirectoryW(LongP(p).c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}
static vector<WIN32_FIND_DATAW> ListDir(const wstring& dir) {
    vector<WIN32_FIND_DATAW> out;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileExW(LongP(Join(dir, L"*")).c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        out.push_back(fd);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return out;
}
static bool ReadAll(const wstring& path, std::string& out) {
    HANDLE h = CreateFileW(LongP(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz; GetFileSizeEx(h, &sz);
    out.resize((size_t)sz.QuadPart);
    DWORD r = 0; bool ok = sz.QuadPart == 0 || ReadFile(h, &out[0], (DWORD)sz.QuadPart, &r, nullptr);
    CloseHandle(h);
    return ok;
}
static bool WriteAll(const wstring& path, const void* data, size_t n) {
    HANDLE h = CreateFileW(LongP(path).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD w = 0; bool ok = n == 0 || WriteFile(h, data, (DWORD)n, &w, nullptr);
    CloseHandle(h);
    return ok;
}
// text read with BOM detection (UTF-16LE / UTF-8 / ANSI fallback)
static bool ReadText(const wstring& path, wstring& out) {
    std::string b; if (!ReadAll(path, b)) return false;
    if (b.size() >= 2 && (unsigned char)b[0] == 0xFF && (unsigned char)b[1] == 0xFE) {
        out.assign((const wchar_t*)(b.data() + 2), (b.size() - 2) / 2);
    } else if (b.size() >= 3 && (unsigned char)b[0] == 0xEF && (unsigned char)b[1] == 0xBB && (unsigned char)b[2] == 0xBF) {
        out = FromBytes(b.substr(3), CP_UTF8);
    } else {
        int ok = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, b.data(), (int)b.size(), nullptr, 0);
        out = FromBytes(b, ok ? CP_UTF8 : CP_ACP);
    }
    return true;
}
static bool WriteUtf16(const wstring& path, const wstring& text) {
    std::string b("\xFF\xFE", 2);
    b.append((const char*)text.data(), text.size() * 2);
    return WriteAll(path, b.data(), b.size());
}
static bool WriteUtf8(const wstring& path, const wstring& text, bool bom) {
    std::string b = bom ? std::string("\xEF\xBB\xBF") : std::string();
    b += ToUtf8(text);
    return WriteAll(path, b.data(), b.size());
}
static ULONGLONG FreeSpace(const wstring& path) {
    ULARGE_INTEGER fb; if (GetDiskFreeSpaceExW(path.c_str(), &fb, nullptr, nullptr)) return fb.QuadPart;
    return 0;
}

// ---------------------------------------------------------------------
//  Process helpers
// ---------------------------------------------------------------------
static bool RunProcess(const wstring& cmd, DWORD* exitCode, DWORD timeoutMs = INFINITE) {
    STARTUPINFOW si{}; si.cb = sizeof(si); si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    vector<wchar_t> buf(cmd.begin(), cmd.end()); buf.push_back(0);
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        if (exitCode) *exitCode = GetLastError();
        return false;
    }
    WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD ec = 0; GetExitCodeProcess(pi.hProcess, &ec);
    if (exitCode) *exitCode = ec;
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return true;
}
static wstring SysExe(const wchar_t* exe) {
    wchar_t b[MAX_PATH]; GetSystemDirectoryW(b, MAX_PATH);
    return Join(b, exe);
}
static bool IsProcessRunning(const wchar_t* exe) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do { if (_wcsicmp(pe.szExeFile, exe) == 0) { found = true; break; } } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

// ---------------------------------------------------------------------
//  Privileges / environment detection
// ---------------------------------------------------------------------
static bool EnablePrivilege(const wchar_t* name) {
    HANDLE tok;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) return false;
    TOKEN_PRIVILEGES tp{}; tp.PrivilegeCount = 1; tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    bool ok = LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid) &&
              AdjustTokenPrivileges(tok, FALSE, &tp, sizeof(tp), nullptr, nullptr) && GetLastError() == ERROR_SUCCESS;
    CloseHandle(tok);
    return ok;
}
static bool DetectWinPE() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\MiniNT", 0, KEY_READ, &k) == ERROR_SUCCESS) {
        RegCloseKey(k); return true;
    }
    return false;
}
static wstring DriveOf(const wstring& p) { return p.size() >= 2 ? Lower(p.substr(0, 2)) : L""; }

static bool IsSkippedProfile(const wstring& name) {
    static const wchar_t* skip[] = {L"default", L"default user", L"public", L"all users", L"defaultuser0", L"defaultuser100000", L"defaultapppool"};
    wstring n = Lower(name);
    for (auto s : skip) if (n == s) return true;
    return false;
}
static vector<wstring> EnumProfiles(const wstring& drive) {
    vector<wstring> out;
    wstring users = drive + L"\\Users";
    for (auto& fd : ListDir(users)) {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) continue;
        if (IsSkippedProfile(fd.cFileName)) continue;
        wstring p = Join(users, fd.cFileName);
        if (IsFile(Join(p, L"NTUSER.DAT"))) out.push_back(p);
    }
    return out;
}
static vector<WinInstall> DetectInstalls() {
    vector<WinInstall> out;
    DWORD mask = GetLogicalDrives();
    for (int i = 2; i < 26; ++i) {                     // skip A: B:
        if (!(mask & (1u << i))) continue;
        wstring root = Fmt(L"%lc:\\", L'A' + i);
        UINT t = GetDriveTypeW(root.c_str());
        if (t != DRIVE_FIXED && t != DRIVE_REMOVABLE) continue;
        wstring drv = root.substr(0, 2);
        if (g_isPE && IEquals(drv, g_sysDrive)) continue;      // the WinPE RAM disk (X:)
        if (!IsFile(drv + L"\\Windows\\System32\\config\\SOFTWARE")) continue;
        if (EnumProfiles(drv).empty()) continue;
        wchar_t vol[MAX_PATH] = L"";
        GetVolumeInformationW(root.c_str(), vol, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0);
        ULARGE_INTEGER total{}, freeB{};
        GetDiskFreeSpaceExW(root.c_str(), nullptr, &total, &freeB);
        WinInstall w;
        w.drive = drv;
        w.live = !g_isPE && IEquals(drv, g_sysDrive);
        w.label = Fmt(L"%ls  [%ls]  %ls total, %ls free%ls", drv.c_str(), vol[0] ? vol : L"no label",
                      SizeStr(total.QuadPart).c_str(), SizeStr(freeB.QuadPart).c_str(),
                      w.live ? L"   (running system)" : L"   (offline)");
        out.push_back(w);
    }
    return out;
}

// ---------------------------------------------------------------------
//  Registry hive helpers
// ---------------------------------------------------------------------
struct Hive {
    HKEY base = nullptr;   // handle to hive root
    wstring shortPath;     // "HKU\\xxx"   (for reg.exe)
    wstring longPath;      // "HKEY_USERS\\xxx" (as it appears in .reg files)
    wstring mount;         // mount name if we loaded it
    bool loaded = false;
};
// If the profile belongs to the running system and its hive is already loaded, find its SID
static wstring FindLoadedSid(const wstring& profile) {
    if (g_isPE) return L"";
    HKEY pl;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", 0, KEY_READ | KEY_WOW64_64KEY, &pl) != ERROR_SUCCESS)
        return L"";
    wstring result;
    wchar_t name[256];
    for (DWORD i = 0;; ++i) {
        DWORD n = 256;
        if (RegEnumKeyExW(pl, i, name, &n, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        wchar_t raw[MAX_PATH * 2], path[MAX_PATH * 2]; DWORD sz = sizeof(raw);
        if (RegGetValueW(pl, name, L"ProfileImagePath", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr, raw, &sz) != ERROR_SUCCESS) continue;
        if (!ExpandEnvironmentStringsW(raw, path, MAX_PATH * 2)) continue;
        if (IEquals(StripSlash(path), StripSlash(profile))) {
            HKEY t;
            if (RegOpenKeyExW(HKEY_USERS, name, 0, KEY_READ, &t) == ERROR_SUCCESS) { RegCloseKey(t); result = name; }
            break;
        }
    }
    RegCloseKey(pl);
    return result;
}
static bool OpenUserHive(const wstring& profile, const wstring& mount, bool write, Hive& h) {
    REGSAM sam = KEY_READ | (write ? KEY_WRITE : 0);
    wstring sid = FindLoadedSid(profile);
    if (!sid.empty() && RegOpenKeyExW(HKEY_USERS, sid.c_str(), 0, sam, &h.base) == ERROR_SUCCESS) {
        h.shortPath = L"HKU\\" + sid; h.longPath = L"HKEY_USERS\\" + sid; h.loaded = false;
        return true;
    }
    wstring dat = Join(profile, L"NTUSER.DAT");
    LONG r = RegLoadKeyW(HKEY_USERS, mount.c_str(), dat.c_str());
    if (r != ERROR_SUCCESS) { Log(L"  ! Cannot load registry of " + FileName(profile) + L": " + ErrText(r)); return false; }
    h.loaded = true; h.mount = mount;
    h.shortPath = L"HKU\\" + mount; h.longPath = L"HKEY_USERS\\" + mount;
    if (RegOpenKeyExW(HKEY_USERS, mount.c_str(), 0, sam, &h.base) != ERROR_SUCCESS) h.base = nullptr;
    return h.base != nullptr;
}
static void CloseHive(Hive& h, HKEY parent = HKEY_USERS) {
    if (h.base) { RegCloseKey(h.base); h.base = nullptr; }
    if (h.loaded) {
        for (int i = 0; i < 10; ++i) {
            if (RegUnLoadKeyW(parent, h.mount.c_str()) == ERROR_SUCCESS) break;
            Sleep(300);
        }
        h.loaded = false;
    }
}
static bool KeyExists(HKEY base, const wstring& sub) {
    HKEY k; if (RegOpenKeyExW(base, sub.c_str(), 0, KEY_READ, &k) != ERROR_SUCCESS) return false;
    RegCloseKey(k); return true;
}
static wstring RegStr(HKEY k, const wchar_t* sub, const wchar_t* val) {
    wchar_t buf[2048]; DWORD sz = sizeof(buf);
    if (RegGetValueW(k, sub, val, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr, buf, &sz) == ERROR_SUCCESS) return buf;
    return L"";
}
static DWORD RegDword(HKEY k, const wchar_t* sub, const wchar_t* val, DWORD def = 0) {
    DWORD v = 0, sz = sizeof(v);
    if (RegGetValueW(k, sub, val, RRF_RT_REG_DWORD, nullptr, &v, &sz) == ERROR_SUCCESS) return v;
    return def;
}

// ---------------------------------------------------------------------
//  Installed-programs inventory
// ---------------------------------------------------------------------
struct Program { wstring name, version, publisher, scope; };
// common filter for uninstall entries (hides updates, runtimes, system components)
static bool KeepEntry(HKEY k, const wchar_t* name, wstring& dn) {
    dn = Trim(RegStr(k, name, L"DisplayName"));
    if (dn.empty()) return false;
    if (RegDword(k, name, L"SystemComponent") == 1) return false;
    if (!RegStr(k, name, L"ParentKeyName").empty()) return false;
    wstring rt = Lower(RegStr(k, name, L"ReleaseType"));
    if (rt == L"update" || rt == L"hotfix" || rt == L"security update") return false;
    wstring l = Lower(dn);
    if (l.find(L"(kb") != wstring::npos || IStarts(l, L"update for") || IStarts(l, L"security update") ||
        IStarts(l, L"microsoft visual c++") || IStarts(l, L"microsoft .net") || IStarts(l, L"microsoft edge") ||
        IStarts(l, L"microsoft update health") || IStarts(l, L"microsoft windows desktop runtime") ||
        IStarts(l, L"windows sdk") || IStarts(l, L"microsoft onedrive"))
        return false;
    return true;
}
static void EnumUninstall(HKEY base, const wchar_t* sub, const wchar_t* scope, std::map<wstring, Program>& out) {
    HKEY k;
    if (RegOpenKeyExW(base, sub, 0, KEY_READ | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) return;
    wchar_t name[512];
    for (DWORD i = 0;; ++i) {
        DWORD n = 512;
        if (RegEnumKeyExW(k, i, name, &n, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        wstring dn;
        if (!KeepEntry(k, name, dn)) continue;
        wstring l = Lower(dn);
        if (out.count(l)) continue;
        out[l] = Program{dn, Trim(RegStr(k, name, L"DisplayVersion")), Trim(RegStr(k, name, L"Publisher")), scope};
    }
    RegCloseKey(k);
}
static wstring WingetIdFor(const wstring& name) {
    // Python 3.x -> Python.Python.3.x
    if (IStarts(name, L"Python 3.")) {
        int minor = _wtoi(name.c_str() + 9);
        if (minor > 0 && name.find(L"Launcher") == wstring::npos) return Fmt(L"Python.Python.3.%d", minor);
        return L"";
    }
    for (auto& m : kWinget) {
        if (m.exact ? IEquals(name, m.pattern) : IStarts(name, m.pattern)) return m.id;
    }
    return L"";
}
// remove version numbers and "(x64)" etc. for name-based winget search
static wstring CleanName(const wstring& n) {
    wstring s;
    int depth = 0;
    for (wchar_t c : n) { if (c == L'(' || c == L'[') depth++; else if (c == L')' || c == L']') { if (depth) depth--; } else if (!depth) s += c; }
    vector<wstring> words; wstring cur;
    for (wchar_t c : s) { if (c == L' ') { if (!cur.empty()) words.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) words.push_back(cur);
    wstring r;
    for (auto& w : words) {
        if (!r.empty() && (iswdigit(w[0]) || ((w[0] == L'v' || w[0] == L'V') && w.size() > 1 && iswdigit(w[1])))) break;
        if (!r.empty()) r += L' ';
        r += w;
    }
    return Trim(r.empty() ? s : r);
}

// ---------------------------------------------------------------------
//  Copy engine
// ---------------------------------------------------------------------
struct CopyTask { wstring src, dst; bool isFile = false; bool skipCaches = false; };
struct CopyStats { ULONGLONG total = 0, done = 0; DWORD files = 0, failed = 0, skipped = 0, cloud = 0; };
static CopyStats* g_st = nullptr;
static ULONGLONG g_fileBase = 0;
static wstring g_curFile;

static bool IsCacheDir(const wchar_t* name) {
    wstring n = Lower(name);
    for (auto c : kCacheDirs) if (n == c) return true;
    return false;
}
static bool IsCloudOnly(DWORD a) {
    return (a & (FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS | FILE_ATTRIBUTE_RECALL_ON_OPEN)) != 0;
}
static void ScanDir(const wstring& dir, bool skipCaches, ULONGLONG& total) {
    if (g_cancel) return;
    for (auto& fd : ListDir(dir)) {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
            if (skipCaches && IsCacheDir(fd.cFileName)) continue;
            ScanDir(Join(dir, fd.cFileName), skipCaches, total);
        } else if (!IsCloudOnly(fd.dwFileAttributes)) {
            total += ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        }
    }
}
static int Permille() { return g_st && g_st->total ? (int)(g_st->done * 1000 / g_st->total) : 0; }

static DWORD CALLBACK CopyProgressCb(LARGE_INTEGER, LARGE_INTEGER done, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID) {
    if (g_st) {
        g_st->done = g_fileBase + done.QuadPart;
        Report(Permille(), Fmt(L"Copying %ls / %ls  :  %ls", SizeStr(g_st->done).c_str(), SizeStr(g_st->total).c_str(), g_curFile.c_str()));
    }
    return g_cancel ? PROGRESS_CANCEL : PROGRESS_CONTINUE;
}
// fallback copy using backup semantics (reads files of other users / some locked files)
static bool BackupCopy(const wstring& src, const wstring& dst, bool openReparse = false) {
    HANDLE in = CreateFileW(LongP(src).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN | (openReparse ? FILE_FLAG_OPEN_REPARSE_POINT : 0), nullptr);
    if (in == INVALID_HANDLE_VALUE) return false;
    HANDLE out = CreateFileW(LongP(dst).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (out == INVALID_HANDLE_VALUE) { CloseHandle(in); return false; }
    static vector<char> buf(1 << 20);
    bool ok = true; DWORD r, w;
    while (!g_cancel) {
        if (!ReadFile(in, buf.data(), (DWORD)buf.size(), &r, nullptr)) { ok = false; break; }
        if (r == 0) break;
        if (!WriteFile(out, buf.data(), r, &w, nullptr) || w != r) { ok = false; break; }
        g_st->done += r;
        Report(Permille(), Fmt(L"Copying %ls / %ls  :  %ls", SizeStr(g_st->done).c_str(), SizeStr(g_st->total).c_str(), g_curFile.c_str()));
    }
    FILETIME c, a, m;
    if (GetFileTime(in, &c, &a, &m)) SetFileTime(out, &c, &a, &m);
    CloseHandle(in); CloseHandle(out);
    if (!ok || g_cancel) DeleteFileW(LongP(dst).c_str());
    return ok && !g_cancel;
}
static void CopyOneFile(const wstring& src, const wstring& dst, const WIN32_FIND_DATAW* fdIn) {
    WIN32_FILE_ATTRIBUTE_DATA sa{};
    if (fdIn) { sa.dwFileAttributes = fdIn->dwFileAttributes; sa.ftLastWriteTime = fdIn->ftLastWriteTime; sa.nFileSizeHigh = fdIn->nFileSizeHigh; sa.nFileSizeLow = fdIn->nFileSizeLow; }
    else if (!GetFileAttributesExW(LongP(src).c_str(), GetFileExInfoStandard, &sa)) { g_st->failed++; Log(L"  ! " + src + L" : " + ErrText(GetLastError())); return; }
    ULONGLONG size = ((ULONGLONG)sa.nFileSizeHigh << 32) | sa.nFileSizeLow;
    if (IsCloudOnly(sa.dwFileAttributes)) { g_st->cloud++; return; }
    // destination exists and is not older -> keep it
    WIN32_FILE_ATTRIBUTE_DATA da{};
    if (GetFileAttributesExW(LongP(dst).c_str(), GetFileExInfoStandard, &da)) {
        if (CompareFileTime(&sa.ftLastWriteTime, &da.ftLastWriteTime) <= 0) { g_st->skipped++; g_st->done += size; return; }
        SetFileAttributesW(LongP(dst).c_str(), FILE_ATTRIBUTE_NORMAL);
    }
    g_curFile = FileName(src);
    g_fileBase = g_st->done;
    if (CopyFileExW(LongP(src).c_str(), LongP(dst).c_str(), CopyProgressCb, nullptr, &g_cancel, 0)) {
        g_st->files++; g_st->done = g_fileBase + size; return;
    }
    DWORD e = GetLastError();
    if (g_cancel) return;
    g_st->done = g_fileBase;
    if ((e == ERROR_ACCESS_DENIED || e == ERROR_SHARING_VIOLATION) && BackupCopy(src, dst)) { g_st->files++; return; }
    // OneDrive "available on this device" files keep a cloud reparse point; offline (WinPE) read the data stream directly
    if ((sa.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && BackupCopy(src, dst, true)) { g_st->files++; return; }
    g_st->failed++; g_st->done = g_fileBase + size;
    Log(L"  ! " + src + L" : " + ErrText(e));
}
static void CopyTree(const wstring& src, const wstring& dst, bool skipCaches) {
    if (g_cancel) return;
    if (!MakeDirs(dst)) { Log(L"  ! cannot create " + dst + L" : " + ErrText(GetLastError())); g_st->failed++; return; }
    for (auto& fd : ListDir(src)) {
        if (g_cancel) return;
        wstring s = Join(src, fd.cFileName), d = Join(dst, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;   // junctions (My Music, Application Data...)
            if (skipCaches && IsCacheDir(fd.cFileName)) continue;
            CopyTree(s, d, skipCaches);
        } else {
            CopyOneFile(s, d, &fd);
        }
    }
}
// scan + copy a list of tasks with progress; returns false if cancelled / not enough space
static bool RunCopyTasks(const vector<CopyTask>& tasks, const wstring& spaceCheckPath) {
    CopyStats st; g_st = &st;
    Log(Fmt(L"Scanning %d location(s) ...", (int)tasks.size()));
    Report(0, L"Scanning files ...", true);
    for (auto& t : tasks) {
        if (t.isFile) { WIN32_FILE_ATTRIBUTE_DATA a; if (GetFileAttributesExW(LongP(t.src).c_str(), GetFileExInfoStandard, &a)) st.total += ((ULONGLONG)a.nFileSizeHigh << 32) | a.nFileSizeLow; }
        else ScanDir(t.src, t.skipCaches, st.total);
    }
    if (g_cancel) { g_st = nullptr; return false; }
    Log(L"Total to copy: " + SizeStr(st.total));
    if (!spaceCheckPath.empty()) {
        ULONGLONG fs = FreeSpace(spaceCheckPath);
        if (fs && fs < st.total + 50ull * 1024 * 1024) {
            Log(L"ERROR: not enough free space on destination (" + SizeStr(fs) + L" free, " + SizeStr(st.total) + L" needed).");
            g_st = nullptr; return false;
        }
    }
    for (auto& t : tasks) {
        if (g_cancel) break;
        Log(L"-> " + t.src);
        if (t.isFile) {
            size_t p = t.dst.find_last_of(L'\\');
            if (p != wstring::npos) MakeDirs(t.dst.substr(0, p));
            CopyOneFile(t.src, t.dst, nullptr);
        } else CopyTree(t.src, t.dst, t.skipCaches);
    }
    Report(1000, L"Copy finished", true);
    Log(Fmt(L"Copied %lu file(s), %lu unchanged, %lu failed, %lu cloud-only (OneDrive placeholders) skipped.",
            st.files, st.skipped, st.failed, st.cloud));
    g_st = nullptr;
    return !g_cancel;
}

// ---------------------------------------------------------------------
//  Registry export / import through reg.exe
// ---------------------------------------------------------------------
typedef vector<std::pair<wstring, wstring>> Repl;
static wstring ApplyRepl(wstring t, const Repl& r) { for (auto& p : r) t = ReplaceAll(t, p.first, p.second); return t; }
// export base\sub (shortRoot is how reg.exe names `base`) to a portable .reg file
static bool ExportKey(HKEY base, const wstring& sub, const wstring& shortRoot, const Repl& repl, const wstring& outFile) {
    if (!KeyExists(base, sub)) return false;
    DWORD ec = 1;
    wstring cmd = L"\"" + SysExe(L"reg.exe") + L"\" export \"" + shortRoot + L"\\" + sub + L"\" \"" + outFile + L"\" /y";
    if (!RunProcess(cmd, &ec, 60000) || ec != 0) { Log(L"  ! reg export failed for " + sub); return false; }
    wstring txt;
    if (ReadText(outFile, txt)) WriteUtf16(outFile, ApplyRepl(txt, repl));
    return true;
}
static bool ImportReg(const wstring& regFile, const Repl& repl) {
    wstring txt;
    if (!ReadText(regFile, txt)) return false;
    txt = ApplyRepl(txt, repl);
    wchar_t tmpDir[MAX_PATH]; GetTempPathW(MAX_PATH, tmpDir);
    wstring tmp = Join(tmpDir, L"pcmig_import.reg");
    if (!WriteUtf16(tmp, txt)) return false;
    DWORD ec = 1;
    bool ok = RunProcess(L"\"" + SysExe(L"reg.exe") + L"\" import \"" + tmp + L"\"", &ec, 120000) && ec == 0;
    DeleteFileW(tmp.c_str());
    return ok;
}
static bool ExportUserKey(const Hive& h, const wstring& sub, const wstring& outFile) {
    return ExportKey(h.base, sub, h.shortPath, {{L"[" + h.longPath, L"[HKEY_CURRENT_USER"}}, outFile);
}
static bool ImportUserReg(const Hive& h, const wstring& regFile) {
    return ImportReg(regFile, {{L"[HKEY_CURRENT_USER", L"[" + h.longPath}});
}

// ---------------------------------------------------------------------
//  Program transfer engine (files + registry + services + per-user data)
// ---------------------------------------------------------------------
static wstring ParentDir(const wstring& p) {
    wstring s = StripSlash(p);
    size_t i = s.find_last_of(L'\\');
    if (i == wstring::npos) return L"";
    if (i <= 2) return s.substr(0, 3);
    return s.substr(0, i);
}
// "C:\x\a.exe" /i ,  "C:\x\a.exe,0"  ->  C:\x\a.exe
static wstring ExeFromCommand(wstring s) {
    s = Trim(s);
    if (s.empty()) return s;
    if (s[0] == L'"') { size_t e = s.find(L'"', 1); return e == wstring::npos ? s.substr(1) : s.substr(1, e - 1); }
    wstring l = Lower(s);
    size_t e = l.find(L".exe");
    if (e != wstring::npos) return s.substr(0, e + 4);
    size_t c = s.find(L',');
    return c == wstring::npos ? s : s.substr(0, c);
}
static vector<wstring> SplitPath(const wstring& p) {
    vector<wstring> v; wstring cur;
    for (wchar_t c : p) { if (c == L'\\') { if (!cur.empty()) v.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) v.push_back(cur);
    return v;
}
static bool IsGenericFolder(const wstring& n) {
    static const wchar_t* g[] = {L"program files", L"program files (x86)", L"programdata", L"programs", L"local", L"roaming",
                                 L"locallow", L"appdata", L"users", L"common files", L"windows", L"temp", L"package cache"};
    wstring l = Lower(n);
    for (auto x : g) if (l == x) return true;
    return false;
}
static bool ValidInstallDir(const wstring& d) {
    if (d.size() < 4 || d[1] != L':' || d[2] != L'\\') return false;
    wstring l = Lower(d);
    if (IStarts(l.substr(2), L"\\windows")) return false;
    if (l.find(L"\\package cache") != wstring::npos || l.find(L"\\installer\\") != wstring::npos ||
        l.find(L"\\temp\\") != wstring::npos || l.find(L"{") != wstring::npos || l.find(L"\\windowsapps") != wstring::npos)
        return false;
    vector<wstring> c = SplitPath(d.substr(3));
    if (c.empty() || IsGenericFolder(c.back())) return false;
    if (Lower(c[0]) == L"users" && c.size() < 4) return false;
    return true;
}
static wstring DetermineInstallDir(HKEY k, const wchar_t* sub) {
    wstring loc = Trim(RegStr(k, sub, L"InstallLocation"));
    if (!loc.empty() && loc[0] == L'"') loc = ExeFromCommand(loc);
    loc = StripSlash(loc);
    if (ValidInstallDir(loc)) return loc;
    wstring icon = ExeFromCommand(RegStr(k, sub, L"DisplayIcon"));
    if (!icon.empty() && Lower(icon).find(L"msiexec") == wstring::npos) {
        wstring d = ParentDir(icon);
        if (ValidInstallDir(d)) return d;
    }
    wstring un = ExeFromCommand(RegStr(k, sub, L"UninstallString"));
    wstring lu = Lower(un);
    if (!un.empty() && lu.find(L"msiexec") == wstring::npos && lu.find(L"rundll32") == wstring::npos) {
        wstring d = ParentDir(un);
        if (ValidInstallDir(d)) return d;
    }
    return L"";
}
// logical path (C:\...) -> where it really is on the source disk
static wstring MapToSource(const wstring& logical, const wstring& srcDrive, bool live) {
    if (live || logical.size() < 3) return logical;
    wstring cand = srcDrive + logical.substr(2);
    if (IsDir(cand)) return cand;
    if (IsDir(logical) && !IEquals(DriveOf(logical), DriveOf(g_sysDrive))) return logical;
    return cand;
}
static wstring FirstWord(const wstring& s) {
    wstring w;
    for (wchar_t c : Trim(s)) { if (c == L' ') break; w += c; }
    while (!w.empty() && (w.back() == L',' || w.back() == L'.' || w.back() == L':' || w.back() == L'-')) w.pop_back();
    return w;
}
// names under which a program usually stores its data / registry keys
struct AppNames { vector<wstring> cands; wstring clean, first, parent, pub; };
static AppNames Candidates(const AppInfo& a) {
    static const wchar_t* leafs[] = {L"application", L"app", L"bin", L"program", L"x64", L"x86", L"current", L"win64", L"win32", L"64bit", L"32bit"};
    static const wchar_t* black[] = {L"microsoft", L"windows", L"classes", L"policies", L"wow6432node", L"clients",
        L"registeredapplications", L"temp", L"packages", L"package cache", L"programs", L"common files", L"software",
        L"system", L"appdata", L"users", L"default", L"public", L"windowsapps", L"start menu", L"crypto", L"comms",
        L"connecteddevicesplatform", L"d3dscache", L"history", L"inetcache", L"virtualstore", L"google", L"mozilla", L"intel"};
    AppNames r;
    std::set<wstring> seen;
    auto isLeaf = [&](const wstring& n) { wstring l = Lower(n); for (auto x : leafs) if (l == x) return true; return false; };
    auto add = [&](const wstring& s0) {
        wstring s = Trim(s0);
        if (s.size() < 2) return;
        for (auto& comp : SplitPath(s)) { wstring l = Lower(comp); for (auto b : black) if (l == b) return; }
        if (seen.insert(Lower(s)).second) r.cands.push_back(s);
    };
    wstring folder, parent;
    if (!a.installDir.empty()) {
        wstring d = a.installDir;
        folder = FileName(d);
        for (int i = 0; i < 3 && isLeaf(folder); ++i) { d = ParentDir(d); folder = FileName(d); }
        wstring p = FileName(ParentDir(d));
        if (!IsGenericFolder(p) && p.find(L':') == wstring::npos) parent = p;
        if (IsGenericFolder(folder) || folder.find(L':') != wstring::npos) folder.clear();
    }
    r.clean = CleanName(a.name);
    r.first = FirstWord(r.clean);
    wstring pub = FirstWord(a.publisher);
    r.parent = parent;
    {
        bool bl = pub.size() < 4;
        for (auto b : black) if (IEquals(pub, b)) bl = true;
        if (!bl) r.pub = pub;
    }
    if (!folder.empty()) { add(folder); if (!parent.empty()) add(parent + L"\\" + folder); }
    add(r.clean);
    if (r.first.size() >= 3 && !IEquals(r.first, pub)) add(r.first); else r.first.clear();
    if (pub.size() >= 3) {
        if (!folder.empty()) add(pub + L"\\" + folder);
        add(pub + L"\\" + r.clean);
    }
    if (IStarts(a.name, L"Microsoft Visual Studio Code")) add(L"Code");
    return r;
}
static void EnumApps(HKEY base, const wchar_t* sub, const wchar_t* scope, const wstring& owner,
                     const wstring& srcDrive, bool live, std::set<wstring>& seen, vector<AppInfo>& out) {
    HKEY k;
    if (RegOpenKeyExW(base, sub, 0, KEY_READ | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) return;
    wchar_t name[512];
    for (DWORD i = 0;; ++i) {
        DWORD n = 512;
        if (RegEnumKeyExW(k, i, name, &n, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        wstring dn;
        if (!KeepEntry(k, name, dn)) continue;
        if (!seen.insert(Lower(dn)).second) continue;
        AppInfo a;
        a.name = dn; a.version = Trim(RegStr(k, name, L"DisplayVersion")); a.publisher = Trim(RegStr(k, name, L"Publisher"));
        a.scope = scope; a.owner = owner; a.uninstallSub = wstring(sub) + L"\\" + name;
        a.sizeKB = RegDword(k, name, L"EstimatedSize");
        a.installDir = DetermineInstallDir(k, name);
        if (RegDword(k, name, L"WindowsInstaller") == 1) a.note = L"MSI";
        if (!a.installDir.empty()) {
            a.srcDir = MapToSource(a.installDir, srcDrive, live);
            if (!IsDir(a.srcDir)) { a.note += a.note.empty() ? L"" : L", "; a.note += L"install folder not found - data only"; a.installDir.clear(); a.srcDir.clear(); }
        } else {
            a.note += a.note.empty() ? L"" : L", ";
            a.note += L"no install folder - data only";
        }
        out.push_back(a);
    }
    RegCloseKey(k);
}
static void StartMenuTasks(const wstring& dir, const wstring& dst, const AppInfo& a, const AppNames& nm, vector<CopyTask>& tasks) {
    for (auto& fd : ListDir(dir)) {
        wstring n = fd.cFileName;
        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        wstring base = n;
        if (!isDir) {
            if (n.size() < 5 || !IEquals(n.substr(n.size() - 4), L".lnk")) continue;
            base = n.substr(0, n.size() - 4);
        }
        bool match = IEquals(base, a.name) || IEquals(base, nm.clean) || (!nm.first.empty() && IStarts(base, nm.first));
        for (auto& c : nm.cands) if (c.find(L'\\') == wstring::npos && IEquals(base, c)) match = true;
        if (isDir && ((!nm.parent.empty() && IEquals(base, nm.parent)) || (!nm.pub.empty() && IStarts(base, nm.pub)))) match = true;
        if (match) tasks.push_back({Join(dir, n), Join(dst, n), !isDir, false});
    }
}
static wstring SafeName(const wstring& n) {
    wstring s;
    for (wchar_t c : n) s += (wcschr(L"\\/:*?\"<>|", c) || c < 32) ? L'_' : c;
    s = Trim(s);
    while (!s.empty() && s.back() == L'.') s.pop_back();
    if (s.size() > 60) s = s.substr(0, 60);
    return s.empty() ? L"app" : s;
}
static wstring EscReg(const wstring& p) { return ReplaceAll(p, L"\\", L"\\\\"); }
// .reg replacements to move user paths C:\Users\Old -> C:\Users\New
static Repl UserPathRepl(const wstring& oldUser, const wstring& newUser) {
    Repl r;
    if (oldUser.empty() || newUser.empty() || IEquals(oldUser, newUser)) return r;
    r.push_back({EscReg(L"\\Users\\" + oldUser + L"\\"), EscReg(L"\\Users\\" + newUser + L"\\")});
    r.push_back({EscReg(L"\\Users\\" + oldUser) + L"\"", EscReg(L"\\Users\\" + newUser) + L"\""});
    return r;
}

// ---------------------------------------------------------------------
//  Backup job
// ---------------------------------------------------------------------
struct BackupJob {
    wstring srcDrive; bool live = false;
    vector<wstring> profiles;
    bool files = true, browsers = true, settings = true, programs = true;
    wstring destRoot;
    vector<AppInfo> apps;          // programs to transfer with files + data
};
static void BackupWiFi(const BackupJob& j, const wstring& pkg) {
    wstring dir = pkg + L"\\System\\WiFi";
    MakeDirs(dir);
    int count = 0;
    if (j.live) {
        DWORD ec = 1;
        RunProcess(L"\"" + SysExe(L"netsh.exe") + L"\" wlan export profile key=clear folder=\"" + dir + L"\"", &ec, 60000);
        for (auto& fd : ListDir(dir)) if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) count++;
        if (count) { Log(Fmt(L"  Wi-Fi: %d profile(s) exported with passwords.", count)); return; }
    }
    wstring base = j.srcDrive + L"\\ProgramData\\Microsoft\\Wlansvc\\Profiles\\Interfaces";
    int idx = 0;
    for (auto& itf : ListDir(base)) {
        if (!(itf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        idx++;
        for (auto& fd : ListDir(Join(base, itf.cFileName))) {
            wstring n = fd.cFileName;
            if (n.size() < 4 || !IEquals(n.substr(n.size() - 4), L".xml")) continue;
            if (CopyFileW(LongP(Join(Join(base, itf.cFileName), n)).c_str(), LongP(Join(dir, Fmt(L"if%d_", idx) + n)).c_str(), FALSE)) count++;
        }
    }
    Log(Fmt(L"  Wi-Fi: %d profile(s) copied (offline: passwords are encrypted per-PC and must be re-typed).", count));
}
static DWORD WINAPI BackupThread(LPVOID p) {
    BackupJob* j = (BackupJob*)p;
    wstring pkg = Join(j->destRoot, L"PCMigrate_" + TimeStamp(true));
    bool ok = MakeDirs(pkg);
    if (!ok) { Log(L"ERROR: cannot create package folder " + pkg + L" : " + ErrText(GetLastError())); PostMessageW(g_hwnd, WM_APP_DONE, 0, 0); delete j; return 0; }
    OpenLogFile(Join(pkg, L"backup.log"));
    Log(L"===== BACKUP started " + TimeStamp(false) + L" =====");
    {
        wchar_t fs[32] = L""; wstring root = j->destRoot.substr(0, 2) + L"\\";
        if (GetVolumeInformationW(root.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fs, 32) && _wcsicmp(fs, L"NTFS") && _wcsicmp(fs, L"exFAT") && _wcsicmp(fs, L"ReFS"))
            Log(wstring(L"WARNING: destination is ") + fs + L" - files bigger than 4 GB cannot be copied. Prefer an NTFS or exFAT disk.");
    }
    Log(L"Source Windows: " + j->srcDrive + (j->live ? L" (running system)" : L" (offline)") + L"   Package: " + pkg);

    vector<CopyTask> tasks;
    std::map<wstring, Program> progs;
    vector<wstring> userNames;

    const bool doApps = !j->apps.empty();
    // ---- machine hives (SOFTWARE for programs, SYSTEM for services)
    HKEY soft = nullptr; bool softLoaded = false;
    wstring softShort = L"HKLM\\SOFTWARE";
    Repl softRepl;
    if ((j->programs || doApps) && !g_cancel) {
        Log(L"Reading installed programs ...");
        Report(0, L"Reading installed programs ...", true);
        if (j->live) RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE", 0, KEY_READ | KEY_WOW64_64KEY, &soft);
        else {
            LONG r = RegLoadKeyW(HKEY_LOCAL_MACHINE, L"PCMIG_SOFT", (j->srcDrive + L"\\Windows\\System32\\config\\SOFTWARE").c_str());
            if (r == ERROR_SUCCESS) { softLoaded = true; RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"PCMIG_SOFT", 0, KEY_READ, &soft); }
            else Log(L"  ! cannot load SOFTWARE hive: " + ErrText(r));
            softShort = L"HKLM\\PCMIG_SOFT";
            softRepl.push_back({L"[HKEY_LOCAL_MACHINE\\PCMIG_SOFT", L"[HKEY_LOCAL_MACHINE\\SOFTWARE"});
        }
        if (soft && j->programs) {
            EnumUninstall(soft, L"Microsoft\\Windows\\CurrentVersion\\Uninstall", L"machine", progs);
            EnumUninstall(soft, L"WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"machine-x86", progs);
        }
    }
    HKEY sys = nullptr; bool sysLoaded = false;
    wstring sysShort = L"HKLM\\SYSTEM\\CurrentControlSet";
    Repl sysRepl;
    if (doApps && !g_cancel) {
        if (j->live) RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet", 0, KEY_READ, &sys);
        else {
            LONG r = RegLoadKeyW(HKEY_LOCAL_MACHINE, L"PCMIG_SYS", (j->srcDrive + L"\\Windows\\System32\\config\\SYSTEM").c_str());
            if (r == ERROR_SUCCESS) {
                sysLoaded = true;
                HKEY root;
                DWORD cs = 1;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"PCMIG_SYS", 0, KEY_READ, &root) == ERROR_SUCCESS) { cs = RegDword(root, L"Select", L"Current", 1); RegCloseKey(root); }
                wstring csName = Fmt(L"ControlSet%03lu", cs);
                RegOpenKeyExW(HKEY_LOCAL_MACHINE, (L"PCMIG_SYS\\" + csName).c_str(), 0, KEY_READ, &sys);
                sysShort = L"HKLM\\PCMIG_SYS\\" + csName;
                sysRepl.push_back({L"[HKEY_LOCAL_MACHINE\\PCMIG_SYS\\" + csName, L"[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet"});
            } else Log(L"  ! cannot load SYSTEM hive (services will not be transferred): " + ErrText(r));
        }
    }
    for (auto& a : j->apps) MakeDirs(pkg + L"\\Apps\\" + a.pkgName);

    // ---- per-user
    int ui = 0;
    for (auto& prof : j->profiles) {
        if (g_cancel) break;
        wstring uname = FileName(prof);
        userNames.push_back(uname);
        wstring udir = pkg + L"\\Users\\" + uname;
        MakeDirs(udir);
        Log(L"User: " + uname);
        if (j->files) {
            for (auto& f : kFolders) {
                wstring s = Join(prof, f.name);
                if (IsDir(s)) tasks.push_back({s, udir + L"\\Files\\" + f.name, false, false});
            }
            for (auto f : kOneDriveFolders) {          // OneDrive-redirected folders
                wstring s = prof + L"\\OneDrive\\" + f;
                if (IsDir(s)) tasks.push_back({s, udir + L"\\Files\\" + f, false, false});
            }
        }
        if (j->browsers) {
            for (auto& b : kBrowsers) {
                wstring s = Join(prof, b.rel);
                if (IsDir(s)) { tasks.push_back({s, udir + L"\\Browsers\\" + b.name, false, true}); Log(wstring(L"  browser found: ") + b.name); }
            }
            if (j->live) for (auto& b : kBrowsers) if (IsProcessRunning(b.exe)) Log(wstring(L"  ! ") + b.exe + L" is running - close it for a consistent copy.");
        }
        if (j->settings || j->programs || doApps) {
            Hive h;
            if (OpenUserHive(prof, Fmt(L"PCMIG_U%d", ++ui), false, h)) {
                if (j->settings) {
                    wstring sdir = udir + L"\\Settings";
                    MakeDirs(sdir);
                    int n = 0, k = 0;
                    for (auto key : kUserRegKeys) {
                        if (ExportUserKey(h, key, Join(sdir, Fmt(L"%02d.reg", ++k)))) n++;
                    }
                    Log(Fmt(L"  settings: %d registry key(s) exported.", n));
                    // wallpaper
                    wstring wp = Join(prof, WALLPAPER_REL);
                    if (IsFile(wp)) {
                        tasks.push_back({wp, sdir + L"\\wallpaper.jpg", true, false});
                        wstring ini = Join(sdir, L"wallpaper.ini");
                        WritePrivateProfileStringW(L"Wallpaper", L"WallpaperStyle", RegStr(h.base, L"Control Panel\\Desktop", L"WallpaperStyle").c_str(), ini.c_str());
                        WritePrivateProfileStringW(L"Wallpaper", L"TileWallpaper", RegStr(h.base, L"Control Panel\\Desktop", L"TileWallpaper").c_str(), ini.c_str());
                    }
                    wstring qa = Join(prof, QUICK_ACCESS_REL);
                    if (IsFile(qa)) tasks.push_back({qa, sdir + L"\\quickaccess.automaticDestinations-ms", true, false});
                }
                if (j->programs) EnumUninstall(h.base, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"user", progs);
            }
            // ---- per-user part of transferred programs: AppData, user Start Menu, HKCU keys
            for (auto& a : j->apps) {
                if (g_cancel) break;
                if (a.scope == L"user" && !IEquals(a.owner, prof)) continue;
                wstring ud = pkg + L"\\Apps\\" + a.pkgName + L"\\Users\\" + uname;
                AppNames nm = Candidates(a);
                std::set<wstring> seenDirs;
                const wchar_t* areas[][2] = {{L"AppData\\Roaming", L"Roaming"}, {L"AppData\\Local", L"Local"}, {L"AppData\\LocalLow", L"LocalLow"}};
                int nd = 0;
                for (auto& c : nm.cands) for (auto& ar : areas) {
                    wstring sdir = Join(Join(prof, ar[0]), c);
                    if (!IsDir(sdir) || !seenDirs.insert(Lower(sdir)).second) continue;
                    if (!a.srcDir.empty() && (IStarts(sdir + L"\\", a.srcDir + L"\\") || IStarts(a.srcDir + L"\\", sdir + L"\\"))) continue;
                    tasks.push_back({sdir, ud + L"\\" + ar[1] + L"\\" + c, false, true});
                    nd++;
                }
                StartMenuTasks(Join(prof, L"AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs"), ud + L"\\StartMenu", a, nm, tasks);
                int nr = 0;
                if (h.base) {
                    MakeDirs(ud + L"\\Reg");
                    std::set<wstring> seenKeys;
                    for (auto& c : nm.cands) {
                        wstring sub = L"Software\\" + c;
                        if (!seenKeys.insert(Lower(sub)).second) continue;
                        if (ExportUserKey(h, sub, ud + Fmt(L"\\Reg\\%02d.reg", nr + 1))) nr++;
                    }
                    if (a.scope == L"user" && ExportUserKey(h, a.uninstallSub, ud + L"\\Reg\\uninstall.reg")) nr++;
                }
                MakeDirs(ud);
                Log(Fmt(L"  program data '%ls': %d folder(s), %d registry key(s)", a.name.c_str(), nd, nr));
            }
            if (h.base || h.loaded) CloseHive(h);
        }
    }

    // ---- machine part of transferred programs: install folder, ProgramData, Start Menu, HKLM keys, services
    for (auto& a : j->apps) {
        if (g_cancel) break;
        wstring adir = pkg + L"\\Apps\\" + a.pkgName;
        AppNames nm = Candidates(a);
        Log(L"Program: " + a.name + (a.installDir.empty() ? L"  (data only)" : L"  [" + a.installDir + L"]"));
        if (!a.srcDir.empty() && IsDir(a.srcDir)) tasks.push_back({a.srcDir, adir + L"\\Files", false, false});
        std::set<wstring> seenDirs;
        for (auto& c : nm.cands) {
            wstring sdir = j->srcDrive + L"\\ProgramData\\" + c;
            if (!IsDir(sdir) || !seenDirs.insert(Lower(sdir)).second) continue;
            if (!a.srcDir.empty() && (IStarts(sdir + L"\\", a.srcDir + L"\\") || IStarts(a.srcDir + L"\\", sdir + L"\\"))) continue;
            tasks.push_back({sdir, adir + L"\\ProgramData\\" + c, false, true});
        }
        StartMenuTasks(j->srcDrive + L"\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs", adir + L"\\StartMenu", a, nm, tasks);
        int nr = 0, nsvc = 0, ndrv = 0;
        if (soft && a.scope != L"user") {
            MakeDirs(adir + L"\\Reg");
            std::set<wstring> seenKeys;
            auto exp = [&](const wstring& sub) {
                if (!seenKeys.insert(Lower(sub)).second) return;
                if (ExportKey(soft, sub, softShort, softRepl, adir + Fmt(L"\\Reg\\m%02d.reg", nr + 1))) nr++;
            };
            exp(a.uninstallSub);
            for (auto& c : nm.cands) { exp(c); exp(L"WOW6432Node\\" + c); }
            // App Paths (lets "Run" find the exe)
            if (!a.installDir.empty()) {
                HKEY ap;
                const wchar_t* apSub = L"Microsoft\\Windows\\CurrentVersion\\App Paths";
                if (RegOpenKeyExW(soft, apSub, 0, KEY_READ, &ap) == ERROR_SUCCESS) {
                    wchar_t kn[260];
                    for (DWORD i = 0;; ++i) {
                        DWORD n = 260;
                        if (RegEnumKeyExW(ap, i, kn, &n, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
                        wstring exe = ExeFromCommand(RegStr(ap, kn, nullptr));
                        if (IStarts(exe, a.installDir + L"\\")) exp(wstring(apSub) + L"\\" + kn);
                    }
                    RegCloseKey(ap);
                }
            }
        }
        if (sys && !a.installDir.empty()) {
            HKEY sv;
            if (RegOpenKeyExW(sys, L"Services", 0, KEY_READ, &sv) == ERROR_SUCCESS) {
                wstring dirL = Lower(a.installDir) + L"\\";
                wchar_t kn[260];
                for (DWORD i = 0;; ++i) {
                    DWORD n = 260;
                    if (RegEnumKeyExW(sv, i, kn, &n, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
                    wstring img = Lower(RegStr(sv, kn, L"ImagePath"));
                    if (img.find(dirL) == wstring::npos) continue;
                    DWORD type = RegDword(sv, kn, L"Type");
                    if ((type & 0x0F) && !(type & 0x30)) { ndrv++; Log(wstring(L"  ! kernel driver not transferable: ") + kn); continue; }
                    MakeDirs(adir + L"\\Reg");
                    if (ExportKey(sv, kn, sysShort + L"\\Services", sysRepl, adir + L"\\Reg\\svc_" + SafeName(kn) + L".reg")) { nsvc++; Log(wstring(L"  service: ") + kn); }
                }
                RegCloseKey(sv);
            }
        }
        wstring ini = adir + L"\\app.ini";
        WritePrivateProfileStringW(L"App", L"Name", a.name.c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"Version", a.version.c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"Publisher", a.publisher.c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"Scope", a.scope.c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"InstallDir", a.installDir.c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"SourceUser", a.owner.empty() ? L"" : FileName(a.owner).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"Services", Fmt(L"%d", nsvc).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"Drivers", Fmt(L"%d", ndrv).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"App", L"Note", a.note.c_str(), ini.c_str());
        Log(Fmt(L"  %d machine registry key(s), %d service(s)%ls", nr, nsvc, ndrv ? L"  - has drivers: reinstall recommended" : L""));
    }
    if (soft) RegCloseKey(soft);
    if (softLoaded) { Hive tmp; tmp.loaded = true; tmp.mount = L"PCMIG_SOFT"; CloseHive(tmp, HKEY_LOCAL_MACHINE); }
    if (sys) RegCloseKey(sys);
    if (sysLoaded) { Hive tmp; tmp.loaded = true; tmp.mount = L"PCMIG_SYS"; CloseHive(tmp, HKEY_LOCAL_MACHINE); }

    if (j->settings && !g_cancel) BackupWiFi(*j, pkg);

    if (j->programs) {
        wstring txt = L"# PC Migrate program list - Name<TAB>Version<TAB>Publisher<TAB>Scope\r\n";
        vector<Program> v; for (auto& kv : progs) v.push_back(kv.second);
        std::sort(v.begin(), v.end(), [](const Program& a, const Program& b) { return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
        for (auto& pr : v) txt += pr.name + L"\t" + pr.version + L"\t" + pr.publisher + L"\t" + pr.scope + L"\r\n";
        WriteUtf8(Join(pkg, L"programs.txt"), txt, true);
        Log(Fmt(L"Programs: %d found, list saved to programs.txt", (int)v.size()));
    }

    // manifest
    wstring man = Join(pkg, L"manifest.ini");
    wstring users; for (auto& u : userNames) users += (users.empty() ? L"" : L"|") + u;
    WritePrivateProfileStringW(L"Package", L"Tool", APP_TITLE, man.c_str());
    WritePrivateProfileStringW(L"Package", L"Created", TimeStamp(false).c_str(), man.c_str());
    WritePrivateProfileStringW(L"Package", L"SourceDrive", j->srcDrive.c_str(), man.c_str());
    WritePrivateProfileStringW(L"Package", L"Users", users.c_str(), man.c_str());

    bool copied = !g_cancel && RunCopyTasks(tasks, j->destRoot);
    if (g_cancel) Log(L"===== BACKUP CANCELLED =====");
    else if (!copied) Log(L"===== BACKUP FAILED =====");
    else {
        Log(L"===== BACKUP FINISHED " + TimeStamp(false) + L" =====");
        Log(L"Package ready: " + pkg);
        Log(L"Next: plug this disk into the NEW PC, run PCMigrate.exe there, tab 2 'Restore'.");
    }
    CloseLogFile();
    PostMessageW(g_hwnd, WM_APP_DONE, 0, 0);
    delete j;
    return 0;
}

// ---------------------------------------------------------------------
//  Restore job
// ---------------------------------------------------------------------
struct RestoreJob { wstring pkg, srcUser, tgtProfile; bool files = true, browsers = true, settings = true; };

static wstring KnownFolder(const KNOWNFOLDERID& id) {
    PWSTR p = nullptr; wstring r;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) r = p;
    if (p) CoTaskMemFree(p);
    return r;
}
static void RestoreWiFi(const wstring& pkg) {
    wstring dir = pkg + L"\\System\\WiFi";
    if (!IsDir(dir)) return;
    if (g_isPE) { Log(L"Wi-Fi: skipped (Wi-Fi profiles can only be imported from a running Windows)."); return; }
    wchar_t tmpDir[MAX_PATH]; GetTempPathW(MAX_PATH, tmpDir);
    int ok = 0, total = 0;
    for (auto& fd : ListDir(dir)) {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        total++;
        wstring f = Join(dir, fd.cFileName), use = f, txt;
        if (ReadText(f, txt) && txt.find(L"<protected>true</protected>") != wstring::npos) {
            size_t a = txt.find(L"<sharedKey>"), b = txt.find(L"</sharedKey>");
            if (a != wstring::npos && b != wstring::npos) {
                txt.erase(a, b + 12 - a);
                use = Join(tmpDir, L"pcmig_wifi.xml");
                WriteUtf8(use, txt, false);
            }
        }
        DWORD ec = 1;
        if (RunProcess(L"\"" + SysExe(L"netsh.exe") + L"\" wlan add profile filename=\"" + use + L"\" user=all", &ec, 30000) && ec == 0) ok++;
        else Log(wstring(L"  ! Wi-Fi profile not added: ") + fd.cFileName);
    }
    Log(Fmt(L"Wi-Fi: %d / %d profile(s) imported.", ok, total));
}
static DWORD WINAPI RestoreThread(LPVOID p) {
    RestoreJob* j = (RestoreJob*)p;
    OpenLogFile(Join(j->pkg, L"restore.log"));
    Log(L"===== RESTORE started " + TimeStamp(false) + L" =====");
    Log(L"Package: " + j->pkg + L"   user: " + j->srcUser + L"  ->  " + j->tgtProfile);
    wstring src = j->pkg + L"\\Users\\" + j->srcUser;
    wstring tgt = j->tgtProfile;
    bool liveUser = !g_isPE && IEquals(StripSlash(tgt), StripSlash(Env(L"USERPROFILE")));
    bool liveSystem = !g_isPE && IEquals(DriveOf(tgt), DriveOf(g_sysDrive));
    vector<CopyTask> tasks;

    if (j->files && IsDir(src + L"\\Files")) {
        for (auto& fd : ListDir(src + L"\\Files")) {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            wstring dst;
            if (liveUser) for (auto& f : kFolders) if (IEquals(f.name, fd.cFileName)) dst = KnownFolder(*f.id);   // honours OneDrive redirection
            if (dst.empty()) dst = Join(tgt, fd.cFileName);
            tasks.push_back({src + L"\\Files\\" + fd.cFileName, dst, false, false});
        }
    }
    if (j->browsers && IsDir(src + L"\\Browsers")) {
        for (auto& b : kBrowsers) {
            wstring s = src + L"\\Browsers\\" + b.name;
            if (!IsDir(s)) continue;
            if (liveSystem && IsProcessRunning(b.exe)) {
                Log(wstring(L"  ! ") + b.name + L" skipped: " + b.exe + L" is running. Close it (for Edge also disable 'Startup boost') and restore again.");
                continue;
            }
            wstring d = Join(tgt, b.rel);
            if (IsDir(d)) {
                wstring bak = d + L".bak_" + TimeStamp(true);
                if (MoveFileExW(LongP(d).c_str(), LongP(bak).c_str(), 0)) Log(L"  existing profile kept as " + bak);
            }
            tasks.push_back({s, d, false, false});
            Log(wstring(L"  browser: ") + b.name);
        }
        Log(L"  note: saved passwords/cookies of Chrome/Edge/Brave/Opera are encrypted per-PC; use browser sync or password export for those.");
    }
    bool copied = RunCopyTasks(tasks, L"");

    if (j->settings && !g_cancel && IsDir(src + L"\\Settings")) {
        Log(L"Applying Windows settings ...");
        wstring sdir = src + L"\\Settings";
        Hive h;
        if (OpenUserHive(tgt, L"PCMIG_TGT", true, h)) {
            int ok = 0, total = 0;
            for (auto& fd : ListDir(sdir)) {
                wstring n = fd.cFileName;
                if (n.size() > 4 && IEquals(n.substr(n.size() - 4), L".reg")) { total++; if (ImportUserReg(h, Join(sdir, n))) ok++; }
            }
            Log(Fmt(L"  registry: %d / %d key file(s) imported.", ok, total));
            // wallpaper
            wstring wpSrc = Join(sdir, L"wallpaper.jpg");
            if (IsFile(wpSrc)) {
                wstring wpDir = Join(tgt, L"AppData\\Roaming\\PCMigrate");
                MakeDirs(wpDir);
                wstring wpDst = Join(wpDir, L"wallpaper.jpg");
                if (CopyFileW(LongP(wpSrc).c_str(), LongP(wpDst).c_str(), FALSE)) {
                    wstring regPath = liveSystem ? wpDst : (L"C:" + wpDst.substr(2));   // offline target will boot as C:
                    HKEY k;
                    if (RegOpenKeyExW(h.base, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
                        RegSetValueExW(k, L"Wallpaper", 0, REG_SZ, (const BYTE*)regPath.c_str(), (DWORD)((regPath.size() + 1) * 2));
                        wstring ini = Join(sdir, L"wallpaper.ini");
                        wchar_t v[32];
                        GetPrivateProfileStringW(L"Wallpaper", L"WallpaperStyle", L"", v, 32, ini.c_str());
                        if (v[0]) RegSetValueExW(k, L"WallpaperStyle", 0, REG_SZ, (const BYTE*)v, (DWORD)((wcslen(v) + 1) * 2));
                        GetPrivateProfileStringW(L"Wallpaper", L"TileWallpaper", L"", v, 32, ini.c_str());
                        if (v[0]) RegSetValueExW(k, L"TileWallpaper", 0, REG_SZ, (const BYTE*)v, (DWORD)((wcslen(v) + 1) * 2));
                        RegCloseKey(k);
                    }
                    if (liveUser) SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)wpDst.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                    Log(L"  wallpaper restored.");
                }
            }
            CloseHive(h);
        }
        wstring qa = Join(sdir, L"quickaccess.automaticDestinations-ms");
        if (IsFile(qa)) {
            wstring d = Join(tgt, QUICK_ACCESS_REL);
            size_t pp = d.find_last_of(L'\\'); MakeDirs(d.substr(0, pp));
            if (CopyFileW(LongP(qa).c_str(), LongP(d).c_str(), FALSE)) Log(L"  Quick Access pins restored.");
        }
        RestoreWiFi(j->pkg);
    }

    if (g_cancel) Log(L"===== RESTORE CANCELLED =====");
    else {
        Log(L"===== RESTORE FINISHED " + TimeStamp(false) + L" =====");
        if (copied) Log(L"Sign out and sign in again (or restart) so all settings take effect.");
        Log(L"Programs: use the list below to reinstall them with winget.");
    }
    CloseLogFile();
    PostMessageW(g_hwnd, WM_APP_DONE, 0, 0);
    delete j;
    return 0;
}

// ---------------------------------------------------------------------
//  winget install job
// ---------------------------------------------------------------------
struct InstallJob { vector<ProgItem> items; wstring pkg, tgtProfile, srcUser; bool preferWinget = false; };

// context shared by all program transfers of one job
struct TransferCtx {
    wstring pkg, tgtProfile, srcUser, newUser, tgtSysDrive;
    bool liveSystem = false;
    Hive user;                         // target user hive
    bool userOk = false;
    bool machineOpen = false;          // offline target hives loaded?
    Repl machineRepl;                  // root remapping for offline target
    int services = 0;
};
static void OpenTargetMachine(TransferCtx& c) {
    if (c.machineOpen || c.liveSystem) return;
    c.machineOpen = true;
    LONG r = RegLoadKeyW(HKEY_LOCAL_MACHINE, L"PCMIG_TSOFT", (c.tgtSysDrive + L"\\Windows\\System32\\config\\SOFTWARE").c_str());
    if (r == ERROR_SUCCESS) c.machineRepl.push_back({L"[HKEY_LOCAL_MACHINE\\SOFTWARE", L"[HKEY_LOCAL_MACHINE\\PCMIG_TSOFT"});
    else Log(L"  ! cannot load target SOFTWARE hive: " + ErrText(r));
    r = RegLoadKeyW(HKEY_LOCAL_MACHINE, L"PCMIG_TSYS", (c.tgtSysDrive + L"\\Windows\\System32\\config\\SYSTEM").c_str());
    if (r == ERROR_SUCCESS) {
        HKEY root; DWORD cs = 1;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"PCMIG_TSYS", 0, KEY_READ, &root) == ERROR_SUCCESS) { cs = RegDword(root, L"Select", L"Current", 1); RegCloseKey(root); }
        c.machineRepl.push_back({L"[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet", Fmt(L"[HKEY_LOCAL_MACHINE\\PCMIG_TSYS\\ControlSet%03lu", cs)});
    } else Log(L"  ! cannot load target SYSTEM hive: " + ErrText(r));
}
static void CloseTargetMachine(TransferCtx& c) {
    if (!c.machineOpen || c.liveSystem) return;
    Hive a; a.loaded = true; a.mount = L"PCMIG_TSOFT"; CloseHive(a, HKEY_LOCAL_MACHINE);
    Hive b; b.loaded = true; b.mount = L"PCMIG_TSYS"; CloseHive(b, HKEY_LOCAL_MACHINE);
    c.machineOpen = false;
}
// full = copy program files + HKLM + services ; otherwise only the user's data / settings
static bool TransferApp(const ProgItem& it, bool full, TransferCtx& c) {
    wstring adir = it.pkgDir, ini = Join(adir, L"app.ini");
    wchar_t buf[1024];
    GetPrivateProfileStringW(L"App", L"InstallDir", L"", buf, 1024, ini.c_str());
    wstring installDir = buf;
    GetPrivateProfileStringW(L"App", L"SourceUser", L"", buf, 1024, ini.c_str());
    wstring appUser = buf;
    // which captured user data to use: the selected package user, else the program's owner, else the first one
    wstring ud = adir + L"\\Users\\" + c.srcUser, dataUser = c.srcUser;
    if (!IsDir(ud)) {
        dataUser = !appUser.empty() && IsDir(adir + L"\\Users\\" + appUser) ? appUser : L"";
        if (dataUser.empty()) for (auto& fd : ListDir(adir + L"\\Users")) if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { dataUser = fd.cFileName; break; }
        ud = adir + L"\\Users\\" + dataUser;
    }
    wstring oldUser = !appUser.empty() ? appUser : dataUser;
    auto mapPath = [&](const wstring& logical) -> wstring {
        if (logical.size() < 3) return logical;
        wstring up = logical.substr(0, 2) + L"\\Users\\" + oldUser;
        if (!oldUser.empty() && (IStarts(logical, up + L"\\") || IEquals(logical, up))) return c.tgtProfile + logical.substr(up.size());
        return c.tgtSysDrive + logical.substr(2);
    };
    Log(L"Transferring " + it.name + (full ? L"  (files + registry + data)" : L"  (data / settings only)"));
    vector<CopyTask> tasks;
    if (full) {
        if (!installDir.empty() && IsDir(adir + L"\\Files")) {
            wstring dst = mapPath(installDir);
            tasks.push_back({adir + L"\\Files", dst, false, false});
            Log(L"  -> " + dst);
        }
        for (auto& fd : ListDir(adir + L"\\ProgramData"))
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                tasks.push_back({adir + L"\\ProgramData\\" + fd.cFileName, c.tgtSysDrive + L"\\ProgramData\\" + fd.cFileName, false, false});
        for (auto& fd : ListDir(adir + L"\\StartMenu"))
            tasks.push_back({adir + L"\\StartMenu\\" + fd.cFileName, c.tgtSysDrive + L"\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\" + fd.cFileName,
                             !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY), false});
    }
    if (!dataUser.empty()) {
        const wchar_t* areas[] = {L"Roaming", L"Local", L"LocalLow"};
        for (auto ar : areas)
            for (auto& fd : ListDir(ud + L"\\" + ar))
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    tasks.push_back({ud + L"\\" + ar + L"\\" + fd.cFileName, c.tgtProfile + L"\\AppData\\" + ar + L"\\" + fd.cFileName, false, false});
        for (auto& fd : ListDir(ud + L"\\StartMenu"))
            tasks.push_back({ud + L"\\StartMenu\\" + fd.cFileName, c.tgtProfile + L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\" + fd.cFileName,
                             !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY), false});
    }
    bool ok = RunCopyTasks(tasks, L"");
    if (g_cancel) return false;

    Repl userPaths = UserPathRepl(oldUser, c.newUser);
    int nm = 0, nu = 0, fail = 0;
    if (full && IsDir(adir + L"\\Reg")) {
        OpenTargetMachine(c);
        Repl r = c.machineRepl; r.insert(r.end(), userPaths.begin(), userPaths.end());
        for (auto& fd : ListDir(adir + L"\\Reg")) {
            wstring n = fd.cFileName;
            if (n.size() < 5 || !IEquals(n.substr(n.size() - 4), L".reg")) continue;
            if (ImportReg(adir + L"\\Reg\\" + n, r)) { nm++; if (IStarts(n, L"svc_")) c.services++; } else fail++;
        }
    }
    if (!dataUser.empty() && IsDir(ud + L"\\Reg") && c.userOk) {
        Repl r = {{L"[HKEY_CURRENT_USER", L"[" + c.user.longPath}};
        r.insert(r.end(), userPaths.begin(), userPaths.end());
        for (auto& fd : ListDir(ud + L"\\Reg")) {
            wstring n = fd.cFileName;
            if (n.size() < 5 || !IEquals(n.substr(n.size() - 4), L".reg")) continue;
            if (ImportReg(ud + L"\\Reg\\" + n, r)) nu++; else fail++;
        }
    }
    Log(Fmt(L"  registry: %d machine + %d user key file(s) imported%ls", nm, nu, fail ? Fmt(L", %d failed", fail).c_str() : L""));
    GetPrivateProfileStringW(L"App", L"Drivers", L"0", buf, 1024, ini.c_str());
    if (full && _wtoi(buf) > 0) Log(L"  ! this program uses drivers - if it does not work, reinstall it (your data is already restored).");
    return ok;
}
static bool WingetInstall(const ProgItem& it) {
    wstring cmd = L"winget install ";
    cmd += it.id.empty() ? (L"--name \"" + CleanName(it.name) + L"\"") : (L"--id " + it.id + L" -e");
    cmd += L" --silent --accept-package-agreements --accept-source-agreements --disable-interactivity";
    Log(L"winget: installing " + it.name + L"  (" + (it.id.empty() ? L"by name" : it.id) + L") ...");
    DWORD ec = 1;
    RunProcess(cmd, &ec);
    if (ec == 0) { Log(L"  OK"); return true; }
    if (ec == 0x8A15002B || ec == 0x8A150061) { Log(L"  already installed"); return true; }
    if (ec == 0x8A150014) Log(L"  ! no package found in winget");
    else Log(Fmt(L"  ! failed (code 0x%08lX)", ec));
    return false;
}
static DWORD WINAPI InstallThread(LPVOID p) {
    InstallJob* j = (InstallJob*)p;
    if (!j->pkg.empty()) OpenLogFile(Join(j->pkg, L"restore.log"));
    Log(L"===== PROGRAMS started " + TimeStamp(false) + L" =====");
    DWORD ec = 0;
    bool wingetOk = !g_isPE && RunProcess(L"winget --version", &ec, 30000) && ec == 0;
    if (!wingetOk) Log(g_isPE ? L"WinPE: winget is not available - only programs saved in the package can be transferred."
                              : L"winget not found (install 'App Installer' from Microsoft Store) - only programs saved in the package can be transferred.");
    TransferCtx c;
    c.pkg = j->pkg; c.tgtProfile = j->tgtProfile; c.srcUser = j->srcUser; c.newUser = FileName(j->tgtProfile);
    c.liveSystem = !g_isPE && IEquals(DriveOf(j->tgtProfile), DriveOf(g_sysDrive));
    c.tgtSysDrive = c.liveSystem ? g_sysDrive : j->tgtProfile.substr(0, 2);
    bool anyPkg = false;
    for (auto& it : j->items) if (!it.pkgDir.empty()) anyPkg = true;
    if (anyPkg) c.userOk = OpenUserHive(j->tgtProfile, L"PCMIG_TGT", true, c.user);

    int n = 0, ok = 0, total = (int)j->items.size();
    for (auto& it : j->items) {
        if (g_cancel) break;
        n++;
        Report(0, Fmt(L"Program %d/%d : %ls", n, total, it.name.c_str()), true);
        bool usePkg = !it.pkgDir.empty();
        bool viaWinget = wingetOk && (!usePkg || (j->preferWinget && !it.id.empty()));
        bool r = false;
        if (viaWinget) {
            r = WingetInstall(it);
            if (usePkg) TransferApp(it, false, c);      // reinstall clean, then bring back its data
        } else if (usePkg) {
            r = TransferApp(it, true, c);
        } else {
            Log(L"Skipped " + it.name + L" (needs winget)");
        }
        if (r) ok++;
    }
    if (c.userOk) CloseHive(c.user);
    CloseTargetMachine(c);
    Log(Fmt(L"===== PROGRAMS finished: %d / %d OK =====", ok, n));
    if (c.services) Log(L"Services were registered: RESTART the PC so they start.");
    if (anyPkg) Log(L"Transferred programs appear in the Start menu. Sign out / restart if a shortcut or setting is missing.");
    Report(1000, L"Done", true);
    CloseLogFile();
    PostMessageW(g_hwnd, WM_APP_DONE, 0, 0);
    delete j;
    return 0;
}

// =====================================================================
//  UI
// =====================================================================
static int S(int v) { return (int)(v * g_scale + 0.5); }
static HWND Ctl(const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id, vector<HWND>* page, DWORD ex = 0) {
    HWND c = CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style, S(x), S(y), S(w), S(h), g_hwnd, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    if (page) page->push_back(c);
    return c;
}
static void AddColumn(HWND lv, int idx, const wchar_t* text, int w) {
    LVCOLUMNW c{}; c.mask = LVCF_TEXT | LVCF_WIDTH; c.pszText = (LPWSTR)text; c.cx = S(w);
    ListView_InsertColumn(lv, idx, &c);
}
static void LvAdd(HWND lv, const vector<wstring>& cols, LPARAM param, bool checked) {
    LVITEMW it{}; it.mask = LVIF_TEXT | LVIF_PARAM; it.iItem = ListView_GetItemCount(lv); it.pszText = (LPWSTR)cols[0].c_str(); it.lParam = param;
    int i = ListView_InsertItem(lv, &it);
    for (size_t c = 1; c < cols.size(); ++c) ListView_SetItemText(lv, i, (int)c, (LPWSTR)cols[c].c_str());
    ListView_SetCheckState(lv, i, checked);
}
static wstring GetText(HWND h) {
    int n = GetWindowTextLengthW(h); wstring s(n, 0);
    if (n) GetWindowTextW(h, &s[0], n + 1);
    return Trim(s);
}
static void AppendLog(const wstring& s) {
    int len = GetWindowTextLengthW(g_log);
    if (len > 900000) { SendMessageW(g_log, EM_SETSEL, 0, len / 2); SendMessageW(g_log, EM_REPLACESEL, FALSE, (LPARAM)L""); len = GetWindowTextLengthW(g_log); }
    SendMessageW(g_log, EM_SETSEL, len, len);
    SendMessageW(g_log, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}
static void SetBusy(bool busy) {
    g_busy = busy;
    HWND toggle[] = {hBStart, hBRefresh, hBBrowse, hBSrc, hBChoose, hRStart, hRLoad, hRBrowse, hRInstall, g_tab};
    for (auto h : toggle) EnableWindow(h, !busy);
    EnableWindow(g_cancelBtn, busy);
    if (busy) { g_cancel = FALSE; SendMessageW(g_progress, PBM_SETPOS, 0, 0); }
}
static void ShowPage(int idx) {
    for (auto h : g_pageB) ShowWindow(h, idx == 0 ? SW_SHOW : SW_HIDE);
    for (auto h : g_pageR) ShowWindow(h, idx == 1 ? SW_SHOW : SW_HIDE);
}
static wstring BrowseFolder(const wchar_t* title) {
    BROWSEINFOW bi{}; bi.hwndOwner = g_hwnd; bi.lpszTitle = title; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return L"";
    wchar_t path[MAX_PATH] = L"";
    SHGetPathFromIDListW(pidl, path);
    CoTaskMemFree(pidl);
    return path;
}

// ---- backup page logic
static void FillUsers() {
    ListView_DeleteAllItems(hBUsers);
    int sel = (int)SendMessageW(hBSrc, CB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)g_installs.size()) return;
    for (auto& p : EnumProfiles(g_installs[sel].drive)) LvAdd(hBUsers, {FileName(p), p}, 0, true);
}
static wstring SuggestDest(const wstring& srcDrive) {
    wstring best; ULONGLONG bestFree = 0; bool bestRemovable = false;
    DWORD mask = GetLogicalDrives();
    for (int i = 2; i < 26; ++i) {
        if (!(mask & (1u << i))) continue;
        wstring root = Fmt(L"%lc:\\", L'A' + i);
        if (IEquals(root.substr(0, 2), srcDrive) || (g_isPE && IEquals(root.substr(0, 2), g_sysDrive))) continue;
        UINT t = GetDriveTypeW(root.c_str());
        if (t != DRIVE_FIXED && t != DRIVE_REMOVABLE) continue;
        bool hasWin = IsDir(root + L"Windows\\System32");
        if (hasWin) continue;
        ULONGLONG fs = FreeSpace(root);
        bool rem = t == DRIVE_REMOVABLE;
        bool better = best.empty() || (rem && !bestRemovable) || (rem == bestRemovable && fs > bestFree);
        if (better && fs > 0) { best = root; bestFree = fs; bestRemovable = rem; }
    }
    return best;
}
static void RefreshInstalls() {
    g_installs = DetectInstalls();
    SendMessageW(hBSrc, CB_RESETCONTENT, 0, 0);
    int liveIdx = -1;
    for (size_t i = 0; i < g_installs.size(); ++i) {
        SendMessageW(hBSrc, CB_ADDSTRING, 0, (LPARAM)g_installs[i].label.c_str());
        if (g_installs[i].live) liveIdx = (int)i;
    }
    // in WinPE prefer the first offline install, otherwise the running one
    int sel = g_installs.empty() ? -1 : (liveIdx >= 0 ? liveIdx : 0);
    SendMessageW(hBSrc, CB_SETCURSEL, sel, 0);
    FillUsers();
    if (sel >= 0 && GetText(hBDest).empty()) SetWindowTextW(hBDest, SuggestDest(g_installs[sel].drive).c_str());
    if (g_installs.empty()) Log(L"No Windows installation with user profiles found. (BitLocker drives must be unlocked first.)");
    else Log(Fmt(L"Found %d Windows installation(s).", (int)g_installs.size()));

    // restore targets
    g_tgtProfiles.clear();
    SendMessageW(hRTgt, CB_RESETCONTENT, 0, 0);
    wstring me = StripSlash(Env(L"USERPROFILE"));
    int meIdx = -1;
    for (auto& w : g_installs)
        for (auto& p : EnumProfiles(w.drive)) {
            if (!g_isPE && IEquals(p, me)) meIdx = (int)g_tgtProfiles.size();
            g_tgtProfiles.push_back(p);
            wstring lbl = p + ((!g_isPE && IEquals(p, me)) ? L"   (you - current user)" : L"");
            SendMessageW(hRTgt, CB_ADDSTRING, 0, (LPARAM)lbl.c_str());
        }
    SendMessageW(hRTgt, CB_SETCURSEL, meIdx >= 0 ? meIdx : 0, 0);
}
// ---- program chooser (transfer with files + data)
static vector<wstring> CheckedProfiles() {
    vector<wstring> v; wchar_t buf[MAX_PATH * 2];
    for (int i = 0, n = ListView_GetItemCount(hBUsers); i < n; ++i)
        if (ListView_GetCheckState(hBUsers, i)) { ListView_GetItemText(hBUsers, i, 1, buf, MAX_PATH * 2); v.push_back(buf); }
    return v;
}
static void UpdateAppsLabel() {
    int n = 0; for (auto& a : g_srcApps) if (a.selected) n++;
    SetWindowTextW(hBApps, Fmt(L"Programs with their files && data  (%d selected)", n).c_str());
}
static bool LoadSourceApps() {
    int sel = (int)SendMessageW(hBSrc, CB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)g_installs.size()) return false;
    const WinInstall& w = g_installs[sel];
    vector<wstring> profs = CheckedProfiles();
    wstring key = w.drive;
    for (auto& p : profs) key += L"|" + p;
    if (key == g_srcAppsKey) return true;
    std::set<wstring> prev;
    for (auto& a : g_srcApps) if (a.selected) prev.insert(Lower(a.name));
    HCURSOR old = SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    g_srcApps.clear();
    std::set<wstring> seen;
    HKEY soft = nullptr; bool loaded = false;
    if (w.live) RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE", 0, KEY_READ | KEY_WOW64_64KEY, &soft);
    else {
        LONG r = RegLoadKeyW(HKEY_LOCAL_MACHINE, L"PCMIG_SOFT", (w.drive + L"\\Windows\\System32\\config\\SOFTWARE").c_str());
        if (r == ERROR_SUCCESS) { loaded = true; RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"PCMIG_SOFT", 0, KEY_READ, &soft); }
        else Log(L"! cannot read programs of " + w.drive + L": " + ErrText(r));
    }
    if (soft) {
        EnumApps(soft, L"Microsoft\\Windows\\CurrentVersion\\Uninstall", L"machine", L"", w.drive, w.live, seen, g_srcApps);
        EnumApps(soft, L"WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"machine-x86", L"", w.drive, w.live, seen, g_srcApps);
        RegCloseKey(soft);
    }
    if (loaded) { Hive t; t.loaded = true; t.mount = L"PCMIG_SOFT"; CloseHive(t, HKEY_LOCAL_MACHINE); }
    int i = 0;
    for (auto& p : profs) {
        Hive h;
        if (OpenUserHive(p, Fmt(L"PCMIG_C%d", ++i), false, h)) {
            EnumApps(h.base, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"user", p, w.drive, w.live, seen, g_srcApps);
            CloseHive(h);
        }
    }
    std::sort(g_srcApps.begin(), g_srcApps.end(), [](const AppInfo& a, const AppInfo& b) { return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
    for (auto& a : g_srcApps) a.selected = prev.count(Lower(a.name)) > 0;
    g_srcAppsKey = key;
    SetCursor(old);
    UpdateAppsLabel();
    return true;
}
static HWND g_chooser, g_chLv;
static bool g_chDone;
static LRESULT CALLBACK ChooserProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_CREATE: {
        auto mk = [&](const wchar_t* cls, const wchar_t* t, DWORD st, int x, int y, int w, int hh, int id, DWORD ex = 0) {
            HWND c = CreateWindowExW(ex, cls, t, WS_CHILD | WS_VISIBLE | st, S(x), S(y), S(w), S(hh), h, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
            SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
            return c;
        };
        mk(L"STATIC", L"Select the programs to move WITH their files, registry, services, shortcuts and user data.\n"
                      L"Best for normal desktop programs. Antivirus, VPN, drivers and hardware-locked licences usually need a reinstall.",
           0, 12, 8, 816, 38, -1);
        g_chLv = mk(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP, 12, 50, 816, 400, 10, WS_EX_CLIENTEDGE);
        ListView_SetExtendedListViewStyle(g_chLv, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        AddColumn(g_chLv, 0, L"Program", 270); AddColumn(g_chLv, 1, L"Version", 90);
        AddColumn(g_chLv, 2, L"Size", 75); AddColumn(g_chLv, 3, L"Install folder / notes", 355);
        for (size_t i = 0; i < g_srcApps.size(); ++i) {
            auto& a = g_srcApps[i];
            wstring info = a.installDir.empty() ? a.note : a.installDir + (a.note.empty() ? L"" : L"   (" + a.note + L")");
            if (a.scope == L"user") info += L"   [user: " + FileName(a.owner) + L"]";
            LvAdd(g_chLv, {a.name, a.version, a.sizeKB ? SizeStr((ULONGLONG)a.sizeKB * 1024) : L"", info}, (LPARAM)i, a.selected);
        }
        mk(L"BUTTON", L"Select all", WS_TABSTOP, 12, 460, 100, 30, 11);
        mk(L"BUTTON", L"Select none", WS_TABSTOP, 120, 460, 100, 30, 12);
        mk(L"BUTTON", L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP, 628, 460, 96, 30, IDOK);
        mk(L"BUTTON", L"Cancel", WS_TABSTOP, 732, 460, 96, 30, IDCANCEL);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case 11: case 12:
            for (int i = 0, n = ListView_GetItemCount(g_chLv); i < n; ++i) ListView_SetCheckState(g_chLv, i, LOWORD(wp) == 11);
            return 0;
        case IDOK:
            for (int i = 0, n = ListView_GetItemCount(g_chLv); i < n; ++i) {
                LVITEMW it{}; it.mask = LVIF_PARAM; it.iItem = i; ListView_GetItem(g_chLv, &it);
                if (it.lParam >= 0 && it.lParam < (LPARAM)g_srcApps.size()) g_srcApps[it.lParam].selected = ListView_GetCheckState(g_chLv, i) != 0;
            }
            g_chDone = true; DestroyWindow(h); return 0;
        case IDCANCEL: g_chDone = true; DestroyWindow(h); return 0;
        }
        return 0;
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp; SetBkColor(dc, GetSysColor(COLOR_WINDOW)); SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    case WM_CLOSE: g_chDone = true; DestroyWindow(h); return 0;
    }
    return DefWindowProcW(h, m, wp, lp);
}
static void ShowChooser() {
    if (!LoadSourceApps()) { MessageBoxW(g_hwnd, L"Select the source Windows installation first.", APP_TITLE, MB_ICONWARNING); return; }
    if (g_srcApps.empty()) { MessageBoxW(g_hwnd, L"No programs found on the source Windows.", APP_TITLE, MB_ICONINFORMATION); return; }
    static bool reg = false;
    if (!reg) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = ChooserProc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"PCMigrateChooser";
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassExW(&wc); reg = true;
    }
    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    RECT r{0, 0, S(840), S(502)};
    AdjustWindowRectEx(&r, style, FALSE, WS_EX_DLGMODALFRAME);
    RECT pr; GetWindowRect(g_hwnd, &pr);
    int w = r.right - r.left, hh = r.bottom - r.top;
    int x = pr.left + ((pr.right - pr.left) - w) / 2, y = pr.top + ((pr.bottom - pr.top) - hh) / 2;
    g_chDone = false;
    EnableWindow(g_hwnd, FALSE);
    g_chooser = CreateWindowExW(WS_EX_DLGMODALFRAME, L"PCMigrateChooser", L"Choose programs to transfer with their data", style | WS_VISIBLE,
                                x < 0 ? 0 : x, y < 0 ? 0 : y, w, hh, g_hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
    MSG msg;
    while (!g_chDone && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(g_chooser, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    if (!g_chDone) PostQuitMessage((int)msg.wParam);
    EnableWindow(g_hwnd, TRUE);
    SetForegroundWindow(g_hwnd);
    int n = 0; for (auto& a : g_srcApps) if (a.selected) n++;
    SendMessageW(hBApps, BM_SETCHECK, n ? BST_CHECKED : BST_UNCHECKED, 0);
    UpdateAppsLabel();
}
static void StartBackup() {
    int sel = (int)SendMessageW(hBSrc, CB_GETCURSEL, 0, 0);
    if (sel < 0) { MessageBoxW(g_hwnd, L"Select the source Windows installation.", APP_TITLE, MB_ICONWARNING); return; }
    BackupJob* j = new BackupJob();
    j->srcDrive = g_installs[sel].drive; j->live = g_installs[sel].live;
    wchar_t buf[MAX_PATH * 2];
    for (int i = 0, n = ListView_GetItemCount(hBUsers); i < n; ++i)
        if (ListView_GetCheckState(hBUsers, i)) { ListView_GetItemText(hBUsers, i, 1, buf, MAX_PATH * 2); j->profiles.push_back(buf); }
    j->files = SendMessageW(hBFiles, BM_GETCHECK, 0, 0) == BST_CHECKED;
    j->browsers = SendMessageW(hBBrowsers, BM_GETCHECK, 0, 0) == BST_CHECKED;
    j->settings = SendMessageW(hBSettings, BM_GETCHECK, 0, 0) == BST_CHECKED;
    j->programs = SendMessageW(hBPrograms, BM_GETCHECK, 0, 0) == BST_CHECKED;
    j->destRoot = StripSlash(GetText(hBDest));
    bool wantApps = SendMessageW(hBApps, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (wantApps) {
        LoadSourceApps();
        std::set<wstring> used;
        for (auto& a : g_srcApps) if (a.selected) {
            AppInfo c = a;
            wstring base = SafeName(a.name), nm = base;
            for (int k = 2; used.count(Lower(nm)); ++k) nm = base + Fmt(L"_%d", k);
            used.insert(Lower(nm));
            c.pkgName = nm;
            j->apps.push_back(c);
        }
    }
    wstring err;
    if (j->profiles.empty() && (j->files || j->browsers || j->settings)) err = L"Select at least one user profile.";
    else if (!j->files && !j->browsers && !j->settings && !j->programs && j->apps.empty()) err = L"Select at least one thing to transfer.";
    else if (wantApps && j->apps.empty()) err = L"'Programs with their files & data' is checked but no program is selected.\nClick 'Choose...' to select them.";
    else if (j->destRoot.size() < 2 || j->destRoot[1] != L':') err = L"Choose a destination folder (external disk / USB), e.g. E:\\";
    else if (!MakeDirs(j->destRoot)) err = L"Destination folder is not accessible.";
    else if (IEquals(DriveOf(j->destRoot), DriveOf(j->srcDrive)))
        err = L"The destination is on the same drive as the source Windows.\nUse an external disk/USB (or another partition).";
    if (!err.empty()) { MessageBoxW(g_hwnd, err.c_str(), APP_TITLE, MB_ICONWARNING); delete j; return; }
    SetBusy(true);
    CloseHandle(CreateThread(nullptr, 0, BackupThread, j, 0, nullptr));
}

// ---- restore page logic
static void LoadPackage() {
    wstring pkg = StripSlash(GetText(hRPkg));
    SendMessageW(hRSrcUser, CB_RESETCONTENT, 0, 0);
    ListView_DeleteAllItems(hRProgs);
    g_progs.clear();
    if (!IsFile(Join(pkg, L"manifest.ini"))) { Log(L"Not a migration package (manifest.ini missing): " + pkg); return; }
    int nu = 0;
    for (auto& fd : ListDir(pkg + L"\\Users"))
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { SendMessageW(hRSrcUser, CB_ADDSTRING, 0, (LPARAM)fd.cFileName); nu++; }
    SendMessageW(hRSrcUser, CB_SETCURSEL, 0, 0);
    wstring txt;
    if (ReadText(Join(pkg, L"programs.txt"), txt)) {
        size_t pos = 0;
        while (pos < txt.size()) {
            size_t e = txt.find(L'\n', pos); if (e == wstring::npos) e = txt.size();
            wstring line = Trim(txt.substr(pos, e - pos)); pos = e + 1;
            if (line.empty() || line[0] == L'#') continue;
            vector<wstring> f; size_t a = 0;
            while (true) { size_t t = line.find(L'\t', a); f.push_back(line.substr(a, t == wstring::npos ? wstring::npos : t - a)); if (t == wstring::npos) break; a = t + 1; }
            while (f.size() < 4) f.push_back(L"");
            g_progs.push_back({f[0], f[1], f[2], WingetIdFor(f[0]), L""});
        }
    }
    // programs saved with their files in the package
    int napps = 0;
    for (auto& fd : ListDir(pkg + L"\\Apps")) {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        wstring adir = pkg + L"\\Apps\\" + fd.cFileName, ini = Join(adir, L"app.ini");
        if (!IsFile(ini)) continue;
        wchar_t nm[512], ver[128], pub[256];
        GetPrivateProfileStringW(L"App", L"Name", fd.cFileName, nm, 512, ini.c_str());
        GetPrivateProfileStringW(L"App", L"Version", L"", ver, 128, ini.c_str());
        GetPrivateProfileStringW(L"App", L"Publisher", L"", pub, 256, ini.c_str());
        napps++;
        bool found = false;
        for (auto& pr : g_progs) if (IEquals(pr.name, nm)) { pr.pkgDir = adir; found = true; break; }
        if (!found) g_progs.push_back({nm, ver, pub, WingetIdFor(nm), adir});
    }
    // order: copied programs, then known winget IDs, then the rest
    auto rank = [](const ProgItem& p) { return !p.pkgDir.empty() ? 0 : (!p.id.empty() ? 1 : 2); };
    std::stable_sort(g_progs.begin(), g_progs.end(), [&](const ProgItem& a, const ProgItem& b) { return rank(a) < rank(b); });
    for (size_t i = 0; i < g_progs.size(); ++i) {
        auto& pr = g_progs[i];
        wstring how = !pr.pkgDir.empty() ? (pr.id.empty() ? L"Copy from package (files + data)" : L"Copy from package  |  or winget: " + pr.id)
                                         : (pr.id.empty() ? L"winget: search by name" : L"winget: " + pr.id);
        LvAdd(hRProgs, {pr.name, pr.version, how}, (LPARAM)i, rank(pr) < 2);
    }
    wchar_t created[64] = L"";
    GetPrivateProfileStringW(L"Package", L"Created", L"?", created, 64, Join(pkg, L"manifest.ini").c_str());
    Log(Fmt(L"Package loaded (created %ls): %d user(s), %d program(s), %d saved with files + data.", created, nu, (int)g_progs.size(), napps));
}
static void AutoFindPackage() {
    wstring best;
    DWORD mask = GetLogicalDrives();
    for (int i = 2; i < 26; ++i) {
        if (!(mask & (1u << i))) continue;
        wstring root = Fmt(L"%lc:\\", L'A' + i);
        UINT t = GetDriveTypeW(root.c_str());
        if (t != DRIVE_FIXED && t != DRIVE_REMOVABLE) continue;
        for (auto& fd : ListDir(root)) {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !IStarts(fd.cFileName, L"PCMigrate_")) continue;
            wstring p = root + fd.cFileName;
            if (IsFile(Join(p, L"manifest.ini")) && (best.empty() || _wcsicmp(fd.cFileName, FileName(best).c_str()) > 0)) best = p;
        }
    }
    if (!best.empty()) { SetWindowTextW(hRPkg, best.c_str()); LoadPackage(); }
}
static void StartRestore() {
    RestoreJob* j = new RestoreJob();
    j->pkg = StripSlash(GetText(hRPkg));
    j->srcUser = GetText(hRSrcUser);
    int t = (int)SendMessageW(hRTgt, CB_GETCURSEL, 0, 0);
    if (t >= 0 && t < (int)g_tgtProfiles.size()) j->tgtProfile = g_tgtProfiles[t];
    j->files = SendMessageW(hRFiles, BM_GETCHECK, 0, 0) == BST_CHECKED;
    j->browsers = SendMessageW(hRBrowsers, BM_GETCHECK, 0, 0) == BST_CHECKED;
    j->settings = SendMessageW(hRSettings, BM_GETCHECK, 0, 0) == BST_CHECKED;
    wstring err;
    if (!IsFile(Join(j->pkg, L"manifest.ini"))) err = L"Choose a valid migration package folder (PCMigrate_xxxx) and click Load.";
    else if (j->srcUser.empty()) err = L"The package contains no user.";
    else if (j->tgtProfile.empty()) err = L"Choose the target profile.";
    else if (!j->files && !j->browsers && !j->settings) err = L"Select at least one thing to restore.";
    if (!err.empty()) { MessageBoxW(g_hwnd, err.c_str(), APP_TITLE, MB_ICONWARNING); delete j; return; }
    wstring q = L"Restore user '" + j->srcUser + L"' into:\n" + j->tgtProfile +
                L"\n\nNewer files already on this PC are kept. Existing browser profiles are renamed to .bak.\nContinue?";
    if (MessageBoxW(g_hwnd, q.c_str(), APP_TITLE, MB_ICONQUESTION | MB_YESNO) != IDYES) { delete j; return; }
    SetBusy(true);
    CloseHandle(CreateThread(nullptr, 0, RestoreThread, j, 0, nullptr));
}
static void StartInstall() {
    InstallJob* j = new InstallJob();
    j->pkg = StripSlash(GetText(hRPkg));
    j->srcUser = GetText(hRSrcUser);
    int t = (int)SendMessageW(hRTgt, CB_GETCURSEL, 0, 0);
    if (t >= 0 && t < (int)g_tgtProfiles.size()) j->tgtProfile = g_tgtProfiles[t];
    j->preferWinget = SendMessageW(hRPrefWinget, BM_GETCHECK, 0, 0) == BST_CHECKED;
    for (int i = 0, n = ListView_GetItemCount(hRProgs); i < n; ++i) {
        if (!ListView_GetCheckState(hRProgs, i)) continue;
        LVITEMW it{}; it.mask = LVIF_PARAM; it.iItem = i; ListView_GetItem(hRProgs, &it);
        if (it.lParam >= 0 && it.lParam < (LPARAM)g_progs.size()) j->items.push_back(g_progs[it.lParam]);
    }
    if (j->items.empty()) { MessageBoxW(g_hwnd, L"Check the programs you want to install / transfer.", APP_TITLE, MB_ICONINFORMATION); delete j; return; }
    int ncopy = 0; for (auto& it : j->items) if (!it.pkgDir.empty()) ncopy++;
    if (ncopy && j->tgtProfile.empty()) { MessageBoxW(g_hwnd, L"Choose the target profile ('Restore into this profile').", APP_TITLE, MB_ICONWARNING); delete j; return; }
    wstring q = Fmt(L"%d program(s) selected:\n - %d saved in the package (copied with their data%ls)\n - %d to download with winget (needs Internet)\n\nTarget profile: %ls\n\nContinue?",
                    (int)j->items.size(), ncopy, j->preferWinget ? L", or winget + data when an ID exists" : L"",
                    (int)j->items.size() - ncopy, j->tgtProfile.c_str());
    if (MessageBoxW(g_hwnd, q.c_str(), APP_TITLE, MB_ICONQUESTION | MB_YESNO) != IDYES) { delete j; return; }
    SetBusy(true);
    CloseHandle(CreateThread(nullptr, 0, InstallThread, j, 0, nullptr));
}

static void BuildUI() {
    // tab
    g_tab = Ctl(WC_TABCONTROLW, L"", WS_CLIPSIBLINGS | WS_TABSTOP, 10, 10, 800, 412, IDC_TAB, nullptr);
    TCITEMW ti{}; ti.mask = TCIF_TEXT;
    ti.pszText = (LPWSTR)L"  1. Backup  (old PC)  "; TabCtrl_InsertItem(g_tab, 0, &ti);
    ti.pszText = (LPWSTR)L"  2. Restore  (new PC)  "; TabCtrl_InsertItem(g_tab, 1, &ti);

    // ---- backup page
    auto* B = &g_pageB;
    Ctl(L"STATIC", L"Source Windows installation (old PC):", 0, 24, 48, 400, 20, -1, B);
    hBSrc = Ctl(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 24, 68, 580, 300, IDC_B_SRC, B);
    hBRefresh = Ctl(L"BUTTON", L"Refresh", WS_TABSTOP, 614, 67, 180, 26, IDC_B_REFRESH, B);
    Ctl(L"STATIC", L"User profiles to transfer:", 0, 24, 104, 300, 20, -1, B);
    hBUsers = Ctl(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | WS_TABSTOP, 24, 124, 370, 186, IDC_B_USERS, B, WS_EX_CLIENTEDGE);
    ListView_SetExtendedListViewStyle(hBUsers, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    AddColumn(hBUsers, 0, L"User", 120); AddColumn(hBUsers, 1, L"Profile folder", 220);
    hBGroup = Ctl(L"BUTTON", L"What to transfer", BS_GROUPBOX, 410, 100, 384, 210, IDC_B_GROUP, B);
    hBFiles = Ctl(L"BUTTON", L"User files  (Desktop, Documents, Pictures, Downloads...)", BS_AUTOCHECKBOX | WS_TABSTOP, 424, 126, 362, 24, IDC_B_FILES, B);
    hBBrowsers = Ctl(L"BUTTON", L"Browsers  (Chrome, Edge, Brave, Opera, Firefox)", BS_AUTOCHECKBOX | WS_TABSTOP, 424, 154, 362, 24, IDC_B_BROWSERS, B);
    hBSettings = Ctl(L"BUTTON", L"Windows settings  (mouse, keyboard, theme, wallpaper, Wi-Fi)", BS_AUTOCHECKBOX | WS_TABSTOP, 424, 182, 362, 24, IDC_B_SETTINGS, B);
    hBPrograms = Ctl(L"BUTTON", L"List of installed programs  (reinstall with winget)", BS_AUTOCHECKBOX | WS_TABSTOP, 424, 210, 362, 24, IDC_B_PROGRAMS, B);
    hBApps = Ctl(L"BUTTON", L"Programs with their files && data  (0 selected)", BS_AUTOCHECKBOX | WS_TABSTOP, 424, 238, 260, 24, IDC_B_APPS, B);
    hBChoose = Ctl(L"BUTTON", L"Choose...", WS_TABSTOP, 690, 236, 96, 26, IDC_B_CHOOSE, B);
    Ctl(L"STATIC", L"Works from Hiren's BootCD PE (offline) or from the running Windows. Close browsers/programs first when the old PC is running.", 0, 424, 268, 362, 38, -1, B);
    Ctl(L"STATIC", L"Save the migration package to (external disk / USB):", 0, 24, 322, 500, 20, -1, B);
    hBDest = Ctl(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, 24, 342, 480, 26, IDC_B_DEST, B, WS_EX_CLIENTEDGE);
    hBBrowse = Ctl(L"BUTTON", L"Browse...", WS_TABSTOP, 512, 341, 92, 28, IDC_B_BROWSE, B);
    hBStart = Ctl(L"BUTTON", L"Start Backup", BS_DEFPUSHBUTTON | WS_TABSTOP, 614, 336, 180, 38, IDC_B_START, B);
    SendMessageW(hBStart, WM_SETFONT, (WPARAM)g_fontBold, TRUE);
    for (auto h : {hBFiles, hBBrowsers, hBSettings, hBPrograms}) SendMessageW(h, BM_SETCHECK, BST_CHECKED, 0);

    // ---- restore page
    auto* R = &g_pageR;
    Ctl(L"STATIC", L"Migration package folder (PCMigrate_xxxx):", 0, 24, 48, 400, 20, -1, R);
    hRPkg = Ctl(WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, 24, 68, 480, 26, IDC_R_PKG, R, WS_EX_CLIENTEDGE);
    hRBrowse = Ctl(L"BUTTON", L"Browse...", WS_TABSTOP, 512, 67, 92, 28, IDC_R_BROWSE, R);
    hRLoad = Ctl(L"BUTTON", L"Load", WS_TABSTOP, 614, 67, 180, 28, IDC_R_LOAD, R);
    Ctl(L"STATIC", L"User in the package:", 0, 24, 104, 200, 20, -1, R);
    hRSrcUser = Ctl(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 24, 124, 200, 300, IDC_R_SRCUSER, R);
    Ctl(L"STATIC", L"Restore into this profile:", 0, 236, 104, 300, 20, -1, R);
    hRTgt = Ctl(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 236, 124, 368, 300, IDC_R_TGT, R);
    hRFiles = Ctl(L"BUTTON", L"User files", BS_AUTOCHECKBOX | WS_TABSTOP, 24, 160, 130, 24, IDC_R_FILES, R);
    hRBrowsers = Ctl(L"BUTTON", L"Browsers", BS_AUTOCHECKBOX | WS_TABSTOP, 160, 160, 120, 24, IDC_R_BROWSERS, R);
    hRSettings = Ctl(L"BUTTON", L"Windows settings + Wi-Fi", BS_AUTOCHECKBOX | WS_TABSTOP, 286, 160, 260, 24, IDC_R_SETTINGS, R);
    hRStart = Ctl(L"BUTTON", L"Start Restore", BS_DEFPUSHBUTTON | WS_TABSTOP, 614, 112, 180, 38, IDC_R_START, R);
    SendMessageW(hRStart, WM_SETFONT, (WPARAM)g_fontBold, TRUE);
    for (auto h : {hRFiles, hRBrowsers, hRSettings}) SendMessageW(h, BM_SETCHECK, BST_CHECKED, 0);
    Ctl(L"STATIC", L"Programs from the old PC  -  'Copy' = transferred from the package with its data,  'winget' = downloaded and installed:", 0, 24, 196, 770, 20, -1, R);
    hRProgs = Ctl(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | WS_TABSTOP, 24, 216, 770, 160, IDC_R_PROGS, R, WS_EX_CLIENTEDGE);
    ListView_SetExtendedListViewStyle(hRProgs, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    AddColumn(hRProgs, 0, L"Program", 330); AddColumn(hRProgs, 1, L"Version", 100); AddColumn(hRProgs, 2, L"Method", 310);
    hRCheckAll = Ctl(L"BUTTON", L"Check recommended", WS_TABSTOP, 24, 382, 150, 30, IDC_R_CHECKALL, R);
    hRUncheck = Ctl(L"BUTTON", L"Uncheck all", WS_TABSTOP, 180, 382, 100, 30, IDC_R_UNCHECK, R);
    hRPrefWinget = Ctl(L"BUTTON", L"Prefer winget (clean install + data)", BS_AUTOCHECKBOX | WS_TABSTOP, 290, 385, 316, 24, IDC_R_PREFWINGET, R);
    hRInstall = Ctl(L"BUTTON", L"Install / transfer checked", WS_TABSTOP, 614, 382, 180, 30, IDC_R_INSTALL, R);

    // ---- common bottom area
    g_progress = Ctl(PROGRESS_CLASSW, L"", PBS_SMOOTH, 10, 432, 690, 22, IDC_PROGRESS, nullptr);
    SendMessageW(g_progress, PBM_SETRANGE32, 0, 1000);
    g_cancelBtn = Ctl(L"BUTTON", L"Cancel", WS_TABSTOP, 710, 430, 100, 26, IDC_CANCEL, nullptr);
    EnableWindow(g_cancelBtn, FALSE);
    g_status = Ctl(L"STATIC", L"Ready.", SS_ENDELLIPSIS | SS_NOPREFIX, 10, 460, 800, 20, IDC_STATUS, nullptr);
    g_log = Ctl(WC_EDITW, L"", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 10, 482, 800, 176, IDC_LOG, nullptr, WS_EX_CLIENTEDGE);
    SendMessageW(g_log, EM_SETLIMITTEXT, 0, 0);

    SetWindowPos(g_tab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hBGroup, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(g_tab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowPage(0);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        BuildUI();
        Log(wstring(APP_TITLE) + (g_isPE ? L"  -  running in WinPE (offline mode)" : L"  -  running on Windows " + g_sysDrive));
        RefreshInstalls();
        AutoFindPackage();
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR* nh = (NMHDR*)lp;
        if (nh->idFrom == IDC_TAB && nh->code == TCN_SELCHANGE) ShowPage(TabCtrl_GetCurSel(g_tab));
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        switch (id) {
        case IDC_B_SRC: if (code == CBN_SELCHANGE) { FillUsers(); int s = (int)SendMessageW(hBSrc, CB_GETCURSEL, 0, 0); if (s >= 0) SetWindowTextW(hBDest, SuggestDest(g_installs[s].drive).c_str()); } break;
        case IDC_B_REFRESH: RefreshInstalls(); AutoFindPackage(); break;
        case IDC_B_BROWSE: { wstring p = BrowseFolder(L"Where to save the migration package (external disk / USB):"); if (!p.empty()) SetWindowTextW(hBDest, p.c_str()); } break;
        case IDC_B_START: StartBackup(); break;
        case IDC_B_CHOOSE: ShowChooser(); break;
        case IDC_B_APPS:
            if (code == BN_CLICKED && SendMessageW(hBApps, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                int n = 0; for (auto& a : g_srcApps) if (a.selected) n++;
                if (!n) ShowChooser();
            }
            break;
        case IDC_R_BROWSE: { wstring p = BrowseFolder(L"Select the PCMigrate_xxxx package folder:"); if (!p.empty()) { SetWindowTextW(hRPkg, p.c_str()); LoadPackage(); } } break;
        case IDC_R_LOAD: LoadPackage(); break;
        case IDC_R_START: StartRestore(); break;
        case IDC_R_CHECKALL:
            for (int i = 0, n = ListView_GetItemCount(hRProgs); i < n; ++i) {
                LVITEMW it{}; it.mask = LVIF_PARAM; it.iItem = i; ListView_GetItem(hRProgs, &it);
                ListView_SetCheckState(hRProgs, i, it.lParam < (LPARAM)g_progs.size() && (!g_progs[it.lParam].id.empty() || !g_progs[it.lParam].pkgDir.empty()));
            }
            break;
        case IDC_R_UNCHECK: for (int i = 0, n = ListView_GetItemCount(hRProgs); i < n; ++i) ListView_SetCheckState(hRProgs, i, FALSE); break;
        case IDC_R_INSTALL: StartInstall(); break;
        case IDC_CANCEL: g_cancel = TRUE; Log(L"Cancelling ..."); break;
        }
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp;
        SetBkColor(dc, GetSysColor(COLOR_WINDOW));
        SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    case WM_APP_LOG: { wstring* s = (wstring*)lp; AppendLog(*s); delete s; return 0; }
    case WM_APP_PROGRESS: {
        SendMessageW(g_progress, PBM_SETPOS, wp, 0);
        wstring* s = (wstring*)lp; if (s) { SetWindowTextW(g_status, s->c_str()); delete s; }
        return 0;
    }
    case WM_APP_DONE: SetBusy(false); SetWindowTextW(g_status, g_cancel ? L"Cancelled." : L"Finished. See the log below."); MessageBeep(MB_ICONINFORMATION); return 0;
    case WM_CLOSE:
        if (g_busy) {
            if (MessageBoxW(hwnd, L"A job is running. Cancel it and exit?", APP_TITLE, MB_ICONWARNING | MB_YESNO) != IDYES) return 0;
            g_cancel = TRUE;
            Sleep(800);
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nShow) {
    InitializeCriticalSection(&g_logCs);
    OleInitialize(nullptr);
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_WIN95_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);
    EnablePrivilege(SE_BACKUP_NAME);
    EnablePrivilege(SE_RESTORE_NAME);
    g_isPE = DetectWinPE();
    wchar_t wd[MAX_PATH]; GetWindowsDirectoryW(wd, MAX_PATH);
    g_sysDrive = wstring(wd).substr(0, 2);

    HDC dc = GetDC(nullptr); g_scale = GetDeviceCaps(dc, LOGPIXELSY) / 96.0; ReleaseDC(nullptr, dc);
    NONCLIENTMETRICSW ncm{}; ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    LOGFONTW lf = ncm.lfMessageFont;
    if (lf.lfHeight == 0) { lf.lfHeight = -S(12); wcscpy(lf.lfFaceName, L"Segoe UI"); }
    g_font = CreateFontIndirectW(&lf);
    lf.lfWeight = FW_BOLD; g_fontBold = CreateFontIndirectW(&lf);

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WndProc; wc.hInstance = hInst; wc.lpszClassName = L"PCMigrateWnd";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1)); if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);

    RECT r{0, 0, S(820), S(668)};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&r, style, FALSE);
    int w = r.right - r.left, h = r.bottom - r.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2, y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, APP_TITLE, style, x < 0 ? 0 : x, y < 0 ? 0 : y, w, h, nullptr, nullptr, hInst, nullptr);
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hwnd, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    OleUninitialize();
    return 0;
}
