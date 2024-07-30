/*
	Copyright (C) 2024  Masoud Naservand

	This file is part of irancal.

	irancal is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	irancal is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with irancal.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef UNICODE
#define UNICODE
#endif
#define _POSIX_C_SOURCE 1

#include <windows.h>
#include <winuser.h>
#include <windowsx.h>
#include <strsafe.h>
#include <stdio.h>
#include <time.h>

#include "jalali.h"
#include "jtime.h"
/*
 * How it works:
 *  check every WM_TIME proc if last_day != current_day
 *  and update tooltip string if yes.
 *  also decide on how long next WM_TIME should be.
 */

/* Useful to change globals: */
#define DATE_STRING_LENGTH 64 //How long is the tooltip we want to show
BOOLEAN PERSIAN = TRUE; //Change to FALSE if you want English as default
#define PERSIAN_FORMAT "%W %G" // refer to jtime.c or jdate format docs
#define ENGLISH_FORMAT "%F %q"
#define CHECK_INTERVAL 3600000 // 1 hour
/* ------------------------ */


wchar_t date_string[DATE_STRING_LENGTH]; //this is what the mouse hover tooltip shows
char utf8_date_string[DATE_STRING_LENGTH]; //holds what we get from libjalali
int last_day;  // day was this last time we checked the date.
time_t t;
struct tm tm;
HMENU menu;
NOTIFYICONDATA nid;

// these are random value, apparently that's how
// it works. You just come up with something random
// and hope it doesn't clash with something else
const wchar_t g_szClassName[] = L"MyHopefullyUniqueWindowClass";
#define ID_MENU_EXIT 9001  
#define MY_TRAY_ICON_MESSAGE 0xBF00
#define IDT_TIMER1 9009


// updates it everytime called
// expects you to call time(&t) before calling it
void update_notification()
{
    struct jtm *j = jlocaltime(&t);
    jstrftime(utf8_date_string, DATE_STRING_LENGTH-1,
                (PERSIAN) ? PERSIAN_FORMAT : ENGLISH_FORMAT, j);
    MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                utf8_date_string, -1, date_string, DATE_STRING_LENGTH);

    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), date_string);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

BOOLEAN should_update()
{
    time(&t);
    localtime_r(&t, &tm);
    if (last_day != tm.tm_yday) {
        last_day = tm.tm_yday;
        return TRUE;
    }
    return FALSE;
}

void init_notification(HWND hwnd)
{
    /* https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa */
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;

    nid.uID = 1;
    nid.uFlags = NIF_TIP | NIF_ICON | NIF_SHOWTIP | NIF_MESSAGE;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), date_string);
    nid.uVersion = NOTIFYICON_VERSION_4;
    nid.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

    Shell_NotifyIcon(NIM_ADD, &nid);
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void init_menu()
{
    menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, ID_MENU_EXIT,
            PERSIAN ? L"خروج" : L"E&xit");
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case MY_TRAY_ICON_MESSAGE:
            switch(LOWORD(lParam))
            {
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_VERNEGANIMATION,
                            GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam), 0, hwnd, NULL);
                break;
            }
        break;

        case WM_COMMAND:
            switch(wParam) {
                case ID_MENU_EXIT:
                    PostQuitMessage(0);
                break;
            }
        break;

        case WM_TIMER:
            switch(wParam) {
                case IDT_TIMER1:
                    if (should_update()) {
                        update_notification();
                    }
                    int interval = CHECK_INTERVAL;
                    if (tm.tm_hour == 23) {
                        interval /= 60; //in the last hour, check every minute
                    }
                    SetTimer(hwnd, IDT_TIMER1, interval, NULL);
                break;
            }
        break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
        break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;

    wc.cbSize           = sizeof(WNDCLASSEX);
    wc.style            = 0;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = g_szClassName;
    wc.hIconSm          = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!",
                MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            g_szClassName,
            L"Iran Solar Hijri Calendar",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
            NULL, NULL, hInstance, NULL);
    if(hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
                MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    init_menu();
    init_notification(hwnd);
    should_update();
    update_notification();

    SetTimer(hwnd, IDT_TIMER1, 60000, NULL); // start with 60 secs

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    return 0;
}
