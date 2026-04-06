/*
 * FolderGuardApp.cpp - v8
 * Compile : Visual Studio x64 Release
 * Libs    : fltlib.lib comctl32.lib shell32.lib shlwapi.lib psapi.lib
 */

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <fltuser.h>
#include <psapi.h>
#include <shellapi.h>
#include <strsafe.h>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>

#include "FolderGuardCommon.h"

#pragma comment(lib, "fltlib.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")

 /* ===========================================================================
    LANGUES
    =========================================================================== */

struct Lang {
    const wchar_t* title;
    const wchar_t* btnConnect;
    const wchar_t* btnDisconnect;
    const wchar_t* btnBrowse;
    const wchar_t* btnAdd;
    const wchar_t* btnAddFile;
    const wchar_t* btnRemove;
    const wchar_t* btnClear;
    const wchar_t* lblFolders;
    const wchar_t* lblLog;
    const wchar_t* lblStatus;
    const wchar_t* lblStatusOff;
    const wchar_t* colTime;
    const wchar_t* colProcess;
    const wchar_t* colPid;
    const wchar_t* colFolder;
    const wchar_t* colFile;
    const wchar_t* colDecision;
    const wchar_t* decAllow;
    const wchar_t* decDeny;
    const wchar_t* popupTitle;
    const wchar_t* popupBody;
    const wchar_t* optOpenExplorer;
    const wchar_t* optLang;
    const wchar_t* errAdmin;
    const wchar_t* errNoDriver;
    const wchar_t* errRootDrive;
    const wchar_t* connectedMsg;
    const wchar_t* tabFolders;
    const wchar_t* tabLog;
    const wchar_t* tabSettings;
    const wchar_t* colPath;
    const wchar_t* colType;
    const wchar_t* typeFolder;
    const wchar_t* typeFile;
};

static const Lang LANG_FR = {
    L"FolderGuard - Protection de dossiers",
    L"Connecter", L"Deconnecter", L"Parcourir", L"Ajouter dossier", L"Ajouter fichier",
    L"Retirer", L"Effacer",
    L"Elements proteges", L"Journal des acces",
    L"CONNECTE", L"NON CONNECTE",
    L"Heure", L"Processus", L"PID", L"Element", L"Fichier", L"Decision",
    L"Autorise", L"Bloque",
    L"FolderGuard - Acces detecte",
    L"ACCES PROTEGE\n\n"
    L"Processus : %s\nPID       : %lu\nExe       : %s\n"
    L"Fichier   : %s\nElement   : %s\nType      : %s\n\n"
    L"Le processus est SUSPENDU.\n\n"
    L"OUI -> Autoriser (memorise pour cet element)\n"
    L"NON -> Bloquer et tuer le processus",
    L"Ouvrir l'explorateur apres un blocage",
    L"Langue / Language",
    L"FolderGuard doit etre lance en tant qu'Administrateur.",
    L"Connexion au driver impossible.\nErreur : 0x%08X\n\n"
    L"Verifiez que le driver est demarre\net que l'app est en mode Admin.",
    L"Impossible de securiser un disque entier (%s).\n"
    L"Choisissez un sous-dossier.",
    L"Connecte au driver !\nLes elements sont maintenant proteges.",
    L"Elements", L"Journal", L"Parametres",
    L"Chemin", L"Type", L"Dossier", L"Fichier"
};

static const Lang LANG_EN = {
    L"FolderGuard - Protection",
    L"Connect", L"Disconnect", L"Browse", L"Add folder", L"Add file",
    L"Remove", L"Clear",
    L"Protected items", L"Access log",
    L"CONNECTED", L"NOT CONNECTED",
    L"Time", L"Process", L"PID", L"Item", L"File", L"Decision",
    L"Allowed", L"Blocked",
    L"FolderGuard - Access Detected",
    L"PROTECTED ACCESS\n\n"
    L"Process : %s\nPID     : %lu\nExe     : %s\n"
    L"File    : %s\nItem    : %s\nType    : %s\n\n"
    L"The process is SUSPENDED.\n\n"
    L"YES -> Allow (saved for this item)\n"
    L"NO  -> Block and kill the process",
    L"Open Explorer after blocking",
    L"Langue / Language",
    L"FolderGuard must be run as Administrator.",
    L"Cannot connect to driver.\nError: 0x%08X\n\n"
    L"Make sure the driver is started\nand the app is run as Admin.",
    L"Cannot protect an entire drive (%s).\n"
    L"Please choose a subfolder.",
    L"Connected to driver!\nItems are now protected.",
    L"Items", L"Log", L"Settings",
    L"Path", L"Type", L"Folder", L"File"
};

static const Lang* L = &LANG_FR;

/* ===========================================================================
   CONSTANTES UI
   =========================================================================== */

#define WM_TRAYICON      (WM_USER + 1)
#define WM_ADD_LOG       (WM_USER + 2)

#define IDM_TRAY_SHOW    2001
#define IDM_TRAY_EXIT    2002
#define IDI_TRAY         1001

#define IDC_TAB          4000
#define IDC_LIST_ITEMS   4001
#define IDC_BTN_ADDFOL   4002
#define IDC_BTN_ADDFILE  4010
#define IDC_BTN_REMOVE   4003
#define IDC_BTN_CONNECT  4004
#define IDC_LIST_LOG     4005
#define IDC_BTN_CLEAR    4006
#define IDC_CHK_EXPLORER 4007
#define IDC_COMBO_LANG   4008
#define IDC_LABEL_STATUS 4009

#define CLR_BG    0x1A1A1A
#define CLR_PANEL 0x222222
#define CLR_ALLOW 0x27AE60
#define CLR_DENY  0xE74C3C
#define CLR_TEXT  0xEEEEEE
#define CLR_DIM   0x888888

   /* ===========================================================================
      PROCESSUS SYSTEME TOUJOURS AUTORISES
      =========================================================================== */

static const wchar_t* SYSTEM_EXE[] = {
    L"explorer.exe", L"svchost.exe", L"lsass.exe", L"csrss.exe",
    L"winlogon.exe", L"services.exe", L"smss.exe", L"wininit.exe",
    L"dwm.exe", L"taskhostw.exe", L"sihost.exe", L"fontdrvhost.exe",
    L"searchindexer.exe", L"searchhost.exe", L"runtimebroker.exe",
    L"applicationframehost.exe", L"shellexperiencehost.exe",
    L"startmenuexperiencehost.exe", L"textinputhost.exe",
    L"msmpeng.exe", L"mssense.exe", L"securityhealthservice.exe",
    L"folderguardapp.exe",
    nullptr
};

/* ===========================================================================
   STRUCTURES
   =========================================================================== */

   // Un element protege peut etre un dossier OU un fichier individuel
struct ProtectedItem {
    std::wstring              path;        // Chemin Win32
    std::wstring              devicePath;  // Chemin NT device
    bool                      isFile;      // true = fichier, false = dossier
    std::unordered_set<std::wstring> allowedExes;
};

struct PendingPopup {
    ULONG          RequestId;
    DWORD          Pid;
    FG_ACCESS_TYPE AccessType;
    std::wstring   ProcessName;
    std::wstring   FilePath;
    std::wstring   ExePath;
    std::wstring   MatchedItem; // Chemin Win32 de l'element concerne
};

struct LogEntry {
    std::wstring ProcessName;
    std::wstring FilePath;
    std::wstring ItemPath;
    DWORD        Pid;
    FG_DECISION  Decision;
};

/* ===========================================================================
   GLOBALES
   =========================================================================== */

HWND              g_hWnd = NULL;
HWND              g_hTab = NULL;
HWND              g_hListItems = NULL;
HWND              g_hListLog = NULL;
HWND              g_hBtnAddFol = NULL;
HWND              g_hBtnAddFile = NULL;
HWND              g_hBtnRemove = NULL;
HWND              g_hBtnConnect = NULL;
HWND              g_hBtnClear = NULL;
HWND              g_hChkExplorer = NULL;
HWND              g_hComboLang = NULL;
HWND              g_hLblStatus = NULL;
HINSTANCE         g_hInst = NULL;
NOTIFYICONDATA    g_nid = {};
HANDLE            g_hPort = INVALID_HANDLE_VALUE;
std::atomic<bool> g_Running = false;

std::vector<ProtectedItem> g_Items;
std::mutex                  g_ItemsMutex;

std::set<std::wstring>      g_PendingExe;
std::mutex                  g_PendingMutex;

std::unordered_set<std::wstring> g_LoggedKeys;
std::mutex                        g_LoggedMutex;

std::wofstream    g_LogFile;
std::mutex        g_LogFileMutex;

bool              g_OpenExplorerOnBlock = false;
bool              g_LangFR = true;

HFONT g_hFontMono = NULL;
HFONT g_hFontUI = NULL;
HFONT g_hFontBig = NULL;
ULONG g_AllowCnt = 0;
ULONG g_DenyCnt = 0;

std::wstring g_AppDir;

int  g_CurrentTab = 0;
HWND g_hTabPanels[3] = {};

/* ===========================================================================
   HELPERS
   =========================================================================== */

static std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

std::wstring GetExePathFromPid(DWORD pid)
{
    if (pid == 0) return L"";
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return L"";
    WCHAR buf[MAX_PATH] = {};
    DWORD len = MAX_PATH;
    QueryFullProcessImageNameW(h, 0, buf, &len);
    CloseHandle(h);
    return buf;
}

std::wstring Win32ToDevicePath(const std::wstring& p)
{
    if (p.size() < 2 || p[1] != L':') return p;
    WCHAR drive[3] = { p[0], L':', L'\0' };
    WCHAR dev[MAX_PATH] = {};
    if (!QueryDosDeviceW(drive, dev, MAX_PATH)) return p;
    return std::wstring(dev) + p.substr(2);
}

bool IsSystemExe(const std::wstring& exePath)
{
    std::wstring name = ToLower(PathFindFileNameW(exePath.c_str()));
    for (int i = 0; SYSTEM_EXE[i]; i++)
        if (name == SYSTEM_EXE[i]) return true;
    return false;
}

bool IsRootDrive(const std::wstring& path)
{
    if (path.size() <= 3 && path.size() >= 2 && path[1] == L':') return true;
    return false;
}

/*
 * Pour un chemin NT de fichier, retourne le chemin Win32 de l'element protege
 * qui correspond (dossier = prefix match, fichier = match exact).
 */
std::wstring FindMatchingItem(const std::wstring& ntFilePath)
{
    std::lock_guard<std::mutex> lk(g_ItemsMutex);
    std::wstring lower = ToLower(ntFilePath);
    for (auto& item : g_Items) {
        std::wstring devLow = ToLower(item.devicePath);
        if (item.isFile) {
            // Fichier individuel : match exact
            if (lower == devLow) return item.path;
        }
        else {
            // Dossier : prefix match
            if (lower.find(devLow) == 0) return item.path;
        }
    }
    return L"";
}

bool IsAllowedForItem(const std::wstring& exePath, const std::wstring& itemPath)
{
    if (IsSystemExe(exePath)) return true;
    std::lock_guard<std::mutex> lk(g_ItemsMutex);
    for (auto& item : g_Items) {
        if (ToLower(item.path) == ToLower(itemPath))
            return item.allowedExes.count(ToLower(exePath)) > 0;
    }
    return false;
}

void AddExeToItem(const std::wstring& exePath, const std::wstring& itemPath)
{
    std::lock_guard<std::mutex> lk(g_ItemsMutex);
    for (auto& item : g_Items)
        if (ToLower(item.path) == ToLower(itemPath)) {
            item.allowedExes.insert(ToLower(exePath));
            return;
        }
}

/* ===========================================================================
   DRIVER : demarrer / arreter automatiquement
   =========================================================================== */

void StartDriverService()
{
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) return;
    SC_HANDLE hSvc = OpenServiceW(hSCM, L"FolderGuardDriver", SERVICE_START | SERVICE_QUERY_STATUS);
    if (hSvc) {
        SERVICE_STATUS ss = {};
        QueryServiceStatus(hSvc, &ss);
        if (ss.dwCurrentState != SERVICE_RUNNING)
            StartServiceW(hSvc, 0, nullptr);
        CloseServiceHandle(hSvc);
    }
    CloseServiceHandle(hSCM);
}

void StopDriverService()
{
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) return;
    SC_HANDLE hSvc = OpenServiceW(hSCM, L"FolderGuardDriver",
        SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (hSvc) {
        SERVICE_STATUS ss = {};
        ControlService(hSvc, SERVICE_CONTROL_STOP, &ss);
        CloseServiceHandle(hSvc);
    }
    CloseServiceHandle(hSCM);
}

/* ===========================================================================
   PERSISTANCE
   =========================================================================== */

void SaveConfig()
{
    std::wstring path = g_AppDir + L"\\fg_config.txt";
    std::wofstream f(path, std::ios::trunc);
    if (!f.is_open()) return;

    f << L"LANG=" << (g_LangFR ? L"FR" : L"EN") << L"\n";
    f << L"EXPLORER=" << (g_OpenExplorerOnBlock ? L"1" : L"0") << L"\n";

    std::lock_guard<std::mutex> lk(g_ItemsMutex);
    for (auto& item : g_Items) {
        f << (item.isFile ? L"FILE=" : L"FOLDER=") << item.path << L"\n";
        for (auto& exe : item.allowedExes)
            f << L"ALLOW=" << item.path << L"|" << exe << L"\n";
    }
}

void LoadConfig()
{
    std::wstring path = g_AppDir + L"\\fg_config.txt";
    std::wifstream f(path);
    if (!f.is_open()) return;

    std::wstring line;
    while (std::getline(f, line)) {
        if (line.find(L"LANG=") == 0)
            g_LangFR = (line.substr(5) == L"FR");
        else if (line.find(L"EXPLORER=") == 0)
            g_OpenExplorerOnBlock = (line.substr(9) == L"1");
        else if (line.find(L"FOLDER=") == 0 || line.find(L"FILE=") == 0) {
            bool isFile = (line.find(L"FILE=") == 0);
            std::wstring p = line.substr(isFile ? 5 : 7);
            if (!p.empty() && (isFile || !IsRootDrive(p))) {
                ProtectedItem item;
                item.path = p;
                item.devicePath = Win32ToDevicePath(p);
                item.isFile = isFile;
                g_Items.push_back(std::move(item));
            }
        }
        else if (line.find(L"ALLOW=") == 0) {
            std::wstring rest = line.substr(6);
            auto pos = rest.find(L'|');
            if (pos != std::wstring::npos) {
                std::wstring ipath = rest.substr(0, pos);
                std::wstring exe = rest.substr(pos + 1);
                for (auto& item : g_Items)
                    if (ToLower(item.path) == ToLower(ipath))
                        item.allowedExes.insert(exe);
            }
        }
    }
}

/* ===========================================================================
   LOG FICHIER
   =========================================================================== */

void OpenLogFile()
{
    SYSTEMTIME st; GetLocalTime(&st);
    WCHAR name[64];
    StringCchPrintfW(name, 64, L"fg_log_%04d%02d%02d_%02d%02d%02d.log",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::lock_guard<std::mutex> lk(g_LogFileMutex);
    g_LogFile.open(g_AppDir + L"\\" + name, std::ios::trunc);
    if (g_LogFile.is_open())
        g_LogFile << L"# FolderGuard log - " << name << L"\n\n";
}

void WriteLogLine(const LogEntry& e)
{
    SYSTEMTIME st; GetLocalTime(&st);
    WCHAR ts[32];
    StringCchPrintfW(ts, 32, L"[%02d:%02d:%02d]", st.wHour, st.wMinute, st.wSecond);
    std::lock_guard<std::mutex> lk(g_LogFileMutex);
    if (!g_LogFile.is_open()) return;
    g_LogFile
        << ts << L" "
        << (e.Decision == FG_DECISION_ALLOW ? L"ALLOW" : L"BLOCK")
        << L" | " << e.ProcessName
        << L" (PID " << e.Pid << L")"
        << L" | item: " << e.ItemPath
        << L" | file: " << PathFindFileNameW(e.FilePath.c_str())
        << L"\n";
    g_LogFile.flush();
}

/* ===========================================================================
   COMMUNICATION DRIVER
   =========================================================================== */

void SendDriverMsg(FG_USER_MESSAGE& msg)
{
    if (g_hPort == INVALID_HANDLE_VALUE) return;
    DWORD ret = 0;
    FilterSendMessage(g_hPort, &msg, sizeof(msg), nullptr, 0, &ret);
}

void SendDecision(ULONG reqId, FG_DECISION decision)
{
    FG_USER_MESSAGE msg = {};
    msg.Type = FG_MSG_RESPONSE;
    msg.RequestId = reqId;
    msg.Decision = decision;
    SendDriverMsg(msg);
}

void SendAllProtectedItems()
{
    std::lock_guard<std::mutex> lk(g_ItemsMutex);
    for (auto& item : g_Items) {
        FG_USER_MESSAGE msg = {};
        msg.Type = FG_MSG_SET_DIR;
        StringCchCopyW(msg.DirPath, 512, item.devicePath.c_str());
        SendDriverMsg(msg);
    }
}

/* ===========================================================================
   LOG UI
   =========================================================================== */

void PostLogEntry(const std::wstring& proc, const std::wstring& file,
    const std::wstring& itemPath, DWORD pid, FG_DECISION decision)
{
    std::wstring key = ToLower(proc) + L"|" + ToLower(itemPath) + L"|" +
        std::to_wstring((int)decision);
    {
        std::lock_guard<std::mutex> lk(g_LoggedMutex);
        if (g_LoggedKeys.count(key)) return;
        g_LoggedKeys.insert(key);
    }
    LogEntry* e = new LogEntry{ proc, file, itemPath, pid, decision };
    WriteLogLine(*e);
    PostMessageW(g_hWnd, WM_ADD_LOG, (WPARAM)decision, (LPARAM)e);
}

void AddLogEntryUI(LogEntry* e)
{
    if (!e) return;
    SYSTEMTIME st; GetLocalTime(&st);
    WCHAR timeBuf[16], pidBuf[16];
    StringCchPrintfW(timeBuf, 16, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    StringCchPrintfW(pidBuf, 16, L"%lu", e->Pid);

    int idx = ListView_GetItemCount(g_hListLog);
    LVITEM lvi = {};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = idx;
    lvi.pszText = timeBuf;
    ListView_InsertItem(g_hListLog, &lvi);
    ListView_SetItemText(g_hListLog, idx, 1, (PWSTR)e->ProcessName.c_str());
    ListView_SetItemText(g_hListLog, idx, 2, pidBuf);
    ListView_SetItemText(g_hListLog, idx, 3, (PWSTR)PathFindFileNameW(e->ItemPath.c_str()));
    ListView_SetItemText(g_hListLog, idx, 4, (PWSTR)PathFindFileNameW(e->FilePath.c_str()));
    ListView_SetItemText(g_hListLog, idx, 5,
        (PWSTR)(e->Decision == FG_DECISION_ALLOW ? L->decAllow : L->decDeny));
    ListView_EnsureVisible(g_hListLog, idx, FALSE);

    WCHAR title[256];
    StringCchPrintfW(title, 256, L"FolderGuard  |  %lu %s  |  %lu %s",
        g_AllowCnt, L->decAllow, g_DenyCnt, L->decDeny);
    SetWindowTextW(g_hWnd, title);
    delete e;
}

/* ===========================================================================
   POPUP
   =========================================================================== */

void ShowAccessPopup(PendingPopup* popup)
{
    const wchar_t* typeStr = popup->AccessType == FG_ACCESS_READ ? L"READ" : L"WRITE";
    const wchar_t* exeStr = popup->ExePath.empty() ? L"(unknown)" : popup->ExePath.c_str();
    const wchar_t* itemStr = popup->MatchedItem.empty() ? L"?" : popup->MatchedItem.c_str();

    WCHAR msg[2048];
    StringCchPrintfW(msg, 2048, L->popupBody,
        popup->ProcessName.c_str(), popup->Pid, exeStr,
        PathFindFileNameW(popup->FilePath.c_str()), itemStr, typeStr);

    int ret = MessageBoxW(NULL, msg, L->popupTitle,
        MB_YESNO | MB_ICONWARNING | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST);

    FG_DECISION decision = (ret == IDYES) ? FG_DECISION_ALLOW : FG_DECISION_DENY;

    if (decision == FG_DECISION_ALLOW) {
        if (!popup->ExePath.empty() && !popup->MatchedItem.empty())
            AddExeToItem(popup->ExePath, popup->MatchedItem);
        g_AllowCnt++;
        SaveConfig();
        // Mettre a jour le compte dans la liste des elements
        PostMessageW(g_hWnd, WM_ADD_LOG, 0, 0); // juste pour refresh le titre
    }
    else {
        g_DenyCnt++;
    }

    // 1. Liberer l'IRP d'abord
    SendDecision(popup->RequestId, decision);

    // 2. Tuer apres
    if (decision == FG_DECISION_DENY) {
        Sleep(50);
        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, popup->Pid);
        if (h) { TerminateProcess(h, 0xDEAD); CloseHandle(h); }

        if (g_OpenExplorerOnBlock && !popup->ExePath.empty()) {
            WCHAR exeDir[MAX_PATH];
            StringCchCopyW(exeDir, MAX_PATH, popup->ExePath.c_str());
            PathRemoveFileSpecW(exeDir);
            ShellExecuteW(NULL, L"open", exeDir, nullptr, nullptr, SW_SHOW);
        }
    }

    PostLogEntry(popup->ProcessName, popup->FilePath,
        popup->MatchedItem, popup->Pid, decision);

    {
        std::lock_guard<std::mutex> lk(g_PendingMutex);
        g_PendingExe.erase(ToLower(popup->ExePath) + L"|" + ToLower(popup->MatchedItem));
    }

    delete popup;
}

/* ===========================================================================
   THREAD D'ECOUTE
   =========================================================================== */

void ListenThread()
{
    struct { FILTER_MESSAGE_HEADER hdr; FG_NOTIFICATION notif; } buf = {};

    while (g_Running) {
        ZeroMemory(&buf, sizeof(buf));
        HRESULT hr = FilterGetMessage(g_hPort, &buf.hdr, sizeof(buf), nullptr);
        if (FAILED(hr)) {
            if (hr == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED) ||
                hr == HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)) break;
            Sleep(50);
            continue;
        }

        FG_NOTIFICATION& notif = buf.notif;

        // PID 0 ou 4 = systeme
        if (notif.ProcessId == 0 || notif.ProcessId == 4) {
            SendDecision(notif.RequestId, FG_DECISION_ALLOW);
            continue;
        }

        std::wstring exePath = GetExePathFromPid(notif.ProcessId);
        std::wstring ntFile = notif.FilePath;
        std::wstring item = FindMatchingItem(ntFile);

        if (item.empty() || IsSystemExe(exePath) || IsAllowedForItem(exePath, item)) {
            SendDecision(notif.RequestId, FG_DECISION_ALLOW);
            continue;
        }

        // Anti-spam
        {
            std::lock_guard<std::mutex> lk(g_PendingMutex);
            std::wstring key = ToLower(exePath) + L"|" + ToLower(item);
            if (g_PendingExe.count(key)) {
                SendDecision(notif.RequestId, FG_DECISION_ALLOW);
                continue;
            }
            g_PendingExe.insert(key);
        }

        PendingPopup* pp = new PendingPopup{
            notif.RequestId, notif.ProcessId, notif.AccessType,
            notif.ProcessName, ntFile, exePath, item
        };
        std::thread([pp]() { ShowAccessPopup(pp); }).detach();
    }
}

/* ===========================================================================
   CONNECT / DISCONNECT
   =========================================================================== */

void UpdateStatus()
{
    bool conn = (g_hPort != INVALID_HANDLE_VALUE);
    SetWindowTextW(g_hLblStatus, conn ? L->lblStatus : L->lblStatusOff);
    InvalidateRect(g_hLblStatus, NULL, TRUE);
}

void ConnectToDriver()
{
    if (g_hPort != INVALID_HANDLE_VALUE) {
        g_Running = false;
        CancelIoEx(g_hPort, nullptr);
        CloseHandle(g_hPort);
        g_hPort = INVALID_HANDLE_VALUE;
        UpdateStatus();
        SetWindowTextW(g_hBtnConnect, L->btnConnect);
        return;
    }

    HRESULT hr = FilterConnectCommunicationPort(
        FG_PORT_NAME, 0, nullptr, 0, nullptr, &g_hPort);
    if (FAILED(hr)) {
        WCHAR err[256];
        StringCchPrintfW(err, 256, L->errNoDriver, hr);
        MessageBoxW(g_hWnd, err, L"FolderGuard", MB_ICONERROR);
        g_hPort = INVALID_HANDLE_VALUE;
        return;
    }

    SendAllProtectedItems();
    OpenLogFile();

    g_Running = true;
    std::thread(ListenThread).detach();

    UpdateStatus();
    SetWindowTextW(g_hBtnConnect,
        (g_hPort != INVALID_HANDLE_VALUE) ? L->btnDisconnect : L->btnConnect);
    MessageBoxW(g_hWnd, L->connectedMsg, L"FolderGuard", MB_ICONINFORMATION);
}

/* ===========================================================================
   GESTION DES ELEMENTS PROTEGES
   =========================================================================== */

void RefreshItemList()
{
    ListView_DeleteAllItems(g_hListItems);
    std::lock_guard<std::mutex> lk(g_ItemsMutex);
    for (int i = 0; i < (int)g_Items.size(); i++) {
        LVITEM lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = (PWSTR)g_Items[i].path.c_str();
        ListView_InsertItem(g_hListItems, &lvi);
        ListView_SetItemText(g_hListItems, i, 1,
            (PWSTR)(g_Items[i].isFile ? L->typeFile : L->typeFolder));
        WCHAR buf[16];
        StringCchPrintfW(buf, 16, L"%d", (int)g_Items[i].allowedExes.size());
        ListView_SetItemText(g_hListItems, i, 2, buf);
    }
}

void AddItem(const std::wstring& path, bool isFile)
{
    if (!isFile && IsRootDrive(path)) {
        WCHAR msg[256];
        StringCchPrintfW(msg, 256, L->errRootDrive, path.c_str());
        MessageBoxW(g_hWnd, msg, L"FolderGuard", MB_ICONWARNING);
        return;
    }
    {
        std::lock_guard<std::mutex> lk(g_ItemsMutex);
        for (auto& item : g_Items)
            if (ToLower(item.path) == ToLower(path)) return;
        ProtectedItem pi;
        pi.path = path;
        pi.devicePath = Win32ToDevicePath(path);
        pi.isFile = isFile;
        g_Items.push_back(std::move(pi));
    }
    RefreshItemList();
    SaveConfig();
    if (g_hPort != INVALID_HANDLE_VALUE) {
        FG_USER_MESSAGE msg = {};
        msg.Type = FG_MSG_SET_DIR;
        StringCchCopyW(msg.DirPath, 512, Win32ToDevicePath(path).c_str());
        SendDriverMsg(msg);
    }
}

void RemoveSelectedItem()
{
    int sel = ListView_GetNextItem(g_hListItems, -1, LVNI_SELECTED);
    if (sel < 0) return;
    {
        std::lock_guard<std::mutex> lk(g_ItemsMutex);
        if (sel < (int)g_Items.size())
            g_Items.erase(g_Items.begin() + sel);
    }
    RefreshItemList();
    SaveConfig();
}

/* ===========================================================================
   LANGUE
   =========================================================================== */

void ApplyLanguage()
{
    SetWindowTextW(g_hWnd, L->title);
    SetWindowTextW(g_hBtnAddFol, L->btnAdd);
    SetWindowTextW(g_hBtnAddFile, L->btnAddFile);
    SetWindowTextW(g_hBtnRemove, L->btnRemove);
    SetWindowTextW(g_hBtnClear, L->btnClear);
    SetWindowTextW(g_hChkExplorer, L->optOpenExplorer);
    SetWindowTextW(g_hBtnConnect,
        (g_hPort != INVALID_HANDLE_VALUE) ? L->btnDisconnect : L->btnConnect);
    SetWindowTextW(g_hLblStatus,
        (g_hPort != INVALID_HANDLE_VALUE) ? L->lblStatus : L->lblStatusOff);

    TCITEMW ti = { TCIF_TEXT };
    ti.pszText = (PWSTR)L->tabFolders; TabCtrl_SetItem(g_hTab, 0, &ti);
    ti.pszText = (PWSTR)L->tabLog;     TabCtrl_SetItem(g_hTab, 1, &ti);
    ti.pszText = (PWSTR)L->tabSettings; TabCtrl_SetItem(g_hTab, 2, &ti);

    // Forcer le re-rendu du panel settings (fix checkbox/combobox qui disparaissent)
    if (g_hTabPanels[2]) {
        RedrawWindow(g_hTabPanels[2], NULL, NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}

/* ===========================================================================
   PANEL PROC (transmet WM_COMMAND au parent)
   =========================================================================== */

LRESULT CALLBACK PanelProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND)
        return SendMessageW(g_hWnd, msg, wParam, lParam);
    // Transmettre WM_NOTIFY seulement pour les controles enfants (listview custom draw etc.)
    // mais PAS pour le tab control lui-meme (son TCN_SELCHANGE va deja directement a g_hWnd)
    if (msg == WM_NOTIFY) {
        LPNMHDR nm = (LPNMHDR)lParam;
        if (nm->hwndFrom != g_hTab)
            return SendMessageW(g_hWnd, msg, wParam, lParam);
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        HDC hdc = (HDC)wParam;
        RECT r; GetClientRect(hWnd, &r);
        FillRect(hdc, &r, CreateSolidBrush(CLR_BG));
        return 1;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

/* ===========================================================================
   CREATION UI
   =========================================================================== */

void SwitchTab(int idx)
{
    if (idx < 0 || idx >= 3) return;
    for (int i = 0; i < 3; i++) {
        if (!g_hTabPanels[i]) continue;
        if (i == idx) {
            ShowWindow(g_hTabPanels[i], SW_SHOW);
            SetWindowPos(g_hTabPanels[i], HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            RedrawWindow(g_hTabPanels[i], NULL, NULL,
                RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        else {
            ShowWindow(g_hTabPanels[i], SW_HIDE);
        }
    }
    g_CurrentTab = idx;
}

void CreateTabItems(HWND hParent, RECT& rc)
{
    int x = rc.left + 8, y = rc.top + 8;
    int W = rc.right - rc.left - 16;
    int H = rc.bottom - rc.top - 16;

    g_hListItems = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        x, y, W, H - 36,
        hParent, (HMENU)IDC_LIST_ITEMS, g_hInst, nullptr);
    ListView_SetExtendedListViewStyle(g_hListItems,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(g_hListItems, CLR_PANEL);
    ListView_SetTextBkColor(g_hListItems, CLR_PANEL);
    ListView_SetTextColor(g_hListItems, CLR_TEXT);

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = (PWSTR)L->colPath;  lvc.cx = W - 230; ListView_InsertColumn(g_hListItems, 0, &lvc);
    lvc.pszText = (PWSTR)L->colType;  lvc.cx = 80;      ListView_InsertColumn(g_hListItems, 1, &lvc);
    lvc.pszText = (PWSTR)L"Autorises"; lvc.cx = 80;      ListView_InsertColumn(g_hListItems, 2, &lvc);

    int by = y + H - 32;
    g_hBtnAddFol = CreateWindowExW(0, L"BUTTON", L->btnAdd,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, by, 130, 26, hParent, (HMENU)IDC_BTN_ADDFOL, g_hInst, nullptr);
    g_hBtnAddFile = CreateWindowExW(0, L"BUTTON", L->btnAddFile,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x + 138, by, 130, 26, hParent, (HMENU)IDC_BTN_ADDFILE, g_hInst, nullptr);
    g_hBtnRemove = CreateWindowExW(0, L"BUTTON", L->btnRemove,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x + 276, by, 100, 26, hParent, (HMENU)IDC_BTN_REMOVE, g_hInst, nullptr);
    g_hBtnConnect = CreateWindowExW(0, L"BUTTON", L->btnConnect,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x + W - 120, by, 120, 26, hParent, (HMENU)IDC_BTN_CONNECT, g_hInst, nullptr);

    SendMessageW(g_hListItems, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    SendMessageW(g_hBtnAddFol, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    SendMessageW(g_hBtnAddFile, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    SendMessageW(g_hBtnRemove, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    SendMessageW(g_hBtnConnect, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
}

void CreateTabLog(HWND hParent, RECT& rc)
{
    int x = rc.left + 8, y = rc.top + 8;
    int W = rc.right - rc.left - 16;
    int H = rc.bottom - rc.top - 16;

    g_hListLog = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER,
        x, y, W, H - 36,
        hParent, (HMENU)IDC_LIST_LOG, g_hInst, nullptr);
    ListView_SetExtendedListViewStyle(g_hListLog,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(g_hListLog, CLR_PANEL);
    ListView_SetTextBkColor(g_hListLog, CLR_PANEL);
    ListView_SetTextColor(g_hListLog, CLR_TEXT);

    struct Col { const wchar_t* n; int w; } cols[] = {
        { L->colTime, 70 }, { L->colProcess, 140 }, { L->colPid, 55 },
        { L->colFolder, 120 }, { L->colFile, 180 }, { L->colDecision, 90 },
    };
    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT; lvc.fmt = LVCFMT_LEFT;
    for (int i = 0; i < 6; i++) {
        lvc.pszText = (PWSTR)cols[i].n; lvc.cx = cols[i].w;
        ListView_InsertColumn(g_hListLog, i, &lvc);
    }

    g_hBtnClear = CreateWindowExW(0, L"BUTTON", L->btnClear,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x + W - 100, y + H - 32, 100, 26,
        hParent, (HMENU)IDC_BTN_CLEAR, g_hInst, nullptr);
    SendMessageW(g_hListLog, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
    SendMessageW(g_hBtnClear, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
}

void CreateTabSettings(HWND hParent, RECT& rc)
{
    int x = rc.left + 16, y = rc.top + 16;

    g_hChkExplorer = CreateWindowExW(0, L"BUTTON", L->optOpenExplorer,
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x, y, 520, 24, hParent, (HMENU)IDC_CHK_EXPLORER, g_hInst, nullptr);
    SendMessageW(g_hChkExplorer, BM_SETCHECK,
        g_OpenExplorerOnBlock ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_hChkExplorer, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

    y += 40;
    HWND hLng = CreateWindowExW(0, L"STATIC", L->optLang,
        WS_CHILD | WS_VISIBLE, x, y + 3, 140, 20,
        hParent, nullptr, g_hInst, nullptr);
    SendMessageW(hLng, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

    g_hComboLang = CreateWindowExW(0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        x + 148, y, 160, 120,
        hParent, (HMENU)IDC_COMBO_LANG, g_hInst, nullptr);
    SendMessageW(g_hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Francais");
    SendMessageW(g_hComboLang, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(g_hComboLang, CB_SETCURSEL, g_LangFR ? 0 : 1, 0);
    SendMessageW(g_hComboLang, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
}

void CreateAllControls(HWND hWnd)
{
    g_hFontUI = CreateFontW(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_hFontMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
    g_hFontBig = CreateFontW(20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right, H = rc.bottom;

    HWND hTitle = CreateWindowExW(0, L"STATIC", L"FolderGuard",
        WS_CHILD | WS_VISIBLE, 12, 10, 200, 26, hWnd, nullptr, g_hInst, nullptr);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontBig, TRUE);

    g_hLblStatus = CreateWindowExW(0, L"STATIC", L->lblStatusOff,
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        W - 210, 14, 202, 20, hWnd, (HMENU)IDC_LABEL_STATUS, g_hInst, nullptr);
    SendMessageW(g_hLblStatus, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

    g_hTab = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | TCS_FLATBUTTONS,
        8, 42, W - 16, H - 50,
        hWnd, (HMENU)IDC_TAB, g_hInst, nullptr);
    SendMessageW(g_hTab, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

    TCITEMW ti = {}; ti.mask = TCIF_TEXT;
    ti.pszText = (PWSTR)L->tabFolders;  TabCtrl_InsertItem(g_hTab, 0, &ti);
    ti.pszText = (PWSTR)L->tabLog;      TabCtrl_InsertItem(g_hTab, 1, &ti);
    ti.pszText = (PWSTR)L->tabSettings; TabCtrl_InsertItem(g_hTab, 2, &ti);

    // Calculer la zone d'affichage du tab control
    // On passe le RECT du tab control en coordonnees client de hWnd,
    // TabCtrl_AdjustRect retourne la zone interieure (sous les onglets)
    RECT tabRc = { 8, 42, W - 16, H - 50 };
    RECT dispRc = tabRc;
    TabCtrl_AdjustRect(g_hTab, FALSE, &dispRc);
    // dispRc est en coordonnees client de hWnd
    int panW = dispRc.right - dispRc.left;
    int panH = dispRc.bottom - dispRc.top;
    RECT panRc = { 0, 0, panW, panH };

    // Enregistrer une classe de panel avec fond sombre
    WNDCLASSEXW pc = {};
    pc.cbSize = sizeof(pc);
    pc.lpfnWndProc = PanelProc;
    pc.hInstance = g_hInst;
    pc.hbrBackground = CreateSolidBrush(CLR_BG);
    pc.lpszClassName = L"FGPanel";
    if (!RegisterClassExW(&pc)) { /* deja enregistree, ok */ }

    for (int i = 0; i < 3; i++) {
        g_hTabPanels[i] = CreateWindowExW(WS_EX_CONTROLPARENT, L"FGPanel", L"",
            WS_CHILD | (i == 0 ? WS_VISIBLE : 0),
            dispRc.left, dispRc.top, panW, panH,
            hWnd, nullptr, g_hInst, nullptr);
    }

    CreateTabItems(g_hTabPanels[0], panRc);
    CreateTabLog(g_hTabPanels[1], panRc);
    CreateTabSettings(g_hTabPanels[2], panRc);

    RefreshItemList();
}

/* ===========================================================================
   TRAY
   =========================================================================== */

void AddTrayIcon(HWND hWnd)
{
    g_nid.cbSize = sizeof(g_nid); g_nid.hWnd = hWnd; g_nid.uID = IDI_TRAY;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(nullptr, IDI_SHIELD);
    StringCchCopyW(g_nid.szTip, 128, L"FolderGuard");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

/* ===========================================================================
   WNDPROC
   =========================================================================== */

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        CreateAllControls(hWnd);
        AddTrayIcon(hWnd);
        return 0;

    case WM_ADD_LOG:
        if (lParam) AddLogEntryUI(reinterpret_cast<LogEntry*>(lParam));
        return 0;

    case WM_NOTIFY: {
        LPNMHDR nm = (LPNMHDR)lParam;
        if (nm->hwndFrom == g_hTab && nm->code == TCN_SELCHANGE) {
            // TabCtrl_GetCurSel est fiable ici car TCN_SELCHANGE est poste apres
            // que la selection soit mise a jour par le tab control
            int idx = TabCtrl_GetCurSel(g_hTab);
            if (idx >= 0 && idx < 3) SwitchTab(idx);
        }
        if (nm->hwndFrom == g_hListLog && nm->code == NM_CUSTOMDRAW) {
            auto* cd = (LPNMLVCUSTOMDRAW)lParam;
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                cd->clrTextBk = CLR_PANEL; cd->clrText = CLR_TEXT;
                return CDRF_NOTIFYSUBITEMDRAW;
            }
            if (cd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
                if (cd->iSubItem == 5) {
                    WCHAR buf[32];
                    ListView_GetItemText(g_hListLog, (int)cd->nmcd.dwItemSpec, 5, buf, 32);
                    cd->clrText = (wcscmp(buf, L->decAllow) == 0) ? CLR_ALLOW : CLR_DENY;
                }
                return CDRF_NEWFONT;
            }
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_ADDFOL: {
            BROWSEINFOW bi = {};
            bi.hwndOwner = hWnd;
            bi.lpszTitle = L"Dossier a proteger";
            bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                WCHAR p[MAX_PATH]; SHGetPathFromIDListW(pidl, p); CoTaskMemFree(pidl);
                AddItem(p, false);
            }
            break;
        }
        case IDC_BTN_ADDFILE: {
            OPENFILENAMEW ofn = {};
            WCHAR path[MAX_PATH] = {};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = path;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = L"Fichier a proteger";
            ofn.lpstrFilter = L"Tous les fichiers\0*.*\0";
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&ofn))
                AddItem(path, true);
            break;
        }
        case IDC_BTN_REMOVE:
            RemoveSelectedItem();
            break;
        case IDC_BTN_CONNECT:
            ConnectToDriver();
            break;
        case IDC_BTN_CLEAR:
            ListView_DeleteAllItems(g_hListLog);
            break;
        case IDC_CHK_EXPLORER:
            g_OpenExplorerOnBlock =
                (SendMessageW(g_hChkExplorer, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveConfig();
            break;
        case IDC_COMBO_LANG:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = (int)SendMessageW(g_hComboLang, CB_GETCURSEL, 0, 0);
                g_LangFR = (sel == 0);
                L = g_LangFR ? &LANG_FR : &LANG_EN;
                ApplyLanguage();
                SaveConfig();
            }
            break;
        case IDM_TRAY_SHOW:
            ShowWindow(hWnd, IsWindowVisible(hWnd) ? SW_HIDE : SW_SHOW);
            break;
        case IDM_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            POINT pt; GetCursorPos(&pt);
            HMENU hm = CreatePopupMenu();
            AppendMenuW(hm, MF_STRING, IDM_TRAY_SHOW, L"Afficher / Show");
            AppendMenuW(hm, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hm, MF_STRING, IDM_TRAY_EXIT, L"Quitter / Exit");
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hm, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
            DestroyMenu(hm);
        }
        if (lParam == WM_LBUTTONDBLCLK) ShowWindow(hWnd, SW_RESTORE);
        return 0;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam; HWND hCtl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, (hCtl == g_hLblStatus)
            ? ((g_hPort != INVALID_HANDLE_VALUE) ? CLR_ALLOW : CLR_DENY)
            : CLR_TEXT);
        return (LRESULT)CreateSolidBrush(CLR_BG);
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam; RECT r; GetClientRect(hWnd, &r);
        FillRect(hdc, &r, CreateSolidBrush(CLR_BG)); return 1;
    }

    case WM_DESTROY:
        g_Running = false;
        if (g_hPort != INVALID_HANDLE_VALUE) {
            CancelIoEx(g_hPort, nullptr);
            CloseHandle(g_hPort);
        }
        StopDriverService();
        { std::lock_guard<std::mutex> lk(g_LogFileMutex); g_LogFile.close(); }
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

/* ===========================================================================
   WINMAIN
   =========================================================================== */

int APIENTRY wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE,
    _In_ LPWSTR, _In_ int nCmdShow)
{
#ifdef _DEBUG
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
#endif

    g_hInst = hInst;

    // Verifier admin
    BOOL isAdmin = FALSE;
    HANDLE hTok = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok)) {
        TOKEN_ELEVATION elev; DWORD sz = sizeof(elev);
        if (GetTokenInformation(hTok, TokenElevation, &elev, sz, &sz))
            isAdmin = elev.TokenIsElevated;
        CloseHandle(hTok);
    }
    if (!isAdmin) {
        MessageBoxW(nullptr, LANG_FR.errAdmin, L"FolderGuard", MB_ICONERROR);
        return 1;
    }

    // Dossier de l'exe
    WCHAR exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    g_AppDir = exePath;

    // Charger config
    LoadConfig();
    L = g_LangFR ? &LANG_FR : &LANG_EN;

    // Demarrer le driver automatiquement
    StartDriverService();
    Sleep(500); // Laisser le temps au service de demarrer

    INITCOMMONCONTROLSEX icc = { sizeof(icc),
        ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    CoInitialize(NULL);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(CLR_BG);
    wc.lpszClassName = L"FolderGuardWnd";
    wc.hIcon = LoadIconW(nullptr, IDI_SHIELD);
    RegisterClassExW(&wc);

    g_hWnd = CreateWindowExW(WS_EX_APPWINDOW,
        L"FolderGuardWnd", L->title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 960, 600,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    CoUninitialize();
    return (int)m.wParam;
}