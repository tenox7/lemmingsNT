#include "dat.h"
#include <stdio.h>
#include <string.h>

static WORD readBE16(BYTE *p) {
    return (WORD)((p[0] << 8) | p[1]);
}

int datOpen(DatFile *dat, const char *path) {
    HANDLE hFile;
    DWORD fileSize, bytesRead;

    memset(dat, 0, sizeof(DatFile));

    hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > 1024 * 1024) {
        CloseHandle(hFile);
        return 0;
    }

    dat->data = (BYTE *)GlobalAlloc(GPTR, fileSize);
    if (!dat->data) {
        CloseHandle(hFile);
        return 0;
    }

    ReadFile(hFile, dat->data, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    if (bytesRead != fileSize) {
        GlobalFree(dat->data);
        dat->data = NULL;
        return 0;
    }

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
