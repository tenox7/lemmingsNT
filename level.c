#include "level.h"
#include "dat.h"
#include <string.h>

static ObjSpriteData objSprites[MAX_GROUND_OBJECTS];

static WORD rdBE16(BYTE *p) {
    return (WORD)((p[0] << 8) | p[1]);
}

static WORD rdLE16(BYTE *p) {
    return (WORD)(p[0] | (p[1] << 8));
}

static DWORD rdBE32(BYTE *p) {
    return ((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) |
           ((DWORD)p[2] << 8) | (DWORD)p[3];
}

static int parseGroundData(BYTE *raw, GroundData *gd) {
    int i;
    BYTE *p;

    memset(gd, 0, sizeof(GroundData));
    gd->numObjects = MAX_GROUND_OBJECTS;
    gd->numTerrains = MAX_GROUND_TERRAINS;

    for (i = 0; i < MAX_GROUND_OBJECTS; i++) {
        p = raw + i * 28;
        gd->objects[i].flags = (int)rdLE16(p);
        gd->objects[i].firstFrame = p[2];
        gd->objects[i].frameCount = p[3];
        gd->objects[i].width = p[4];
        gd->objects[i].height = p[5];
        gd->objects[i].frameDataSize = (int)rdLE16(p + 6);
        gd->objects[i].maskLoc = (int)rdLE16(p + 8);
        gd->objects[i].triggerLeft = (int)rdLE16(p + 14) * 4;
        gd->objects[i].triggerTop = (int)rdLE16(p + 16) * 4 - 4;
        gd->objects[i].triggerWidth = p[18] * 4;
        gd->objects[i].triggerHeight = p[19] * 4;
        gd->objects[i].triggerEffect = p[20];
        gd->objects[i].imageLoc = (int)rdLE16(p + 21);
        gd->objects[i].trapSoundId = p[27];
    }

    for (i = 0; i < MAX_GROUND_TERRAINS; i++) {
        p = raw + 0x01C0 + i * 8;
        gd->terrains[i].width = p[0];
        gd->terrains[i].height = p[1];
        gd->terrains[i].imageLoc = (int)rdLE16(p + 2);
        gd->terrains[i].maskLoc = (int)rdLE16(p + 4);
    }

    p = raw + 0x03D8;
    for (i = 0; i < 8; i++) {
        gd->palette[i + 8].rgbRed = (BYTE)(p[i * 3 + 0] << 2);
        gd->palette[i + 8].rgbGreen = (BYTE)(p[i * 3 + 1] << 2);
        gd->palette[i + 8].rgbBlue = (BYTE)(p[i * 3 + 2] << 2);
    }

    p = raw + 0x03F0;
    for (i = 0; i < 8; i++) {
        gd->palette[i].rgbRed = (BYTE)(p[i * 3 + 0] << 2);
        gd->palette[i].rgbGreen = (BYTE)(p[i * 3 + 1] << 2);
        gd->palette[i].rgbBlue = (BYTE)(p[i * 3 + 2] << 2);
    }

    return 1;
}

static int decodeTerrain(BYTE *vgaData, int vgaLen, TerrainMeta *meta,
                          BYTE *pixels) {
    int pixCount, bitBufLen, bitBuf;
    int plane, px;
    int srcPos;

    if (meta->width == 0 || meta->height == 0)
        return 0;

    pixCount = meta->width * meta->height;
    memset(pixels, 0, pixCount);
    srcPos = meta->imageLoc;

    for (plane = 0; plane < 3; plane++) {
        bitBufLen = 0;
        bitBuf = 0;
        for (px = 0; px < pixCount; px++) {
            if (bitBufLen <= 0) {
                if (srcPos >= vgaLen)
                    return 0;
                bitBuf = vgaData[srcPos++];
                bitBufLen = 8;
            }
            pixels[px] |= (BYTE)(((bitBuf & 0x80) >> (7 - plane)));
            bitBuf <<= 1;
            bitBufLen--;
        }
    }

    {
        int maskPos = meta->maskLoc;
        bitBufLen = 0;
        bitBuf = 0;
        for (px = 0; px < pixCount; px++) {
            if (bitBufLen <= 0) {
                if (maskPos >= vgaLen)
                    break;
                bitBuf = vgaData[maskPos++];
                bitBufLen = 8;
            }
            if (!(bitBuf & 0x80))
                pixels[px] |= 0x80;
            bitBuf <<= 1;
            bitBufLen--;
        }
    }

    return 1;
}

static int placeTerrain(Level *level, GroundData *gd,
                         BYTE *vgaData, int vgaLen,
                         BYTE *rawLevel) {
    BYTE terrainPixels[256 * 256];
    int i;
    BYTE *p;

    for (i = 0; i < MAX_LEVEL_TERRAIN; i++) {
        DWORD val;
        int x, y, id, flags;
        int yVal;
        int isErase, isUpsideDown, noOverwrite;
        TerrainMeta *meta;
        int tx, ty;

        p = rawLevel + 0x0120 + i * 4;
        val = rdBE32(p);
        if (val == 0xFFFFFFFF)
            continue;

        x = (int)((val >> 16) & 0x0FFF) - 16;
        yVal = (int)((val >> 7) & 0x01FF);
        y = yVal - ((yVal > 256) ? 516 : 4);
        id = (int)(val & 0x003F);
        flags = (int)((val >> 29) & 0x0F);
        isErase = flags & 1;
        isUpsideDown = (flags >> 1) & 1;
        noOverwrite = (flags >> 2) & 1;

        if (id < 0 || id >= MAX_GROUND_TERRAINS)
            continue;

        meta = &gd->terrains[id];
        if (meta->width == 0 || meta->height == 0)
            continue;

        if (!decodeTerrain(vgaData, vgaLen, meta, terrainPixels))
            continue;

        for (ty = 0; ty < meta->height; ty++) {
            int srcY = isUpsideDown ? (meta->height - 1 - ty) : ty;
            int destY = y + ty;

            if (destY < 0 || destY >= LEVEL_H)
                continue;

            for (tx = 0; tx < meta->width; tx++) {
                int destX = x + tx;
                BYTE pixel;
                int destIdx;

                if (destX < 0 || destX >= LEVEL_W)
                    continue;

                pixel = terrainPixels[srcY * meta->width + tx];
                destIdx = destY * LEVEL_W + destX;

                if (isErase) {
                    level->terrain[destIdx] = 0;
                    level->visual[destIdx] = 0;
                    continue;
                }

                if (pixel & 0x80)
                    continue;

                if (noOverwrite && level->terrain[destIdx])
                    continue;

                level->visual[destIdx] = (pixel & 0x07) + 8;
                level->terrain[destIdx] = 1;
            }
        }
    }

    return 1;
}

static int decodeObjFrame(BYTE *vga, int vgaLen, int offset, int maskOfs,
                           int w, int h, BYTE *pixels) {
    int pixCount = w * h;
    int plane, srcPos, bitBufLen, bitBuf, px;

    memset(pixels, 0, pixCount);
    srcPos = offset;

    for (plane = 0; plane < 4; plane++) {
        bitBufLen = 0;
        bitBuf = 0;
        for (px = 0; px < pixCount; px++) {
            if (bitBufLen <= 0) {
                if (srcPos >= vgaLen)
                    return 0;
                bitBuf = vga[srcPos++];
                bitBufLen = 8;
            }
            pixels[px] |= (BYTE)(((bitBuf & 0x80) >> (7 - plane)));
            bitBuf <<= 1;
            bitBufLen--;
        }
    }

    srcPos = offset + maskOfs;
    bitBufLen = 0;
    bitBuf = 0;
    for (px = 0; px < pixCount; px++) {
        if (bitBufLen <= 0) {
            if (srcPos >= vgaLen)
                break;
            bitBuf = vga[srcPos++];
            bitBufLen = 8;
        }
        if (!(bitBuf & 0x80))
            pixels[px] |= 0x80;
        bitBuf <<= 1;
        bitBufLen--;
    }

    return 1;
}

static int decodeObjSprites(GroundData *gd, BYTE *vga, int vgaLen) {
    int i, f;

    memset(objSprites, 0, sizeof(objSprites));

    for (i = 0; i < MAX_GROUND_OBJECTS; i++) {
        ObjectMeta *m = &gd->objects[i];
        ObjSpriteData *s = &objSprites[i];

        s->width = m->width;
        s->height = m->height;
        s->numFrames = m->frameCount;
        s->animType = m->flags & 3;

        if (m->frameCount <= 0 || m->width <= 0 || m->height <= 0)
            continue;
        if (m->width > MAX_OBJ_DIM || m->height > MAX_OBJ_DIM)
            continue;
        if (m->frameCount > MAX_OBJ_AFRAMES)
            s->numFrames = MAX_OBJ_AFRAMES;

        for (f = 0; f < s->numFrames; f++) {
            int fOfs = m->imageLoc + f * m->frameDataSize;
            decodeObjFrame(vga, vgaLen, fOfs, m->maskLoc,
                           m->width, m->height, s->pixels[f]);
        }
    }

    return 1;
}

ObjSpriteData *levelGetObjSprite(int objId) {
    if (objId < 0 || objId >= MAX_GROUND_OBJECTS)
        return NULL;
    return &objSprites[objId];
}

static int parseLevelInfo(BYTE *raw, LevelInfo *info) {
    int i;

    info->releaseRate = (int)rdBE16(raw + 0x00);
    info->numLemmings = (int)rdBE16(raw + 0x02);
    info->numToSave = (int)rdBE16(raw + 0x04);
    info->timeLimit = (int)rdBE16(raw + 0x06);

    for (i = 0; i < 8; i++)
        info->skills[i] = (int)rdBE16(raw + 0x08 + i * 2);

    info->startX = (int)rdBE16(raw + 0x18);
    info->graphicSet = (int)rdBE16(raw + 0x1A);

    memset(info->name, 0, sizeof(info->name));
    memcpy(info->name, raw + 0x07E0, 32);

    return 1;
}

static int parseSteel(BYTE *raw, Level *level) {
    int i;

    for (i = 0; i < MAX_LEVEL_STEEL; i++) {
        BYTE *p = raw + 0x0760 + i * 4;
        WORD pos = rdBE16(p);
        BYTE sz = p[2];
        int x, y, w, h;
        int sx, sy;

        if (pos == 0 && sz == 0)
            continue;

        x = (int)((pos & 0x01FF) << 2) - 16;
        y = (int)(((pos >> 9) & 0x3F) << 2);
        w = (int)((sz >> 4) & 0x0F) * 4 + 4;
        h = (int)(sz & 0x0F) * 4 + 4;

        for (sy = y; sy < y + h && sy < LEVEL_H; sy++) {
            if (sy < 0) continue;
            for (sx = x; sx < x + w && sx < LEVEL_W; sx++) {
                if (sx < 0) continue;
                level->terrain[sy * LEVEL_W + sx] = 2;
            }
        }
    }

    return 1;
}

static int parseLevelObjects(BYTE *rawLevel, GroundData *gd, Level *level,
                              int *ex, int *ey) {
    int i;
    BYTE *p;

    level->numTriggers = 0;
    level->numObjs = 0;
    *ex = 0;
    *ey = 0;

    for (i = 0; i < MAX_LEVEL_OBJECTS; i++) {
        int ox, oy, oid, flags;
        ObjectMeta *meta;
        PlacedObj *po;
        p = rawLevel + 0x20 + i * 8;
        ox = (int)((p[0] << 8) | p[1]) - 16;
        oy = (int)((p[2] << 8) | p[3]);
        oid = (int)((p[4] << 8) | p[5]);
        flags = (int)((p[6] << 8) | p[7]);
        if (flags == 0 && oid == 0)
            continue;
        if (oid >= MAX_GROUND_OBJECTS)
            continue;

        meta = &gd->objects[oid];

        if (oid == 1) {
            *ex = ox + 24;
            *ey = oy + 14;
        }

        if (level->numObjs < MAX_OBJECTS) {
            po = &level->objs[level->numObjs];
            po->x = ox;
            po->y = oy;
            po->objId = oid;
            po->frame = 0;
            po->animDone = 0;
            level->numObjs++;
        }

        if (meta->triggerEffect != 0 && level->numTriggers < MAX_TRIGGERS) {
            Trigger *t = &level->triggers[level->numTriggers];
            t->type = meta->triggerEffect;
            t->x1 = ox + meta->triggerLeft;
            t->y1 = oy + meta->triggerTop;
            t->x2 = t->x1 + meta->triggerWidth;
            t->y2 = t->y1 + meta->triggerHeight;
            level->numTriggers++;
        }
    }

    return 1;
}

static const int funOrder[30] = {
    91,95,96,92,93,94,97,-6,-12,-32,
    -42,-7,16,-17,-22,-24,-27,-43,-51,-63,
    -84,13,-41,-57,-60,-71,-46,-61,-65,-82
};

int levelLoad(int levelNumber, Level *level, LevelInfo *info, RGBQUAD *palette) {
    DatFile levelDat, vgaDat;
    BYTE levelRaw[2048];
    BYTE groundRaw[1056];
    BYTE vgaRaw[DAT_MAX_DECOMP];
    GroundData gd;
    int graphicSet;
    int fileNum, levelInFile;
    char path[260];
    int totalVgaSize, pi, vgaObjBase;
    int orderVal;

    memset(level, 0, sizeof(Level));

    if (levelNumber < 0 || levelNumber >= 30)
        return 0;
    orderVal = funOrder[levelNumber];
    if (orderVal < 0) orderVal = -orderVal;
    fileNum = orderVal / 10;
    levelInFile = orderVal % 10;

    wsprintf(path, "data\\LEVEL00%d.DAT", fileNum);
    if (!datOpen(&levelDat, path))
        return 0;

    if (levelInFile >= levelDat.numParts) {
        datClose(&levelDat);
        return 0;
    }

    if (datDecompress(&levelDat, levelInFile, levelRaw, sizeof(levelRaw)) != 2048) {
        datClose(&levelDat);
        return 0;
    }
    datClose(&levelDat);

    parseLevelInfo(levelRaw, info);
    graphicSet = info->graphicSet;

    wsprintf(path, "data\\GROUND%dO.DAT", graphicSet);
    {
        DWORD sz;
        BYTE *res = resLoad(path, &sz);
        if (!res || sz != 1056)
            return 0;
        memcpy(groundRaw, res, 1056);
    }

    parseGroundData(groundRaw, &gd);

    wsprintf(path, "data\\VGAGR%d.DAT", graphicSet);
    if (!datOpen(&vgaDat, path))
        return 0;

    totalVgaSize = 0;
    vgaObjBase = 0;
    for (pi = 0; pi < vgaDat.numParts; pi++) {
        int partSize = datDecompress(&vgaDat, pi, vgaRaw + totalVgaSize,
                                      sizeof(vgaRaw) - totalVgaSize);
        if (partSize <= 0)
            break;
        if (pi == 0)
            vgaObjBase = partSize;
        totalVgaSize += partSize;
    }
    datClose(&vgaDat);

    if (totalVgaSize <= 0)
        return 0;

    placeTerrain(level, &gd, vgaRaw, totalVgaSize, levelRaw);
    decodeObjSprites(&gd, vgaRaw + vgaObjBase, totalVgaSize - vgaObjBase);
    parseSteel(levelRaw, level);

    info->entranceX = info->startX + GAME_W / 2;
    info->entranceY = 20;
    parseLevelObjects(levelRaw, &gd, level, &info->entranceX, &info->entranceY);

    if (palette)
        memcpy(palette, gd.palette, sizeof(gd.palette));

    memcpy(level->info.name, info->name, sizeof(info->name));
    level->info.releaseRate = info->releaseRate;
    level->info.numLemmings = info->numLemmings;
    level->info.numToSave = info->numToSave;
    level->info.timeLimit = info->timeLimit;
    memcpy(level->info.skills, info->skills, sizeof(info->skills));
    level->info.startX = info->startX;
    level->info.graphicSet = info->graphicSet;
    level->info.entranceX = info->entranceX;
    level->info.entranceY = info->entranceY;

    return 1;
}
