#include "sprite.h"
#include "dat.h"
#include <string.h>

static Animation anims[ANIM_COUNT];

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
        int frameStart = srcPos;
        BYTE *pix = anim->frames[f].pixels;
        memset(pix, 0, SPRITE_W * SPRITE_H);

        for (plane = 0; plane < bpp; plane++) {
            int bitBufLen = 0;
            int bitBuf = 0;
            int lineSize = w / 8;
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

int spriteInit(int dummy) {
    DatFile mainDat;
    BYTE animData[DAT_MAX_DECOMP];
    int animSize;

    memset(anims, 0, sizeof(anims));

    if (!datOpen(&mainDat, "data\\MAIN.DAT"))
        return 0;

    animSize = datDecompress(&mainDat, 0, animData, sizeof(animData));
    datClose(&mainDat);

    if (animSize <= 0)
        return 0;

    decodeAnim(animData, animSize, 0x0000, 16, 10, 2, 8, &anims[ANIM_WALK_R]);
    decodeAnim(animData, animSize, 0x0168, 16, 10, 2, 8, &anims[ANIM_WALK_L]);
    decodeAnim(animData, animSize, 0x37F0, 16, 10, 2, 4, &anims[ANIM_FALL_R]);
    decodeAnim(animData, animSize, 0x3890, 16, 10, 2, 4, &anims[ANIM_FALL_L]);
    decodeAnim(animData, animSize, 0x02D0, 16, 14, 3, 16, &anims[ANIM_DIG]);
    anims[ANIM_DIG].footY = 12;
    decodeAnim(animData, animSize, 0x3F30, 16, 10, 2, 16, &anims[ANIM_SPLAT]);
    decodeAnim(animData, animSize, 0x41B0, 16, 13, 2, 8, &anims[ANIM_EXIT]);
    anims[ANIM_EXIT].footY = 13;

    return 1;
}

Animation *spriteGetAnim(int animId) {
    if (animId < 0 || animId >= ANIM_COUNT)
        return &anims[0];
    return &anims[animId];
}
