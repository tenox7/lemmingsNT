#include <windows.h>
#include "game.h"
#include "render.h"

static HWND hWndMain;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int mx, my;
    switch (msg) {
    case WM_DESTROY:
        game.running = 0;
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            game.running = 0;
            PostQuitMessage(0);
            return 0;
        }
        gameKeyDown((int)wParam);
        return 0;
    case WM_KEYUP:
        gameKeyUp((int)wParam);
        return 0;
    case WM_LBUTTONDOWN:
        mx = LOWORD(lParam) / 2;
        my = HIWORD(lParam) / 2;
        gameClick(mx, my);
        return 0;
    case WM_QUERYNEWPALETTE:
        return renderRealizePalette(hwnd);
    case WM_PALETTECHANGED:
        if ((HWND)wParam != hwnd)
            renderRealizePalette(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow) {
    WNDCLASS wc;
    MSG msg;
    RECT rc;
    DWORD style;
    DWORD lastTick, now, elapsed;
    SYSTEM_INFO si;
    char title[64];
    const char *arch;

    GetSystemInfo(&si);
    switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL: arch = "x86"; break;
    case PROCESSOR_ARCHITECTURE_MIPS:  arch = "MIPS"; break;
    case PROCESSOR_ARCHITECTURE_ALPHA: arch = "Alpha"; break;
    case PROCESSOR_ARCHITECTURE_PPC:   arch = "PowerPC"; break;
    default:                           arch = "Unknown"; break;
    }
    wsprintf(title, "%s %s", GAME_TITLE, arch);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = GAME_TITLE;
    RegisterClass(&wc);

    style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    rc.left = 0;
    rc.top = 0;
    rc.right = SCREEN_W;
    rc.bottom = SCREEN_H;
    AdjustWindowRect(&rc, style, FALSE);

    hWndMain = CreateWindow(GAME_TITLE, title, style,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rc.right - rc.left, rc.bottom - rc.top,
                            NULL, NULL, hInst, NULL);
    if (!hWndMain)
        return 1;

    ShowWindow(hWndMain, cmdShow);
    UpdateWindow(hWndMain);

    if (!gameInit(hWndMain))
        return 1;

    lastTick = GetTickCount();

    while (game.running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                game.running = 0;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!game.running)
            break;

        now = GetTickCount();
        elapsed = now - lastTick;
        if (elapsed >= FRAME_MS) {
            lastTick = now;
            gameUpdate(hWndMain);
            gameRender(hWndMain);
        } else {
            Sleep(1);
        }
    }

    gameShutdown(hWndMain);
    return 0;
}
