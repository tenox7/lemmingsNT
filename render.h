#ifndef RENDER_H
#define RENDER_H

#include <windows.h>
#include "game.h"

int renderInit(HWND hwnd);
int renderShutdown(HWND hwnd);
int renderFrame(HWND hwnd);
int renderSetPixel(int x, int y, BYTE color);
int renderClear(BYTE color);
BYTE *renderGetBuffer(int dummy);
int renderSetPalette(RGBQUAD *colors, int start, int count);

#endif
