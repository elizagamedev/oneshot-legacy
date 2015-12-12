#include "window.h"

#include "util.h"

//For the kitty escape window
const char wndKitty_classname[] = "wndKitty";
HWND wndKitty = NULL;
HBITMAP wndKitty_bmps[3] = {NULL};
volatile HANDLE wndKittyThread = NULL;
int wndKitty_offset = 0;

//Is the kitty walking?
BOOL wndKitty_isActive = FALSE;

#define FPS 60

//kitty thread
DWORD WINAPI wndKitty_thread(LPVOID lpParameter)
{
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    unsigned long tickLast = 0;

    //Get starting position
    RECT area;
    GetWindowRect(wndKitty, &area);

    wndKitty_isActive = TRUE;

    //Just keep walking down
    for (;;) {
        //Move window
        wndKitty_offset += 2;

        if (area.top + wndKitty_offset >= screenHeight)
            break;

        SetWindowPos(wndKitty, NULL, area.left, area.top + wndKitty_offset, 0, 0, SWP_NOSIZE);

        //Repaint window
        InvalidateRect(wndKitty, NULL, FALSE);

        //Regulate FPS
        unsigned long delta = GetTickCount() - tickLast;
        if (delta < 1000 / FPS)
            Sleep(1000 / FPS - delta);
        tickLast = GetTickCount();
    }

    wndKitty_isActive = FALSE;

    //If the window is destroyed, quit the game
    if (isWindowDestroyed) {
        util_saveEnding();
        exit(0);
    }

    return 0;
}

//Kitty window procedure
LRESULT CALLBACK wndKitty_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CLOSE:
        return 0;

    case WM_PAINT: {
        static int frame = 0;
        if (wndKitty_offset % 32 >= 16) {
            frame = 1;
        } else {
            if ((wndKitty_offset / 32) % 2)
                frame = 0;
            else
                frame = 2;
        }

        PAINTSTRUCT ps;
        HDC hdc;
        BITMAP bitmap;
        HDC hdcMem;
        HGDIOBJ oldBitmap;

        hdc = BeginPaint(hwnd, &ps);

        hdcMem = CreateCompatibleDC(hdc);
        oldBitmap = SelectObject(hdcMem, wndKitty_bmps[frame]);

        GetObject(wndKitty_bmps[frame], sizeof(bitmap), &bitmap);
        BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, oldBitmap);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
    }
    break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void wndKitty_create()
{
    //load kitty walk animation
    wndKitty_bmps[0] = LoadBitmap(dll_hInstance, "NIKO1");
    wndKitty_bmps[1] = LoadBitmap(dll_hInstance, "NIKO2");
    wndKitty_bmps[2] = LoadBitmap(dll_hInstance, "NIKO3");

    WNDCLASSEX wc;
    wc.cbSize           = sizeof(wc);
    wc.style            = 0;
    wc.lpfnWndProc      = wndKitty_proc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = dll_hInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = wndKitty_classname;
    wc.hIconSm          = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
        return;

    if (!(wndKitty = CreateWindowEx(WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                                    wndKitty_classname,
                                    "Kitty",
                                    WS_POPUP,
                                    0, 0,
                                    24 * 2, 32 * 2,
                                    NULL, NULL, dll_hInstance, NULL))) {
        return;
    }

    SetLayeredWindowAttributes(wndKitty, RGB(0, 255, 0), 0, LWA_COLORKEY);
}

void wndKitty_start()
{
    //Calculate where to stick the window
    POINT pos;
    pos.x = (9 * 16 - 4) * 2;
    pos.y = (13 * 16) * 2;
    ClientToScreen(window, &pos);

    SetWindowPos(wndKitty, NULL, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

    ShowWindow(wndKitty, SW_SHOWNOACTIVATE);
    UpdateWindow(wndKitty);

    wndKittyThread = CreateThread(NULL, 0, wndKitty_thread, NULL, 0, NULL);
}

void wndKitty_destroy()
{
    if (wndKitty)
        DestroyWindow(wndKitty);

    int i = 0;
    for (; i < 3; i++) {
        if (wndKitty_bmps[i])
            DeleteObject(wndKitty_bmps[i]);
    }
}
