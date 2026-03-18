#include "dat.h"
#include "resource.h"
#include <stdio.h>
#include <string.h>

static WORD readBE16(BYTE *p) {
    return (WORD)((p[0] << 8) | p[1]);
}

static int pathToResId(const char *path) {
    const char *name;
    name = strrchr(path, '\\');
    if (name) name++; else name = path;

    if (strncmp(name, "LEVEL00", 7) == 0 && name[7] >= '0' && name[7] <= '9')
        return IDR_LEVEL000 + (name[7] - '0');
    if (strncmp(name, "GROUND", 6) == 0 && name[6] >= '0' && name[6] <= '4')
        return IDR_GROUND0O + (name[6] - '0');
    if (strncmp(name, "VGAGR", 5) == 0 && name[5] >= '0' && name[5] <= '4')
        return IDR_VGAGR0 + (name[5] - '0');
    if (strncmp(name, "VGASPEC", 7) == 0 && name[7] >= '0' && name[7] <= '3')
        return IDR_VGASPEC0 + (name[7] - '0');
    if (strncmp(name, "MAIN", 4) == 0) return IDR_MAIN;
    if (strncmp(name, "ODDTABLE", 8) == 0) return IDR_ODDTABLE;
    if (strncmp(name, "EXPLODE", 7) == 0) return IDR_EXPLODE;
    return 0;
}

BYTE *resLoad(const char *path, DWORD *outSize) {
    int resId = pathToResId(path);
    HRSRC hRes;
    HGLOBAL hData;

    if (!resId) { *outSize = 0; return NULL; }
    hRes = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resId), RT_RCDATA);
    if (!hRes) { *outSize = 0; return NULL; }
    *outSize = SizeofResource(GetModuleHandle(NULL), hRes);
    hData = LoadResource(GetModuleHandle(NULL), hRes);
    if (!hData) { *outSize = 0; return NULL; }
    return (BYTE *)LockResource(hData);
}

int datOpen(DatFile *dat, const char *path) {
    DWORD fileSize;
    BYTE *resPtr;

    memset(dat, 0, sizeof(DatFile));

    resPtr = resLoad(path, &fileSize);
    if (!resPtr || fileSize == 0)
        return 0;

    dat->data = (BYTE *)GlobalAlloc(GPTR, fileSize);
    if (!dat->data)
        return 0;
    memcpy(dat->data, resPtr, fileSize);

    dat->length = (int)fileSize;
    dat->numParts = 0;

    {
        int pos = 0;
        while (pos + 10 <= dat->length && dat->numParts < DAT_MAX_PARTS) {
            BYTE *h = dat->data + pos;
            int totalSize = (int)readBE16(h + 8);
            DatPart *p;

            if (totalSize < 10 || totalSize > 0xFFFFFF)
                break;
            if (pos + totalSize > dat->length)
                break;

            p = &dat->parts[dat->numParts];
            p->initialBufLen = h[0];
            p->checksum = h[1];
            p->decompSize = (int)readBE16(h + 4);
            p->compSize = totalSize - 10;
            p->offset = pos + 10;
            dat->numParts++;
            pos += totalSize;
        }
    }

    return dat->numParts > 0;
}

int datClose(DatFile *dat) {
    if (dat->data) {
        GlobalFree(dat->data);
        dat->data = NULL;
    }
    dat->numParts = 0;
    return 1;
}

typedef struct {
    BYTE *data;
    int pos;
    int buffer;
    int bufLen;
    int checksum;
} BitReader;

static int brInit(BitReader *br, BYTE *data, int offset, int length, int initBufLen) {
    br->data = data;
    br->pos = offset + length - 1;
    br->buffer = data[br->pos];
    br->bufLen = initBufLen;
    br->checksum = br->buffer;
    return 1;
}

static int brRead(BitReader *br, int bitCount) {
    int result = 0;
    int i;

    for (i = bitCount; i > 0; i--) {
        if (br->bufLen <= 0) {
            br->pos--;
            br->buffer = br->data[br->pos];
            br->checksum ^= br->buffer;
            br->bufLen = 8;
        }
        br->bufLen--;
        result = (result << 1) | (br->buffer & 1);
        br->buffer >>= 1;
    }
    return result;
}

int datDecompress(DatFile *dat, int partIndex, BYTE *out, int outSize) {
    DatPart *part;
    BitReader br;
    int outPos;

    if (partIndex < 0 || partIndex >= dat->numParts)
        return 0;

    part = &dat->parts[partIndex];
    if (part->decompSize > outSize)
        return 0;

    memset(out, 0, part->decompSize);
    brInit(&br, dat->data, part->offset, part->compSize, part->initialBufLen);
    outPos = part->decompSize;

    while (outPos > 0) {
        if (brRead(&br, 1) == 0) {
            if (brRead(&br, 1) == 0) {
                int count = brRead(&br, 3) + 1;
                int i;
                for (i = 0; i < count && outPos > 0; i++) {
                    outPos--;
                    out[outPos] = (BYTE)brRead(&br, 8);
                }
            } else {
                int offset = brRead(&br, 8) + 1;
                int i;
                for (i = 0; i < 2 && outPos > 0; i++) {
                    outPos--;
                    out[outPos] = out[outPos + offset];
                }
            }
        } else {
            int code = brRead(&br, 2);
            switch (code) {
            case 0: {
                int offset = brRead(&br, 9) + 1;
                int i;
                for (i = 0; i < 3 && outPos > 0; i++) {
                    outPos--;
                    out[outPos] = out[outPos + offset];
                }
                break;
            }
            case 1: {
                int offset = brRead(&br, 10) + 1;
                int i;
                for (i = 0; i < 4 && outPos > 0; i++) {
                    outPos--;
                    out[outPos] = out[outPos + offset];
                }
                break;
            }
            case 2: {
                int count = brRead(&br, 8) + 1;
                int offset = brRead(&br, 12) + 1;
                int i;
                for (i = 0; i < count && outPos > 0; i++) {
                    outPos--;
                    out[outPos] = out[outPos + offset];
                }
                break;
            }
            case 3: {
                int count = brRead(&br, 8) + 9;
                int i;
                for (i = 0; i < count && outPos > 0; i++) {
                    outPos--;
                    out[outPos] = (BYTE)brRead(&br, 8);
                }
                break;
            }
            }
        }
    }

    return part->decompSize;
}
