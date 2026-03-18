#include "sprite.h"
#include "dat.h"
#include <string.h>

static Animation anims[ANIM_COUNT];
static BYTE panelPixels[320 * 40];
static int panelLoaded = 0;

static int decodeAnim(BYTE *data, int dataLen, int offset,
                       int w, int h, int bpp, int numFrames,
                       Animation *anim) {
    int f, plane;
    int pixCount = w * h;
    int srcPos;

    anim->width = w;
    anim->height = h;
    anim->numFrames = numFrames;
    anim->footX = 8;
    anim->footY = 10;

    srcPos = offset;

    for (f = 0; f < numFrames; f++) {
        BYTE *pix = anim->frames[f].pixels;
        memset(pix, 0, SPRITE_W * SPRITE_H);

        for (plane = 0; plane < bpp; plane++) {
            int bitBufLen = 0;
            int bitBuf = 0;
            int y, x;

            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    if (bitBufLen <= 0) {
                        if (srcPos >= dataLen)
                            return 0;
                        bitBuf = data[srcPos++];
                        bitBufLen = 8;
                    }
                    pix[y * SPRITE_W + x] |= (BYTE)(((bitBuf & 0x80) >> (7 - plane)));
                    bitBuf <<= 1;
                    bitBufLen--;
                }
            }
        }
    }

    return 1;
}

static int decodePanel(BYTE *data, int dataLen) {
    int plane, px;
    int pixCount = 320 * 40;
    int srcPos = 0;

    memset(panelPixels, 0, sizeof(panelPixels));

    for (plane = 0; plane < 4; plane++) {
        int bitBufLen = 0;
        int bitBuf = 0;
        for (px = 0; px < pixCount; px++) {
            if (bitBufLen <= 0) {
                if (srcPos >= dataLen)
                    return 0;
                bitBuf = data[srcPos++];
                bitBufLen = 8;
            }
            panelPixels[px] |= (BYTE)(((bitBuf & 0x80) >> (7 - plane)));
            bitBuf <<= 1;
            bitBufLen--;
        }
    }

    panelLoaded = 1;
    return 1;
}

int spriteInit(int dummy) {
    DatFile mainDat;
    BYTE data[DAT_MAX_DECOMP];
    int size;

    memset(anims, 0, sizeof(anims));
    memset(panelPixels, 0, sizeof(panelPixels));

    if (!datOpen(&mainDat, "data\\MAIN.DAT"))
        return 0;

    size = datDecompress(&mainDat, 0, data, sizeof(data));
    if (size > 0) {
        decodeAnim(data, size, 0x0000, 16, 10, 2, 8, &anims[ANIM_WALK_R]);
        decodeAnim(data, size, 0x0168, 16, 10, 2, 8, &anims[ANIM_WALK_L]);
        decodeAnim(data, size, 0x37F0, 16, 10, 2, 4, &anims[ANIM_FALL_R]);
        decodeAnim(data, size, 0x3890, 16, 10, 2, 4, &anims[ANIM_FALL_L]);
        decodeAnim(data, size, 0x02D0, 16, 14, 3, 16, &anims[ANIM_DIG]);
        anims[ANIM_DIG].footY = 12;
        decodeAnim(data, size, 0x3F30, 16, 10, 2, 16, &anims[ANIM_SPLAT]);
        decodeAnim(data, size, 0x41B0, 16, 13, 2, 8, &anims[ANIM_EXIT]);
        anims[ANIM_EXIT].footY = 13;
        decodeAnim(data, size, 0x0810, 16, 12, 2, 8, &anims[ANIM_CLIMB_R]);
        anims[ANIM_CLIMB_R].footY = 12;
        decodeAnim(data, size, 0x0990, 16, 12, 2, 8, &anims[ANIM_CLIMB_L]);
        anims[ANIM_CLIMB_L].footY = 12;
        decodeAnim(data, size, 0x1090, 16, 13, 3, 16, &anims[ANIM_BUILD_R]);
        anims[ANIM_BUILD_R].footY = 13;
        decodeAnim(data, size, 0x1570, 16, 13, 3, 16, &anims[ANIM_BUILD_L]);
        anims[ANIM_BUILD_L].footY = 13;
        decodeAnim(data, size, 0x1A50, 16, 10, 3, 32, &anims[ANIM_BASH_R]);
        decodeAnim(data, size, 0x21D0, 16, 10, 3, 32, &anims[ANIM_BASH_L]);
        decodeAnim(data, size, 0x2950, 16, 13, 3, 24, &anims[ANIM_MINE_R]);
        anims[ANIM_MINE_R].footY = 13;
        decodeAnim(data, size, 0x30A0, 16, 13, 3, 24, &anims[ANIM_MINE_L]);
        anims[ANIM_MINE_L].footY = 13;
        decodeAnim(data, size, 0x3930, 16, 16, 3, 8, &anims[ANIM_FLOAT_R]);
        anims[ANIM_FLOAT_R].footY = 16;
        decodeAnim(data, size, 0x3C30, 16, 16, 3, 8, &anims[ANIM_FLOAT_L]);
        anims[ANIM_FLOAT_L].footY = 16;
        decodeAnim(data, size, 0x4970, 16, 10, 2, 16, &anims[ANIM_BLOCK]);
        decodeAnim(data, size, 0x4E70, 16, 10, 2, 16, &anims[ANIM_OHNO]);
        decodeAnim(data, size, 0x50F0, 32, 32, 3, 1, &anims[ANIM_EXPLODE]);
        anims[ANIM_EXPLODE].footX = 16;
        anims[ANIM_EXPLODE].footY = 25;
    }

    size = datDecompress(&mainDat, 6, data, sizeof(data));
    if (size > 0)
        decodePanel(data, size);

    datClose(&mainDat);
    return 1;
}

Animation *spriteGetAnim(int animId) {
    if (animId < 0 || animId >= ANIM_COUNT)
        return &anims[0];
    return &anims[animId];
}

BYTE *spriteGetPanel(int dummy) {
    if (!panelLoaded)
        return NULL;
    return panelPixels;
}
