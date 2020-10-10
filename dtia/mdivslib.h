#pragma once

#if !defined(MDVISLIBAPI)
#define MDVISLIBAPI __declspec(dllimport)
#endif

constexpr int WM_HOOK_READY = WM_APP;
constexpr int WM_SAVE_ARRANGEMENT = WM_APP + 1;
constexpr int WM_LOAD_ARRANGEMENT = WM_APP + 2;
constexpr int WM_DELETE_PROFILE = WM_APP + 3;
constexpr int WM_DELETE_ALL_PROFILES = WM_APP + 4;
constexpr int WM_SAVED = WM_APP + 5;
constexpr int WM_DELETED = WM_APP + 6;
constexpr int WM_CLEARED = WM_APP + 7;
constexpr int WM_DISPLAY_CHANGE_EVENT_SIGNALED = WM_APP + 8;
constexpr int WM_POST_DISPLAY_CHANGE_WORKAROUND = WM_APP + 9;

MDVISLIBAPI DesktopDisplays* WINAPI setHookToDesktopWindow();
