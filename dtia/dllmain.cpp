// dllmain.cpp : Defines the entry point for the DLL application.

//#include <windows.h>
#include "framework.h"
#include <cstdio>
#include <memory>
#include <clocale>
#include <cassert>
#include <crtdbg.h>

#include <wchar.h>
#include <strsafe.h>
#include <algorithm>
#include <CommCtrl.h>
#include "mdivs.h"
#include "resource.h"
#include <ktmw32.h>
#include <winuser.h>

#include "../agshelpers/agshelpers.h"
#define MDVISLIBAPI __declspec(dllexport)
#include "mdivslib.h"

#pragma data_seg("Shared")
HHOOK gs_hook = nullptr;
DWORD gs_dwThreadIdMDIVS = 0;
#pragma data_seg()
#pragma comment(linker, "/section:Shared,rws")

static const TCHAR g_mainmonregkey[] =
    TEXT("Software\\AGS\\Multi desktop item view saver");

HINSTANCE g_hinst = nullptr;
HANDLE g_threadheap = nullptr;
DDRegistryExtension* g_pdesktop;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hinst = hModule;
        g_threadheap = HeapCreate(0, 16384, 0);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        HeapDestroy(g_threadheap);
        break;
    }
    return TRUE;
}

LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI Dlg_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DDRegistryExtension* DDRegistryExtension::instance = nullptr;

DDRegistryExtension* DDRegistryExtension::getDDRegExt() {
    if (instance == nullptr) {
        instance = static_cast<DDRegistryExtension*>(HeapAlloc(g_threadheap, 0, sizeof(DDRegistryExtension)));
        if (instance) instance = new(instance)DDRegistryExtension();
    }
    return instance;
}

void DDRegistryExtension::closeDDRegExt() {
    if (instance) {
        instance->~DDRegistryExtension();
        HeapFree(g_threadheap, 0, instance);
        instance = nullptr;
    }
}

DDRegistryExtension::DDRegistryExtension() 
    : DesktopDisplays()
{
    if (valid) retriveCurrentProfileIndexFromRegistry();
    else index = 0, subkeysnum = 0, keyexist = false;
}

DDRegistryExtension::DDRegistryExtension(int regnum) 
    : DesktopDisplays(regnum)
{
    index = regnum;
    subkeysnum = 0;
    TCHAR keypath[63];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);

    HKEY hkey;
    LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER, keypath,
        0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkey);
    if (status == ERROR_SUCCESS) {
        keyexist = true;

        DWORD regkeysnum;
        RegQueryInfoKeyW(hkey, nullptr, nullptr, nullptr, &regkeysnum, 
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        subkeysnum = regkeysnum;
        RegCloseKey(hkey);

        StringCchPrintfW(keypath + _countof(g_mainmonregkey) - 1, 5, TEXT("\\%03d"), index);
        StringCchPrintfW(keypath + _countof(g_mainmonregkey) + 3, 17, TEXT("\\DesktopMonitors"));

        status = RegOpenKeyEx(HKEY_CURRENT_USER, keypath, 0, KEY_QUERY_VALUE, &hkey);
        if (status == ERROR_SUCCESS) {
            DWORD values;
            RegQueryInfoKeyW(hkey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                &values, nullptr, nullptr, nullptr, nullptr);

            moncount = values - 1;
            allocateMonitors();

            MonitorRects mr;
            TCHAR keyvalue[5];
            DWORD length, mrsize;

            for (unsigned i = 0, monindex = 0; i < values;) {
                length = 5;
                mrsize = sizeof(MonitorRects);
                status = RegEnumValueW(hkey, i++, keyvalue, &length, nullptr, nullptr, (PBYTE)&mr, &mrsize);
                if (!wcscmp(keyvalue, TEXT("full")))
                    fulldesktop = *((RECT*)&mr);
                else
                    monitors[monindex++] = mr;
            }
            RegCloseKey(hkey);
            valid = true;
        }
    }
    else 
        keyexist = false;
}

void DDRegistryExtension::retriveCurrentProfileIndexFromRegistry() {
    HKEY hkey;
    TCHAR keypath[_countof(g_mainmonregkey) + 20];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);

    LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER, keypath,
        0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkey);
    index = 1;
    subkeysnum = 0;
    keyexist = false;
    if (status == ERROR_SUCCESS) {
        TCHAR keyvalue[4];
        DWORD regkeysnum;
        
        RegQueryInfoKeyW(hkey, nullptr, nullptr, nullptr, &regkeysnum, 
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        subkeysnum = regkeysnum;
        if (subkeysnum == 0) return;
        int* configs = static_cast<int*>(
            HeapAlloc(g_threadheap, 0, sizeof(int) * subkeysnum));
        if (configs == 0) return;
        for (int i = -1; i < (int)(subkeysnum - 1);) {
            RegEnumKeyW(hkey, ++i, keyvalue, 4);
            configs[i] = wchardecimaltoint(keyvalue);
            if (*this == DDRegistryExtension(configs[i])) {
                index = configs[i];
                keyexist = true;
                HeapFree(g_threadheap, 0, configs);
                RegCloseKey(hkey);
                return;
            }
        }
        std::sort(configs, configs + subkeysnum);
        unsigned char min = 0;
        std::find_if(configs, configs + subkeysnum, [&min](int keyind) { return ++min != keyind; });
        if (min == *(configs + subkeysnum - 1) || subkeysnum == 0) ++min;
        index = min;
        HeapFree(g_threadheap, 0, configs);
        RegCloseKey(hkey);
    }
}

void DDRegistryExtension::refreshData() {
    this->DesktopDisplays::refreshData();
    if (valid) retriveCurrentProfileIndexFromRegistry();
    else index = 0, subkeysnum = 0, keyexist = false;
}

LSTATUS DDRegistryExtension::saveCurrentDesktopMetricsToRegistry() const {
    HKEY hkey;
    TCHAR keypath[_countof(g_mainmonregkey) + 20];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);
    StringCchPrintfW(keypath + _countof(g_mainmonregkey) - 1, 5, TEXT("\\%03d"), index);
    StringCchPrintfW(keypath + _countof(g_mainmonregkey) + 3, 17, TEXT("\\DesktopMonitors"));
    
    HANDLE ht = CreateTransaction(nullptr, 0, 0, 0, 0, 50, nullptr);
    LSTATUS status = RegCreateKeyTransactedW(HKEY_CURRENT_USER, keypath, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hkey, nullptr, ht, nullptr);
    if (status == ERROR_SUCCESS) {
        TCHAR imon[4];
        for (int i = 0; i < moncount; ++i) {
            StringCchPrintfW(imon, 4, TEXT("%03d"), i + 1);
            RegSetValueEx(hkey, imon, 0, REG_BINARY, (PBYTE)&monitors[i], sizeof(MonitorRects));
        }
        if (status == ERROR_SUCCESS) {
            RegSetValueEx(hkey, TEXT("full"), 0, REG_BINARY, (PBYTE)&fulldesktop, sizeof(RECT));
            if (CommitTransaction(ht) == 0) status = -1;
        }
        RegCloseKey(hkey);
    }
    CloseHandle(ht);
    return status;
}

LRESULT saveCurrenDesktopItems() {
    HWND hdesktopwnd = g_pdesktop->getDesktopHandle();
    int index = g_pdesktop->getRegistryIndex();

    int itemsnum = ListView_GetItemCount(hdesktopwnd);

    HKEY hkey;
    TCHAR keypath[_countof(g_mainmonregkey) + 4];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);
    StringCchPrintfW(keypath + _countof(g_mainmonregkey) - 1, 5, TEXT("\\%03d"), index);

    TCHAR valName[MAX_PATH];
    DWORD valNameLength = MAX_PATH;
    POINT pt;

    HANDLE ht = CreateTransaction(nullptr, 0, 0, 0, 0, 50, nullptr);
    LSTATUS status = RegOpenKeyTransactedW(HKEY_CURRENT_USER, keypath,
        0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkey, ht, nullptr);

    if (status == ERROR_SUCCESS) {
        DWORD valueslen;
        RegQueryInfoKeyA(hkey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            &valueslen, nullptr, nullptr, nullptr, nullptr);

        for (unsigned i = 0; i < valueslen; ++i) {
            RegEnumValueW(hkey, 0, valName, &valNameLength, nullptr, nullptr,
                nullptr, nullptr);
            RegDeleteValueW(hkey, valName);
            valNameLength = MAX_PATH;
        }
        if (CommitTransaction(ht) == 0) status = -1;
        RegCloseKey(hkey);
        
        if (status == ERROR_SUCCESS) {
            CloseHandle(ht);
            ht = CreateTransaction(nullptr, 0, 0, 0, 0, 50, nullptr);
            LSTATUS status = RegOpenKeyTransactedW(HKEY_CURRENT_USER, keypath,
                0, KEY_SET_VALUE, &hkey, ht, nullptr);
            for (int i = 0; i < itemsnum; ++i) {
                ListView_GetItemText(hdesktopwnd, i, 0, valName, MAX_PATH);
                ListView_GetItemPosition(hdesktopwnd, i, &pt);
                RegSetValueEx(hkey, valName, 0, REG_BINARY, (PBYTE)&pt, sizeof(pt));
            }
            if (CommitTransaction(ht) == 0) status = -1;
            RegCloseKey(hkey);
        }
    }
    CloseHandle(ht);
    return status;
}

struct entry {
    PTCHAR name;
    POINT pt;
};

int sortcompx(const void* p1, const void* p2) {
    if (((entry*)p1)->pt.x > ((entry*)p2)->pt.x) return 1;
    else if (((entry*)p1)->pt.x < ((entry*)p2)->pt.x) return -1;
    else return 0;
}

int sortcompy(const void* p1, const void* p2) {
    if (((entry*)p1)->pt.y > ((entry*)p2)->pt.y) return 1;
    else if (((entry*)p1)->pt.y < ((entry*)p2)->pt.y) return -1;
    else return 0;
}

LRESULT restoreLayoutFromProfile(unsigned char index) {
    HKEY hkey;
    TCHAR keypath[_countof(g_mainmonregkey) + 4];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);
    StringCchPrintfW(keypath + _countof(g_mainmonregkey) - 1, 5, TEXT("\\%03d"), index);

    DWORD maxpath = MAX_PATH;
    WCHAR valName[MAX_PATH];

    LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER, keypath, 0, KEY_QUERY_VALUE, &hkey);

    if (status == ERROR_SUCCESS) {
        HWND hdesktop = g_pdesktop->getDesktopHandle();
        LONG dwStyle = GetWindowLongW(hdesktop, GWL_STYLE);
        if (dwStyle & LVS_AUTOARRANGE)
            SetWindowLong(hdesktop, GWL_STYLE, dwStyle & ~LVS_AUTOARRANGE);

        // Query values count from registry key associated with profile
        DWORD valuescount;
        RegQueryInfoKeyW(hkey, nullptr, nullptr, nullptr, nullptr, nullptr, 
            nullptr, &valuescount, nullptr, nullptr, nullptr, nullptr);

        entry* entries = (entry*)HeapAlloc(g_threadheap, 0, valuescount * sizeof(entry));
        unsigned actualvaluescount = 0;

        POINT pt, pt2;
        DWORD type, size = sizeof(pt);

        // Buffering data with validating entries
        for (int i = 0; i < valuescount; ++i) {
            DWORD maxpath = MAX_PATH;
            WCHAR valName[MAX_PATH];
            RegEnumValue(hkey, i, valName, &maxpath,
                nullptr, &type, (PBYTE)&pt, &size);
            if ((type == REG_BINARY) && (size == sizeof(pt))) {
                entries[i].name = (TCHAR*)HeapAlloc(g_threadheap, 0, sizeof(TCHAR)*(maxpath + 1));
                StringCchCopyW(entries[i].name, maxpath + 1, valName);
                entries[i].pt = pt;
                ++actualvaluescount;
            }
            size = sizeof(pt);
            maxpath = MAX_PATH;
        }
        
        // Sorting entries for displaying it coordinate squence.
        // If we will not do this, we may run into overlapping of
        // existing item desktop disposition with loaded one,
        // next it can shift nearby item that was already aligned.
        // As a result our items may be displaced from their
        // saved positions replaced with chaotic ones.
        qsort(entries, actualvaluescount, sizeof(entry), sortcompx);
        for (int i = 0; i < actualvaluescount;) {
            entry* estart = entries + i;
            int currenty = entries[i].pt.x;
            int ynum = 1;

            while (currenty == entries[++i].pt.x)
                ++ynum;
            qsort(estart, ynum, sizeof(entry), sortcompy);
        }

        DesktopDisplays::MonitorRects mr;
        g_pdesktop->getCornerMonitorRects(DesktopDisplays::Corner::SOUTH_EAST, &mr);
        pt = { mr.workArea.right - 10, mr.workArea.bottom - 10 };

        int placedright = 0, placedrightprev, itemindex;

        LV_FINDINFO lvfi;
        lvfi.flags = LVFI_STRING;
        
        // Main arranging cycle
        for (;;) {
            for (int i = placedright; i < actualvaluescount; ++i) {

                // First we need to find every saved item on the desktop
                lvfi.psz = entries[i].name;
                itemindex = ListView_FindItem(hdesktop, -1, &lvfi);

                // There is a somekind of bug in ListView_FindItem with
                // certain names of items. It considers that they are the
                // same when they are not. Im not actually sure what name's 
                // pattern lead to such behavior, but ListView_GetItemText 
                // always generates accurate results.
                while (itemindex != -1) {
                    ListView_GetItemText(hdesktop, itemindex, 0, valName, maxpath);
                    if (!lstrcmpW(entries[i].name, valName)) break;
                    itemindex = ListView_FindItem(hdesktop, itemindex, &lvfi);
                }

                // If item is found we set its coordinates to the south-east corner
                // that allow us clear desktop from random interference of
                if (itemindex != -1)
                    ListView_SetItemPosition(hdesktop, itemindex, pt.x, pt.y);
            }
            
            // Now we start placing items according their sorted positions
            for (int i = placedright; i < actualvaluescount; ++i) {
                lvfi.psz = entries[i].name;
                itemindex = ListView_FindItem(hdesktop, -1, &lvfi);

                // Here we making some workaround with aforementioned ListView_FindItem bug
                while (itemindex != -1) {
                    ListView_GetItemText(hdesktop, itemindex, 0, valName, maxpath);
                    if (!lstrcmpW(entries[i].name, valName)) break;
                    itemindex = ListView_FindItem(hdesktop, itemindex, &lvfi);
                }

                // Finally we placing our item on its legal place
                if (itemindex != -1)
                    ListView_SetItemPosition(hdesktop, itemindex, entries[i].pt.x, entries[i].pt.y);
            }
            
            // If there is a plenty of items on the desktop and
            // some of them maybe were not saved before we can
            // get another collision. So for extra precisety
            // we need to check current coordinates of items one by one
            placedrightprev = placedright;
            POINT pt2;
            DWORD ptsize = sizeof(POINT);
            
            while (placedright < actualvaluescount) {
                RegQueryValueExW(hkey, entries[placedright].name, NULL, NULL, (LPBYTE)&pt, &ptsize);
                lvfi.psz = entries[placedright].name;
                itemindex = ListView_FindItem(hdesktop, -1, &lvfi);

                while (itemindex != -1) {
                    ListView_GetItemText(hdesktop, itemindex, 0, valName, maxpath);
                    if (!lstrcmpW(entries[placedright].name, valName)) break;
                    itemindex = ListView_FindItem(hdesktop, itemindex, &lvfi);
                }
                ListView_GetItemPosition(hdesktop, itemindex, &pt2);

                // Here we compare current desktop coordinates of item and saved in the registry
                // if they correct, we incrementing total right placed items value
                // if the not, we need to break current cycle and start arranging 
                // again begining with first wrong placed item
                if (pt.x != pt2.x || pt.y != pt2.y) break;
                ++placedright;
            }
            
            // If we do another full cycle and our new arrangement check again stops on the
            // same item, there is nothing we can do, with it, and next cycle we start
            // with following item
            if (placedright == placedrightprev) ++placedright;

            // If we came to the last item, we're done
            if (placedright == actualvaluescount) break;
        }

        for (int i = 0; i < actualvaluescount; ++i)
            HeapFree(g_threadheap, 0, entries[i].name);
        HeapFree(g_threadheap, 0, entries);

        SetWindowLong(hdesktop, GWL_STYLE, dwStyle);
    }
    RegCloseKey(hkey);
    return status;
}

struct CheckSavedProfileResult {
    LSTATUS status;
    bool result;
};

CheckSavedProfileResult isThereAnySavedProfilesInRegistry() {
    HKEY hkey;
    TCHAR keypath[_countof(g_mainmonregkey)];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);

    DWORD subkeysnum = 0;
    bool result = false;
    LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER, keypath,
        0, KEY_QUERY_VALUE, &hkey);
    
    if (status == ERROR_SUCCESS) {
        status = RegQueryInfoKeyW(hkey, nullptr, nullptr, nullptr, &subkeysnum, 
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        RegCloseKey(hkey);
        if (subkeysnum) result = true;
    }
    return { status, result };
}

LSTATUS deleteSavedProfile(unsigned char index) {
    HKEY hkey;
    TCHAR keypath[_countof(g_mainmonregkey) + 4];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);
    StringCchPrintfW(keypath + _countof(g_mainmonregkey) - 1, 5, TEXT("\\%03d"), index);

    HANDLE ht = CreateTransaction(nullptr, 0, 0, 0, 0, 50, nullptr);
    LSTATUS status = RegOpenKeyTransactedW(HKEY_CURRENT_USER, keypath,
        0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkey, ht, nullptr);

    if (status == ERROR_SUCCESS) {
        RegDeleteTreeW(hkey, nullptr);
        RegDeleteKeyTransactedW(HKEY_CURRENT_USER, keypath, KEY_WOW64_64KEY, 0, ht, nullptr);
        if (CommitTransaction(ht) == 0) status = -1;
        RegCloseKey(hkey);
    }
    CloseHandle(ht);
    return status;
}

LSTATUS deleteAllSavedPrifilesFromRegistry() {
    HKEY hkey;
    TCHAR keypath[_countof(g_mainmonregkey) + 4];
    StringCchCopyW(keypath, _countof(g_mainmonregkey), g_mainmonregkey);

    HANDLE ht = CreateTransaction(nullptr, 0, 0, 0, 0, 50, nullptr);
    LSTATUS status = RegOpenKeyTransactedW(HKEY_CURRENT_USER, keypath,
        0, KEY_SET_VALUE, &hkey, ht, nullptr);

    if (status == ERROR_SUCCESS) {
        RegDeleteTreeW(hkey, nullptr);
        if (CommitTransaction(ht) == 0) status = -1;
        RegCloseKey(hkey);
    }
    CloseHandle(ht);
    return status;
}

DesktopDisplays* WINAPI setHookToDesktopWindow() {    
    if (gs_hook == 0) {
        g_pdesktop = reinterpret_cast<DDRegistryExtension*>(DesktopDisplays::getDD());
        HANDLE ttt = g_threadheap;
        HWND hdesktopwnd = g_pdesktop->getDesktopHandle();
        if (!IsWindow(hdesktopwnd)) return nullptr;

        DWORD dwThreadID = GetWindowThreadProcessId(hdesktopwnd, nullptr);
        if (dwThreadID == 0) return nullptr;
        
        gs_dwThreadIdMDIVS = GetCurrentThreadId();
        gs_hook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, g_hinst, dwThreadID);
        if (gs_hook) {
            PostThreadMessage(dwThreadID, WM_NULL, 0, 0);
            return g_pdesktop;
        }
        else
            return nullptr;
    }
    else {
        DDRegistryExtension::closeDDRegExt();
        UnhookWindowsHookEx(gs_hook);
        return nullptr;
    }
}

LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
    static bool hooked = false;

    if (!hooked) {
        hooked = true;
        CreateDialog(g_hinst, MAKEINTRESOURCE(HOOK_DIALOG), NULL, Dlg_Proc);
    }
    return(CallNextHookEx(gs_hook, nCode, wParam, lParam));
}

INT_PTR WINAPI Dlg_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LSTATUS status;

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        g_pdesktop = DDRegistryExtension::getDDRegExt();
        PostThreadMessage(gs_dwThreadIdMDIVS, WM_NULL, !g_pdesktop->isValid(),
            ((g_pdesktop->getRegistrySubkeysNum())?1:0) + (int)g_pdesktop->isRegistryKeyExist());
        break;
    }
    case WM_NULL:
    {
        switch (wParam) {
        case WM_SAVE_ARRANGEMENT: {
            status = g_pdesktop->saveCurrentDesktopMetricsToRegistry();
            if (status == ERROR_SUCCESS)
                saveCurrenDesktopItems();
            PostThreadMessage(gs_dwThreadIdMDIVS, WM_NULL, WM_SAVED, 0);
        }
            break;
        case WM_LOAD_ARRANGEMENT:
            restoreLayoutFromProfile(g_pdesktop->getRegistryIndex());
            break;
        case WM_DELETE_PROFILE:
            deleteSavedProfile(g_pdesktop->getRegistryIndex());
            g_pdesktop->retriveCurrentProfileIndexFromRegistry();
            if (g_pdesktop->getRegistrySubkeysNum())
                PostThreadMessage(gs_dwThreadIdMDIVS, WM_NULL, WM_DELETED, 0);
            else
                PostThreadMessage(gs_dwThreadIdMDIVS, WM_NULL, WM_CLEARED, 0);
            break;
        case WM_DELETE_ALL_PROFILES:
            deleteAllSavedPrifilesFromRegistry();
            g_pdesktop->retriveCurrentProfileIndexFromRegistry();
            PostThreadMessage(gs_dwThreadIdMDIVS, WM_NULL, WM_CLEARED, 0);
            break;
        case WM_POST_DISPLAY_CHANGE_WORKAROUND:
        {
            // When display configuration changes and WM_DISPLAYCHANGE message is received
            // GetWindowRect function writes to RECT struct old values with 50% chance.
            // So, before update DDRegistryExtension data, we need to check that 
            // we will get actually acurate settings.
            RECT test;
            GetWindowRect(g_pdesktop->getDesktopHandle(), &test);
            if (test == g_pdesktop->getFullDesktop()) {
                PostMessage(hWnd, WM_NULL, WM_POST_DISPLAY_CHANGE_WORKAROUND, 0);
                break;
            }
            g_pdesktop->refreshData();
            PostThreadMessage(gs_dwThreadIdMDIVS, WM_NULL, WM_DISPLAY_CHANGE_EVENT_SIGNALED,
                ((g_pdesktop->getRegistrySubkeysNum()) ? 1 : 0) + (int)g_pdesktop->isRegistryKeyExist() + 5);
            break;
        }
        }
        break;
    }
    case WM_DISPLAYCHANGE:
        PostMessage(hWnd, WM_NULL, WM_POST_DISPLAY_CHANGE_WORKAROUND, 0);
        break;
    case WM_CLOSE:
        DDRegistryExtension::closeDDRegExt();
        DestroyWindow(hWnd);
        break;
    }
    return FALSE;
}
