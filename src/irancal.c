#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <winuser.h>
#include <windowsx.h>
#include <strsafe.h>
#include <stdio.h>

const wchar_t g_szClassName[] = L"myWindowClass";
HMENU menu;
#define ID_MENU_EXIT 9001
#define MY_TRAY_ICON_MESSAGE 0xBF00

void init_menu()
{
    /*
    menu = CreatePopupMenu();
    if (menu)
    {
        MENUITEMINFO exit;
        exit.cbSize = sizeof(MENUITEMINFO);
        exit.fMask = MIIM_FTYPE;
        exit.fType = MFT_STRING;

        InsertMenuItemW(menu, 0, TRUE, &exit);
    }
    */
    menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, ID_MENU_EXIT, L"E&xit");
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
                    TrackPopupMenu(
                            menu,
                            TPM_RIGHTBUTTON | TPM_VERNEGANIMATION,
                            GET_X_LPARAM(wParam),
                            GET_Y_LPARAM(wParam),
                            0,
                            hwnd,
                            NULL
                    );
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

    /* just don't show the window */
    //ShowWindow(hwnd, nCmdShow);
    //UpdateWindow(hwnd);

    init_menu();




/* NOTIFICATION */

    /* https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa */
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;

    nid.uID = 1;
    nid.uFlags = NIF_TIP | NIF_ICON | NIF_SHOWTIP | NIF_MESSAGE;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), L"TODO: display current date");
    nid.uVersion = NOTIFYICON_VERSION_4;
    nid.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

    Shell_NotifyIcon(NIM_ADD, &nid);
    Shell_NotifyIcon(NIM_SETVERSION, &nid);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    return 0;
}
