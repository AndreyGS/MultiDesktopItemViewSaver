#include <Windows.h>
#include <WindowsX.h>
#include "../agshelpers/agshelpers.h"
#include "../dtia/mdivslib.h"
#include "resource.h"
#include <strsafe.h>
#include <process.h>

HWND g_hhookwnd = NULL;
HWND g_hdesktop = NULL;
DWORD g_dwhmainappthread = NULL;

INT_PTR WINAPI Dlg_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(_In_ HINSTANCE hInstExe, _In_opt_ HINSTANCE, _In_ PTSTR pszCmdLine, _In_ int) {
    DesktopDisplays* dd;
    // First we need to set hook function, from mdivs.dll to explorer
    if ((dd = setHookToDesktopWindow()) == nullptr)
        exit(1);
    
    // Second, we need to wait, while injected function create a server 
    // as modeless dialog which sends us a message that it ready for action.
    MSG msg;
    int i;
    for (i = 0;i < 50; ++i) {
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (msg.message == WM_NULL) break;
        Sleep(20);
    }

    g_hhookwnd = FindWindow(NULL, TEXT("MultiDIVS"));
    if (!IsWindow(g_hhookwnd))
        exit(1);
    
    g_dwhmainappthread = GetCurrentThreadId();
    // Then we create main user dialog window
    CreateDialogParamW(hInstExe, MAKEINTRESOURCE(IDD_MDIVSAPP), NULL, Dlg_Proc, msg.lParam);
    HWND hdialog = FindWindow(nullptr, TEXT("Multi Desktop Item View Saver"));
    
    for (;;) {
        GetMessage(&msg, 0, 0, 0);
        
        if (msg.message == WM_NULL) {
            switch (msg.wParam) {
            case WM_SAVED:
                if (msg.lParam == 0) {
                    EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), true);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETE), true);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETEALL), true);
                }
                else
                    MessageBox(hdialog, TEXT("Something happens wrong. Saving error."),
                        TEXT("Error completing operation"), MB_OK);
                break;
            case WM_DELETED:
                EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), false);
                EnableWindow(GetDlgItem(hdialog, BTN_DELETE), false);
                break;
            case WM_CLEARED:
                EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), false);
                EnableWindow(GetDlgItem(hdialog, BTN_DELETE), false);
                EnableWindow(GetDlgItem(hdialog, BTN_DELETEALL), false);
                break;
            case WM_DISPLAY_CHANGE_EVENT_SIGNALED:
            {
                RECT rt;
                DesktopDisplays::MonitorRects mrs;
                if (msg.lParam > 2) {
                    dd->refreshData(); msg.lParam -= 5;
                }
                int i = dd->getPrimaryMonitorIndex();
                GetWindowRect(hdialog, &rt);
                dd->getMonitorRects(i, &mrs);
                MoveWindow(hdialog,
                    abs(mrs.workArea.right - mrs.workArea.left) / 2 - abs(rt.right - rt.left) / 2 + 0.5,
                    abs(mrs.workArea.bottom - mrs.workArea.top) / 2 - abs(rt.bottom - rt.top) / 2 + 0.5,
                    abs(rt.right - rt.left), abs(rt.bottom - rt.top), FALSE);
                
                // There is no any saved profiles yet
                if (msg.lParam == 0) {
                    EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), false);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETEALL), false);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETE), false);
                // There is no saved profile for current desktop configuration
                } else if (msg.lParam == 1) {
                    EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), false);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETE), false);
                // There is a profile for this virtual monitor settings
                } else {
                    EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), true);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETE), true);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETEALL), true);
                }
                ShowWindow(hdialog, SW_SHOW);
                SetFocus(hdialog);
            }
                break;
            }
        }
        else if (msg.message == WM_QUIT) {
            DispatchMessageW(&msg);
            break;
        }
        DispatchMessageW(&msg);
    }

    SendMessage(g_hhookwnd, WM_CLOSE, 0, 0);
    // Before unhooking we need to make sure that all memory cleanup
    // on the explorer side is made and server window is closed
    for (int i = 0; i < 50; ++i) {
        if (!IsWindow(g_hhookwnd)) break;
        Sleep(20);
    }
    setHookToDesktopWindow();

    return 0;
}

INT_PTR WINAPI Dlg_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
    {
        PostThreadMessage(g_dwhmainappthread, WM_NULL, WM_DISPLAY_CHANGE_EVENT_SIGNALED, lParam);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case BTN_SAVE:
            g_hhookwnd = FindWindow(NULL, TEXT("MultiDIVS"));
            if (MessageBox(hWnd, TEXT("Are you sure want to save current items layout?"),
                TEXT("Confirmation required"), MB_YESNO) == IDYES)
                    SendMessage(g_hhookwnd, WM_NULL, WM_SAVE_ARRANGEMENT, 0);
            return TRUE;
        case BTN_RESTORE_PROFILE:
            if (MessageBox(hWnd, TEXT("Restore previous saved desktop items layout?"),
                TEXT("Confirmation required"), MB_YESNO) == IDYES)
                    SendMessage(g_hhookwnd, WM_NULL, WM_LOAD_ARRANGEMENT, 0);
            return TRUE;
        case BTN_DELETEALL:
            if (MessageBox(hWnd, TEXT("Current action will destroy all saved profiles."),
                TEXT("Are you sure?"), MB_YESNO) == IDYES)
                    SendMessage(g_hhookwnd, WM_NULL, WM_DELETE_ALL_PROFILES, 0);
            return TRUE;
        case BTN_DELETE:
            if (MessageBox(hWnd, TEXT("This action will delete saved profile for current desktop configuration."),
                TEXT("Are you sure?"), MB_YESNO) == IDYES)
                SendMessage(g_hhookwnd, WM_NULL, WM_DELETE_PROFILE, 0);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        PostThreadMessage(g_dwhmainappthread, WM_QUIT, 0, 0);
        DestroyWindow(hWnd);
        return TRUE;
    }
    return FALSE;
}