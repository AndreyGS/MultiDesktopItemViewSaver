// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

#define AGSHELPERSAPI __declspec(dllexport)
#include "agshelpers.h"
#include <memory>

HANDLE g_threadheap = nullptr;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_threadheap = HeapCreate(0, 4096, 0);
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

int wchardecimaltoint(const WCHAR* input) {
    int i = 0, iprev = 0;
    int neg = 0;
    if (*input == '-') { neg = 1; ++input; }
    while (*input >= '0' && *input <= '9') {
        i = i * 10 + *input - '0';
        if ((i - (*input - '0')) / 10 != iprev || iprev > i) {
            if ((i == -i) && neg) return i;
            i = iprev;
            break;
        }
        else
            iprev = i;
        ++input;
    }
    if (neg) i = -i;
    return i;
}

DesktopDisplays* DesktopDisplays::instance = nullptr;
int DesktopDisplays::usage = 0;

DesktopDisplays* DesktopDisplays::getDD() {
    if (instance == nullptr) {
        instance = static_cast<DesktopDisplays*>(HeapAlloc(g_threadheap, 0, sizeof(DesktopDisplays)));
        if (instance) instance = new(instance)DesktopDisplays();
    }
    if (instance) ++usage;
    return instance;
}

void DesktopDisplays::closeDD() {
    if (usage) --usage;
    if (usage == 0 && instance) {
        instance->~DesktopDisplays();
        HeapFree(g_threadheap, 0, instance);
        instance = nullptr;
    }
}

inline DesktopDisplays::DesktopDisplays() { 
    initialize();
}

inline DesktopDisplays::DesktopDisplays(int) 
    : monitors(0), fulldesktop{ 0 }, primmon(0), moncount(0), valid(false) { }

void DesktopDisplays::initialize() {
    valid = true;
    GetWindowRect(getDesktopHandle(), &fulldesktop);
    if ((moncount = GetSystemMetrics(SM_CMONITORS)) == 0) {
        fulldesktop = { 0 };
        monitors = nullptr;
        primmon = 255;
    }
    else
        getDisplaysMetricsFromCurrentDesktop();
}

DesktopDisplays::~DesktopDisplays() {
    cleanHeap();
}

inline void DesktopDisplays::refreshData() {
    cleanHeap();
    initialize();
}

inline void DesktopDisplays::cleanHeap() {
    if (monitors) HeapFree(g_threadheap, 0, monitors);
}

inline HWND DesktopDisplays::getDesktopHandle() const {
    return GetTopWindow(GetTopWindow(FindWindow(TEXT("ProgMan"), NULL)));
}

void DesktopDisplays::getDisplaysMetricsFromCurrentDesktop() {
    HMONITOR* hms = static_cast<HMONITOR*>(HeapAlloc(g_threadheap, 0, sizeof(HMONITOR) * moncount));
    if (hms == 0) {
        valid = false; return;
    }

    POINT pt;
    HMONITOR tempmon;
    bool alreadyadded;
    int addedmons = 0;

    for (pt.y = fulldesktop.top; pt.y < fulldesktop.bottom; pt.y += 480) {
        for (pt.x = fulldesktop.left; pt.x < fulldesktop.right; pt.x += 640) {
            tempmon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
            alreadyadded = false;
            for (int i = 0; i < addedmons; ++i)
                if (hms[i] == tempmon) {
                    alreadyadded = true; break;
                }
            if (alreadyadded) continue;
            hms[addedmons++] = tempmon;
            if (addedmons == moncount) break;
        }
        if (addedmons == moncount) break;
    }

    allocateMonitors();
    if (valid) {
        MONITORINFO mi;
        mi.cbSize = sizeof(MONITORINFO);

        for (int i = 0; i < addedmons; ++i) {
            GetMonitorInfo(hms[i], &mi);
            monitors[i].fullArea = mi.rcMonitor;
            monitors[i].workArea = mi.rcWork;
            if (mi.dwFlags == MONITORINFOF_PRIMARY) primmon = i;
        }
    }
    HeapFree(g_threadheap, 0, hms);
}

inline void DesktopDisplays::allocateMonitors() {
    monitors = static_cast<MonitorRects*>(HeapAlloc(g_threadheap, 0, sizeof(MonitorRects) * moncount));
    if (monitors == nullptr) valid = false;
}

inline unsigned char DesktopDisplays::getPrimaryMonitorIndex() const { return primmon; }
inline unsigned char DesktopDisplays::getMonitorCount() const { return moncount; }
inline RECT DesktopDisplays::getFullDesktop() const { return fulldesktop; }

inline bool DesktopDisplays::getMonitorRects(unsigned char index, DesktopDisplays::MonitorRects* mr) const
{
    if (index >= 0 && index < moncount) {
        *mr = monitors[index];
        return true;
    }
    else
        return false;
}

void DesktopDisplays::getCornerMonitorRects(Corner corner, MonitorRects* mr) const {
    
    if (moncount == 1) {
        *mr = monitors[0];
        return;
    }
    
    int x = monitors[0].workArea.left, y = monitors[0].workArea.top;

    for (int i = 0; i < moncount; ++i) {
        switch (corner) {
        case Corner::NORTH_EAST: case Corner::SOUTH_EAST:
            if (x < monitors[i].workArea.right) x = monitors[i].workArea.right; break;
        case Corner::SOUTH_WEST: case Corner::NORTH_WEST:
            if (x > monitors[i].workArea.left) x = monitors[i].workArea.left; break;
        }
    }
    
    int index;
    for (int i = 0; i < moncount; ++i) {
        switch (corner) {
        case Corner::NORTH_EAST: 
            if (x == monitors[i].workArea.right
                && y >= monitors[i].workArea.top)
            {
                y = monitors[i].workArea.top;
                index = i;
            }
            break;
        case Corner::SOUTH_EAST:
            if (x == monitors[i].workArea.right
                && y <= monitors[i].workArea.bottom)
            {
                y = monitors[i].workArea.bottom;
                index = i;
            }
            break;
        case Corner::SOUTH_WEST: 
            if (x == monitors[i].workArea.left
                && y <= monitors[i].workArea.bottom)
            {
                y = monitors[i].workArea.bottom;
                index = i;
            }
            break;
        case Corner::NORTH_WEST:
            if (x == monitors[i].workArea.left
                && y >= monitors[i].workArea.bottom)
            {
                y = monitors[i].workArea.bottom;
                index = i;
            }
            break;
        }
    }

    *mr = monitors[index];
}

inline bool DesktopDisplays::isValid() const { return valid; }

bool DesktopDisplays::operator==(const DesktopDisplays& dd) const {
    if (fulldesktop == dd.fulldesktop && moncount == dd.moncount) {
        int equals = 0;
        for (int i = 0; i < moncount; ++i) {
            for (int j = 0; j < moncount; ++j) {
                if (monitors[i].fullArea == dd.monitors[j].fullArea
                    && monitors[i].workArea == dd.monitors[j].workArea) {
                    ++equals;
                    break;
                }
            }
        }
        if (equals == moncount) return true;
    }
    return false;
}

inline bool operator==(RECT rt1, RECT rt2) {
    if (rt1.left == rt2.left && rt1.right == rt2.right
        && rt1.top == rt2.top && rt1.bottom == rt2.bottom)
        return true;
    else
        return false;
}