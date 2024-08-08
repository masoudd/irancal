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
#include <wingdi.h>
#include <strsafe.h>
#include <stdio.h>
#include <time.h>

#include "jalali.h"
#include "jtime.h"
/*
 * How it works:
 *  check every WM_TIME proc if last_day != current_day
 *  and update tooltip string and shown year calendar in window if yes.
 *  also decide on how long next WM_TIME should be.
 */

/* Useful to change globals: */
#define DATE_STRING_LENGTH 64 //How long is the tooltip we want to show
BOOLEAN PERSIAN = TRUE; //Change to FALSE if you want English as default
#define PERSIAN_FORMAT "%W %G" // refer to jtime.c or jdate format docs
#define ENGLISH_FORMAT "%F %q"
#define CHECK_INTERVAL 3600000 // 1 hour
#define WIDTH 600
#define HEIGHT 600
/* ------------------------ */

#define year_buf_length 3000
TCHAR year_buf[year_buf_length];
struct tm tm;


HMENU menu;
NOTIFYICONDATA nid;
HWND static_hwnd; // handle to the label (called static in win32 api)
LPTSTR utf16_jalali_months[] = {
        TEXT("Farvardin"), TEXT("Ordibehesht"), TEXT("Khordaad"),
        TEXT("Tir"), TEXT("Mordaad"), TEXT("Shahrivar"),
        TEXT("Mehr"), TEXT("Aabaan"), TEXT("Aazar"),
        TEXT("Dey"), TEXT("Bahman"), TEXT("Esfand")};
// how many spaces we need before the name of the month in the year table
// + length of the name of the month to give to printf %*s as width
// so that the name of the month is centered on top the 7 days of the week.
int jalali_months_table_width[] = { 14, 24, 21, 11, 25, 24, 11, 24, 23, 11, 24, 23 };
    
// these are random value, apparently that's how
// it works. You just come up with something random
// and hope it doesn't clash with something else
const wchar_t g_szClassName[] = L"MyHopefullyUniqueWindowClass";
#define ID_MENU_EXIT 9001  
#define MY_TRAY_ICON_MESSAGE 0xBF00
#define IDT_TIMER1 9009
#define ID_MYSTATIC 9010

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
// Use libjalali api to make a representation of current hijri shamsi year.
// A table with 4 rows and 3 columns with each cell being a month.
// Each month is a smaller table with 7 columns (month_column) and
// 5 or 6 rows (month_row) depending on length of month and which
// day of the week the 1st day of that month is.
void update_year_buf(int year)
{
    static LPTSTR week =
        TEXT("Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo\n");
    size_t week_len = 67; //lenth of the LPTSTR week
    // For finding which day of the week 1st of each month is.
    struct jtm temp_jtm = {.tm_mday=1, .tm_year=year,};
    int tm_wday_1st_of_months[12];
    size_t index = 0;
    int days_in_month[12];

    for (int i = 0; i < 12; i++) {
        temp_jtm.tm_mon = i;
        jalali_update(&temp_jtm);
        tm_wday_1st_of_months[i] = temp_jtm.tm_wday;
        days_in_month[i] = jalali_year_month_days(year, i);
    }
    for (int i = 0; i < 30; i++) {
        year_buf[index] = L' ';
        index++;
    }
    StringCchPrintf(&year_buf[index], year_buf_length - index, L"%d\n\n", year);
    while(year_buf[index] != L'\0') { index++; }

    for (int row = 0; row < 4; row++) {
        // Print name of the months
        StringCchPrintf(&year_buf[index], year_buf_length - index,
                L"%*s%*s%*s\n",
                jalali_months_table_width[row * 3 + 0], utf16_jalali_months[row * 3 + 0],
                jalali_months_table_width[row * 3 + 1], utf16_jalali_months[row * 3 + 1],
                jalali_months_table_width[row * 3 + 2], utf16_jalali_months[row * 3 + 2]
                );
        while(year_buf[index] != L'\0') { index++; }
        StringCchCopy(&year_buf[index], year_buf_length - index, week);
        index += week_len;

        // Print 3 months row by row (month_row) until all 3 months (column) are done.
        int printed_days_month[3] = {0, 0, 0};
        while (printed_days_month[0] < days_in_month[row * 3 + 0] &&
               printed_days_month[1] < days_in_month[row * 3 + 1] &&
               printed_days_month[2] < days_in_month[row * 3 + 2]) {
            for (int column = 0; column < 3; column++) {
                int month = (row * 3) + column;
                for (int month_column = 0; month_column < 7; month_column++) {
                    if ((printed_days_month[column] == 0 && tm_wday_1st_of_months[month] > month_column) ||
                       (printed_days_month[column] >= days_in_month[month])) {
                        //three spaces
                        year_buf[index] = L' '; index++;
                        year_buf[index] = L' '; index++;
                        year_buf[index] = L' '; index++;
                    } else {
                        printed_days_month[column]++;
                        StringCchPrintf(&year_buf[index], year_buf_length - index,
                                L"%2d ", printed_days_month[column]);
                        index += 3;
                    }
                }
                // two spaces between each 'column'.
                year_buf[index] = L' '; index++;
                year_buf[index] = L' '; index++;
            }
            year_buf[index] = L'\n'; index++;
        }
        year_buf[index] = L'\n'; index++;
    }

    year_buf[index] = L'\0';
    year_buf[year_buf_length - 1] = L'\0';
}

void update_notification_and_window_content()
{

    static int last_day = -1; // Day was this last time we checked the date.
    static int last_year = -1; // Note the year we track is hijri shamsi.
    static time_t t;
    static struct jtm jtm;
    time(&t);
    localtime_r(&t, &tm);
    if (last_day == tm.tm_yday) {
        return;
    }
    last_day = tm.tm_yday;
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

    // window content
    // only changes once a year
    if (last_year == jtm.tm_year) {
        return;
    }
    last_year = jtm.tm_year;
    update_year_buf(jtm.tm_year);
    SendMessage(static_hwnd, WM_SETTEXT, (WPARAM) 0, (LPARAM)year_buf);
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
    AppendMenu(menu, MF_STRING, ID_MENU_EXIT,
            PERSIAN ? TEXT("خروج") : TEXT("E&xit"));
}

//create a Label (called a static control in win23 api) to
//show the year calendar in.
void init_window_content(HWND hwnd, HINSTANCE hInstance)
{
    static_hwnd = CreateWindow(
            TEXT("STATIC"),
            TEXT("Calendar goes here"),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20,
            10,
            WIDTH - 40,
            HEIGHT - 20,
            hwnd,
            (HMENU) ID_MYSTATIC,
            hInstance,
            NULL);
    HFONT hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    SendMessage(static_hwnd, WM_SETFONT, (WPARAM)hFont, FALSE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL shown = FALSE;
    switch(msg)
    {
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
                case ID_MENU_EXIT:
                    PostQuitMessage(0);
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

    SetTimer(hwnd, IDT_TIMER1, 60000, NULL); // start with 60 secs

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    return 0;
}
