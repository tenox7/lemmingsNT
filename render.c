#include "render.h"

static BITMAPINFO *bmpInfo;
static HBITMAP hBitmap;
static HDC memDC;
static BYTE *pixels;
static char bmpInfoBuf[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];

int renderInit(HWND hwnd) {
    HDC hdc;
    int i;

    bmpInfo = (BITMAPINFO *)bmpInfoBuf;
    memset(bmpInfo, 0, sizeof(BITMAPINFOHEADER));
    bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo->bmiHeader.biWidth = GAME_W;
    bmpInfo->bmiHeader.biHeight = -GAME_H;
    bmpInfo->bmiHeader.biPlanes = 1;
    bmpInfo->bmiHeader.biBitCount = 8;
    bmpInfo->bmiHeader.biCompression = BI_RGB;
    bmpInfo->bmiHeader.biClrUsed = 256;

    for (i = 0; i < 256; i++) {
        bmpInfo->bmiColors[i].rgbRed = 0;
        bmpInfo->bmiColors[i].rgbGreen = 0;
        bmpInfo->bmiColors[i].rgbBlue = 0;
        bmpInfo->bmiColors[i].rgbReserved = 0;
    }

    /* basic colors for skeleton */
    bmpInfo->bmiColors[1].rgbRed = 0;
    bmpInfo->bmiColors[1].rgbGreen = 128;
    bmpInfo->bmiColors[1].rgbBlue = 0;

    bmpInfo->bmiColors[2].rgbRed = 0;
    bmpInfo->bmiColors[2].rgbGreen = 0;
    bmpInfo->bmiColors[2].rgbBlue = 255;

    bmpInfo->bmiColors[3].rgbRed = 128;
    bmpInfo->bmiColors[3].rgbGreen = 64;
    bmpInfo->bmiColors[3].rgbBlue = 0;

    bmpInfo->bmiColors[4].rgbRed = 255;
    bmpInfo->bmiColors[4].rgbGreen = 255;
    bmpInfo->bmiColors[4].rgbBlue = 255;

    hdc = GetDC(hwnd);
    memDC = CreateCompatibleDC(hdc);
    hBitmap = CreateDIBSection(hdc, bmpInfo, DIB_RGB_COLORS,
                               (LPVOID *)&pixels, NULL, 0);
    SelectObject(memDC, hBitmap);
    ReleaseDC(hwnd, hdc);

    if (!hBitmap || !pixels)
        return 0;

    memset(pixels, 0, GAME_W * GAME_H);
    return 1;
}

int renderShutdown(HWND hwnd) {
    if (memDC) DeleteDC(memDC);
    if (hBitmap) DeleteObject(hBitmap);
    return 1;
}

int renderClear(BYTE color) {
    memset(pixels, color, GAME_W * GAME_H);
    return 1;
}

int renderSetPixel(int x, int y, BYTE color) {
    if (x < 0 || x >= GAME_W || y < 0 || y >= GAME_H)
        return 0;
    pixels[y * GAME_W + x] = color;
    return 1;
}

BYTE *renderGetBuffer(int dummy) {
    return pixels;
}

int renderSetPalette(RGBQUAD *colors, int start, int count) {
    int i;
    for (i = 0; i < count && (start + i) < 256; i++) {
        bmpInfo->bmiColors[start + i].rgbRed = colors[i].rgbRed;
        bmpInfo->bmiColors[start + i].rgbGreen = colors[i].rgbGreen;
        bmpInfo->bmiColors[start + i].rgbBlue = colors[i].rgbBlue;
        bmpInfo->bmiColors[start + i].rgbReserved = 0;
    }
    return 1;
}

int renderFrame(HWND hwnd) {
    HDC hdc;
    RECT rc;

    hdc = GetDC(hwnd);
    GetClientRect(hwnd, &rc);
    StretchDIBits(hdc, 0, 0, rc.right, rc.bottom,
                  0, 0, GAME_W, GAME_H,
                  pixels, bmpInfo, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwnd, hdc);
    return 1;
}
