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
    if ((dd = setHookToDesktopWindow()) == nullptr)
        exit(1);
    
    MSG msg;
    for (;;) {
        GetMessage(&msg, NULL, 0, 0);
        if (msg.message == WM_NULL) break;
    }

    g_hhookwnd = FindWindow(NULL, TEXT("MultiDIVS"));
    if (!IsWindow(g_hhookwnd))
        exit(1);
    
    g_dwhmainappthread = GetCurrentThreadId();

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
                        TEXT("Error compliting operation"), MB_OK);
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
                
                if (msg.lParam == 0) {
                    EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), false);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETEALL), false);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETE), false);
                } else if (msg.lParam == 1) {
                    EnableWindow(GetDlgItem(hdialog, BTN_RESTORE_PROFILE), false);
                    EnableWindow(GetDlgItem(hdialog, BTN_DELETE), false);
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
    case WM_NULL:
    case WM_CLOSE:
        PostThreadMessage(g_dwhmainappthread, WM_QUIT, 0, 0);
        DestroyWindow(hWnd);
        return TRUE;
    }
    return FALSE;
}