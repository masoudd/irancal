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
#define STRICT

#include <windows.h>
#include <winuser.h>
#include <windowsx.h>
#include <wingdi.h>
#include <strsafe.h>
#include <stdio.h>
#include <time.h>

#include "jalali.h"
#include "jtime.h"

#define IRANCAL_VERSION "0.3"
#define IRANCAL_VERSION_LEN 5

#ifndef DEBUG
#define DEBUG 0
#endif

/*
 * How it works:
 *  every WM_TIME proc update tooltip string and shown year calendar in window.
 *  also decide on how long next WM_TIME should be.
 */

/* Useful to change globals: */
#define DATE_STRING_LENGTH 64 //How long is the tooltip we want to show
BOOLEAN PERSIAN = TRUE; //Change to FALSE if you want English as default
#define PERSIAN_FORMAT "%W %G" // refer to jtime.c or jdate format docs
#define ENGLISH_FORMAT "%F %q"
#define CHECK_INTERVAL 3600000 // 1 hour
#define WIDTH 600
#define HEIGHT 900
/* ------------------------ */

#define month_buf_length 200 
TCHAR month_buf[month_buf_length];
struct tm tm;


HMENU menu;
NOTIFYICONDATA nid;
HWND static_hwnd_months[12]; // handle to the label (called static in win32 api)
HWND static_hwnd_yeartitle;
HWND static_hwnd_credits;
LPCTSTR jalali_months_en[] = {
        TEXT("Farvardin"), TEXT("Ordibehesht"), TEXT("Khordaad"),
        TEXT("Tir"), TEXT("Mordaad"), TEXT("Shahrivar"),
        TEXT("Mehr"), TEXT("Aabaan"), TEXT("Aazar"),
        TEXT("Dey"), TEXT("Bahman"), TEXT("Esfand")};
LPCTSTR jalali_months_fa[] = {
        TEXT("فروردین"), TEXT("اردیبهشت"), TEXT("خرداد"),
        TEXT("تیر"), TEXT("مرداد"), TEXT("شهریور"),
        TEXT("مهر"), TEXT("آبان"), TEXT("آذر"),
        TEXT("دی"), TEXT("بهمن"), TEXT("اسفند")};
// how many spaces we need before the name of the month in the year table
// + length of the name of the month to give to printf %*s as width
// so that the name of the month is centered on top the 7 days of the week.
LPCTSTR window_title_en = TEXT("Iran Solar Hijri Calendar");
LPCTSTR window_title_fa = TEXT("تقویم هجری شمسی");

// need global handle to the font to call DeleteObject on it on exit.
HFONT hFont;

// these are random value, apparently that's how
// it works. You just come up with something random
// and hope it doesn't clash with something else
const wchar_t g_szClassName[] = L"MyHopefullyUniqueWindowClass";
#define ID_MENU_EXIT 9001  
#define ID_MENU_CHANGE_LANG 9002
#define ID_MENU_TOGGLE_WINDOW 9003
#define MY_TRAY_ICON_MESSAGE 0xBF00
#define IDT_TIMER1 9009

/* This is what I want it to look like at the end.
 * Same as what `jcal -y` prints:
                               1403

     Farvardin             Ordibehesht             Khordaad       
Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo
             1  2  3    1  2  3  4  5  6  7             1  2  3  4
 4  5  6  7  8  9 10    8  9 10 11 12 13 14    5  6  7  8  9 10 11
11 12 13 14 15 16 17   15 16 17 18 19 20 21   12 13 14 15 16 17 18
18 19 20 21 22 23 24   22 23 24 25 26 27 28   19 20 21 22 23 24 25
25 26 27 28 29 30 31   29 30 31               26 27 28 29 30 31   
                                                                  
        Tir                  Mordaad               Shahrivar      
Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo
                   1          1  2  3  4  5                   1  2
 2  3  4  5  6  7  8    6  7  8  9 10 11 12    3  4  5  6  7  8  9
 9 10 11 12 13 14 15   13 14 15 16 17 18 19   10 11 12 13 14 15 16
16 17 18 19 20 21 22   20 21 22 23 24 25 26   17 18 19 20 21 22 23
23 24 25 26 27 28 29   27 28 29 30 31         24 25 26 27 28 29 30
30 31                                         31                  
       Mehr                  Aabaan                  Aazar        
Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo
    1  2  3  4  5  6             1  2  3  4                   1  2
 7  8  9 10 11 12 13    5  6  7  8  9 10 11    3  4  5  6  7  8  9
14 15 16 17 18 19 20   12 13 14 15 16 17 18   10 11 12 13 14 15 16
21 22 23 24 25 26 27   19 20 21 22 23 24 25   17 18 19 20 21 22 23
28 29 30               26 27 28 29 30         24 25 26 27 28 29 30
                                                                  
        Dey                  Bahman                 Esfand        
Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo
 1  2  3  4  5  6  7          1  2  3  4  5                1  2  3
 8  9 10 11 12 13 14    6  7  8  9 10 11 12    4  5  6  7  8  9 10
15 16 17 18 19 20 21   13 14 15 16 17 18 19   11 12 13 14 15 16 17
22 23 24 25 26 27 28   20 21 22 23 24 25 26   18 19 20 21 22 23 24
29 30                  27 28 29 30            25 26 27 28 29      
*/

// Use libjalali api to make a representation of a hijri shamsi month in given year.
// Each month is a table with 7 columns and
// 5 or 6 rows (month_row) depending on length of month and which
// day of the week the 1st day of that month is.
void update_month_buf(int year, int month)
{
    static LPCTSTR week_en = TEXT("Sh Ye Do Se Ch Pa Jo\n");
    static size_t week_len_en = 21; //length of the LPTSTR week
    
    static LPCTSTR week_fa = TEXT("ج  پ  چ  س  د  ی  ش\n");
    static size_t week_len_fa = 20;

    static LPCTSTR numbers_en[] = {TEXT(" 0"), TEXT(" 1"), TEXT(" 2"), TEXT(" 3"), TEXT(" 4"), TEXT(" 5"), TEXT(" 6"), TEXT(" 7"), TEXT(" 8"), TEXT(" 9"), TEXT("10"), TEXT("11"), TEXT("12"), TEXT("13"), TEXT("14"), TEXT("15"), TEXT("16"), TEXT("17"), TEXT("18"), TEXT("19"), TEXT("20"), TEXT("21"), TEXT("22"), TEXT("23"), TEXT("24"), TEXT("25"), TEXT("26"), TEXT("27"), TEXT("28"), TEXT("29"), TEXT("30"), TEXT("31"), TEXT("32")};
    static LPCTSTR numbers_fa[] = {TEXT(" ۰"), TEXT(" ۱"), TEXT(" ۲"), TEXT(" ۳"), TEXT(" ۴"), TEXT(" ۵"), TEXT(" ۶"), TEXT(" ۷"), TEXT(" ۸"), TEXT(" ۹"), TEXT("۱۰"), TEXT("۱۱"), TEXT("۱۲"), TEXT("۱۳"), TEXT("۱۴"), TEXT("۱۵"), TEXT("۱۶"), TEXT("۱۷"), TEXT("۱۸"), TEXT("۱۹"), TEXT("۲۰"), TEXT("۲۱"), TEXT("۲۲"), TEXT("۲۳"), TEXT("۲۴"), TEXT("۲۵"), TEXT("۲۶"), TEXT("۲۷"), TEXT("۲۸"), TEXT("۲۹"), TEXT("۳۰"), TEXT("۳۱"), TEXT("۳۲")};
//
    LPCTSTR *jalali_months = PERSIAN ? jalali_months_fa : jalali_months_en;
    LPCTSTR week = PERSIAN ? week_fa : week_en;
    size_t week_len = PERSIAN ? week_len_fa : week_len_en;
    LPCTSTR *numbers = PERSIAN ? numbers_fa : numbers_en;
    
    // For finding which day of the week 1st of the month is.
    struct jtm temp_jtm = {.tm_mday=1, .tm_mon=month, .tm_year=year,};
    int tm_wday_1st_of_month;
    int days_in_month;

    jalali_update(&temp_jtm);
    tm_wday_1st_of_month = temp_jtm.tm_wday;
    days_in_month = jalali_year_month_days(year, month);

    // Zero everything
    for (int i = 0; i < month_buf_length; i++) {
	month_buf[i] = L'\0';
    }

    size_t index = 0;
    // Print name of the month
    StringCchPrintf(&month_buf[index], month_buf_length - index, TEXT("%s\n"), jalali_months[month]);
    while(month_buf[index] != L'\0') { index++; }
    // Print days of week name
    StringCchCopy(&month_buf[index], month_buf_length - index, week);
    index += week_len;

    // Print days of the month
    // Print spaces before first day of the month to reach the correct weekday
    for (int i = 0; i < tm_wday_1st_of_month; i++) {
	month_buf[index] = L' '; index++;
	month_buf[index] = L' '; index++;
	month_buf[index] = L' '; index++;
    }
    int month_column = tm_wday_1st_of_month;
    for (int day = 1; day <= days_in_month; day++) {
	
	StringCchCopy(&month_buf[index], month_buf_length - index, numbers[day]);
	index += 2; // is it still 2 for persian chars?
	month_column++;
	if (month_column >= 7) {
	    month_column = 0;
	    month_buf[index] = L'\n'; index++;
	} else {
	    month_buf[index] = L' '; index++;
	}
    }
    // fill out the last line
    for (int i = (7 - month_column) * 3 - 1; (month_column > 0) && (i > 0); i-- ) {
	month_buf[index] = L' '; index++;
    }

    month_buf[index] = L'\0';
    month_buf[month_buf_length - 1] = L'\0';
    if (DEBUG) {
        fprintf(stderr, "update_month_buf(year=%d, month=%d) returned:\n%ls\n", year, month, month_buf);
    }
}

void update_notification_and_window_content()
{
    if (DEBUG) {
        fprintf(stderr, "update_notification_and_window_content():\n");
        fprintf(stderr, "PERSIAN is %s\n", PERSIAN ? "TRUE":"FALSE");
        fflush(stderr);
    }

    static time_t t;
    static struct jtm jtm;
    time(&t);
    localtime_r(&t, &tm);
    jlocaltime_r(&t, &jtm);

    TCHAR date_string[DATE_STRING_LENGTH]; //this is what the mouse hover tooltip shows
    char utf8_date_string[DATE_STRING_LENGTH]; //holds what we get from libjalali
    // notification
    jstrftime(utf8_date_string, DATE_STRING_LENGTH-1,
                (PERSIAN) ? PERSIAN_FORMAT : ENGLISH_FORMAT, &jtm);
    MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                utf8_date_string, -1, date_string, DATE_STRING_LENGTH);


    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), date_string);
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    for (int i = 0; i < 12; i++) {
	update_month_buf(jtm.tm_year, i);
	SendMessage(static_hwnd_months[i], WM_SETTEXT, (WPARAM) 0, (LPARAM)month_buf);
    }

    static LPCTSTR number_en_to_fa = TEXT("۰۱۲۳۴۵۶۷۸۹");
    TCHAR year_string[6] = {0};
    StringCchPrintf(&year_string[0], 6, TEXT("%d"), jtm.tm_year);
    if (PERSIAN) {
	for (size_t i = 0; i < 6 && year_string[i]; i++) {
	    year_string[i] = number_en_to_fa[year_string[i] - '0'];
	}
    }
    SendMessage(static_hwnd_yeartitle, WM_SETTEXT, (WPARAM) 0, (LPARAM)year_string);
}

void init_notification(HWND hwnd)
{
    /* https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa */
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;

    nid.uID = 1;
    nid.uFlags = NIF_TIP | NIF_ICON | NIF_SHOWTIP | NIF_MESSAGE;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), TEXT("Loading..."));
    nid.uVersion = NOTIFYICON_VERSION_4;
    nid.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

    Shell_NotifyIcon(NIM_ADD, &nid);
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void init_menu()
{
    menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, ID_MENU_TOGGLE_WINDOW,
            PERSIAN ? TEXT("پیدا / پنهان") : TEXT("Show/Hide"));
    AppendMenu(menu, MF_STRING, ID_MENU_CHANGE_LANG,
            PERSIAN ? TEXT("English") : TEXT("فارسی"));
    AppendMenu(menu, MF_STRING, ID_MENU_EXIT,
            PERSIAN ? TEXT("خروج") : TEXT("E&xit"));
}

//create a Label (called a static control in win23 api) to
//show the year calendar in.
void init_window_content(HWND hwnd, HINSTANCE hInstance)
{

    static_hwnd_yeartitle = CreateWindow(
	TEXT("STATIC"),
	TEXT("yeartitle"),
	WS_CHILD | WS_VISIBLE | SS_CENTER,
	0,
	10,
	WIDTH,
	30,
	hwnd,
	NULL,
	hInstance,
	NULL);
/*
 ┌──────────────────────────────┐
 │   ▲50       year             │
 │   ▼                          │
 │  ┌──────┐ ┌──────┐ ┌──────┐  │
 │20│      │ │      │ │      │20│
 │◄►│      │ │      │ │      │◄►│
 │  └──────┘ └──────┘ └──────┘  │
 │  ┌──────┐ ┌──────┐ ┌──────┐  │
 │  │      │ │      │ │      │  │
 │  │      │ │      │ │      │  │
 │  └──────┘ └──────┘ └──────┘  │
 │  ┌──────┐ ┌──────┐ ┌──────┐  │
 │  │      │ │      │ │      │  │
 │  │      │ │      │ │      │  │
 │  └──────┘ └──────┘ └──────┘  │
 │  ┌──────┐ ┌──────┐ ┌──────┐  │
 │  │      │ │      │ │      │  │
 │  │      │ │      │ │      │  │
 │  └──────┘ └──────┘ └──────┘  │
 │             ▲                │
 │   credits   │150             │
 │             │                │
 │             ▼                │
 └──────────────────────────────┘

*/
    TCHAR text[10] = {0};
    for (int i = 0; i < 12; i++) {
	StringCchPrintf(text, 10, TEXT("month %d"), i);
	static_hwnd_months[i] = CreateWindow(
		TEXT("STATIC"),
		text,
		WS_CHILD | WS_VISIBLE | SS_CENTER,
		20 + (i % 3) * ((WIDTH - 40)/3),
		50 + (i / 3) * ((HEIGHT - 200)/4),
		(WIDTH - 40)/3,
		(HEIGHT - 200)/4,
		hwnd,
		NULL,
		hInstance,
		NULL);
    }
    static_hwnd_credits = CreateWindow(
	    TEXT("STATIC"),
	    TEXT("Irancal version: " IRANCAL_VERSION "\ngithub.com/masoudd/irancal\nBy Masoud Naservand masoudd.ir\nLicense: GPLv3"),
	    WS_CHILD | WS_VISIBLE | SS_LEFT,
	    20,
	    HEIGHT - 150,
	    WIDTH - 40,
	    150,
	    hwnd,
	    NULL,
	    hInstance,
	    NULL);

    int ret_addfont = AddFontResource(TEXT("Vazir-Code-Extra-Height.ttf"));
    if (ret_addfont == 0) {
        fprintf(stderr, "Error: Font file not found: Vazir-Code-Extra-Height.ttf\n");
        fflush(stderr);
        hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    } else {
        hFont = CreateFont(20, 0, 0, 0, 0, FALSE, FALSE, FALSE, ARABIC_CHARSET,
                OUT_DEFAULT_PRECIS, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                /* DEFAULT_PITCH */ FIXED_PITCH|FF_DONTCARE, TEXT("Vazir Code Extra Height"));
    }
    for (int i = 0; i < 12; i++) {
	SendMessage(static_hwnd_months[i], WM_SETFONT, (WPARAM)hFont, FALSE);
    }
    SendMessage(static_hwnd_yeartitle, WM_SETFONT, (WPARAM)hFont, FALSE);
    SendMessage(static_hwnd_credits, WM_SETFONT, (WPARAM)hFont, FALSE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL shown = FALSE;
    switch(msg)
    {
	case WM_POWERBROADCAST:
	    if (wParam == PBT_APMRESUMEAUTOMATIC) {
		if (DEBUG) {fprintf(stderr, "Received resume from suspend notification");}
		update_notification_and_window_content();
	    }
	break;

        case MY_TRAY_ICON_MESSAGE:
            switch(LOWORD(lParam))
            {
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    SetForegroundWindow(hwnd);
                    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_VERNEGANIMATION,
                            GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam), 0, hwnd, NULL);

                break;

                case WM_LBUTTONDOWN:
                    if (shown) {
                        ShowWindow(hwnd, SW_HIDE);
                    } else {
                        BringWindowToTop(hwnd);
                        ShowWindow(hwnd, SW_SHOW);
                    }
                    shown = !shown;
                break;
            }
        break;

        case WM_COMMAND:
            switch(wParam) {
                case ID_MENU_TOGGLE_WINDOW:
                    if (shown) {
                        ShowWindow(hwnd, SW_HIDE);
                    } else {
                        BringWindowToTop(hwnd);
                        ShowWindow(hwnd, SW_SHOW);
                    }
                    shown = !shown;
                break;
                case ID_MENU_CHANGE_LANG:
                    PERSIAN = !PERSIAN;
                    DestroyMenu(menu);
                    init_menu();
                    update_notification_and_window_content();
                    SetWindowText(hwnd,
                            PERSIAN ? window_title_fa : window_title_en);
                break;
                case ID_MENU_EXIT:
		    DestroyWindow(hwnd);
                break;
            }
        break;

        case WM_TIMER:
            switch(wParam) {
                case IDT_TIMER1:
                    update_notification_and_window_content();
                    int interval = CHECK_INTERVAL;
                    if (tm.tm_hour == 23) {
                        interval /= 60; //in the last hour, check every minute
                    }
                    if (DEBUG) {
                        interval = 5000;
                    }
                    SetTimer(hwnd, IDT_TIMER1, interval, NULL);
                break;
            }
        break;

	case WM_SIZE:
	    if (DEBUG) {
		fprintf(stderr, "WM_SIZE received in mainproc\n");
	    }
	    RECT rcClient;
	    GetClientRect(hwnd, &rcClient);
	    if (DEBUG) {
		fprintf(stderr, "RECT: %ld, %ld, %ld, %ld\n", rcClient.left, rcClient.top,
							rcClient.right, rcClient.bottom);
	    }
	    //MoveWindow(static_hwnd, 20, 10, rcClient.right/2, rcClient.bottom, TRUE);
	    //MoveWindow(static_hwnd2, rcClient.right/2, 10, rcClient.right, rcClient.bottom, TRUE);
	    
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
    wc.style            = CS_HREDRAW | CS_VREDRAW;
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
            WS_EX_APPWINDOW,
            g_szClassName,
            PERSIAN ? window_title_fa : window_title_en,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
            NULL, NULL, hInstance, NULL);
    if(hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
                MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    init_window_content(hwnd, hInstance);
    init_menu();
    init_notification(hwnd);
    update_notification_and_window_content();

    ShowWindow(hwnd, nCmdShow);
    ShowWindow(hwnd, SW_HIDE); //don't show the window on start

    if (DEBUG) {
        SetTimer(hwnd, IDT_TIMER1, 1000, NULL); // start with 1 secs
    } else {
        SetTimer(hwnd, IDT_TIMER1, 60000, NULL); // start with 60 secs
    }

    // Need the handle to unregister on exit
    HPOWERNOTIFY powernotifyhwnd = RegisterSuspendResumeNotification(hwnd, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (DEBUG) {
	if (powernotifyhwnd == NULL) {
	    fprintf(stderr, "Failed to register for suspend resume notification");
	} else {
	    fprintf(stderr, "Registered for suspend resume notification");
	}
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (DEBUG) {
	fprintf(stderr, "Exiting...");
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);

    if(powernotifyhwnd) {
	if(UnregisterSuspendResumeNotification(powernotifyhwnd) == 0) {
	    fprintf(stderr, "Failed to unregister from suspend resume notification");
	} else {
	    if (DEBUG) {
		fprintf(stderr, "Unregistered from suspend resume notification");
	    }
	}
    }


    DeleteObject(hFont);

    return 0;
}
