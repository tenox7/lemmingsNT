#ifndef DAT_H_INCLUDED
#define DAT_H_INCLUDED

#include <windows.h>

#define DAT_MAX_PARTS 32
#define DAT_MAX_DECOMP 131072

typedef struct {
    int initialBufLen;
    int checksum;
    int decompSize;
    int compSize;
    int offset;
} DatPart;

typedef struct {
    BYTE *data;
    int length;
    DatPart parts[DAT_MAX_PARTS];
    int numParts;
} DatFile;

int datOpen(DatFile *dat, const char *path);
int datClose(DatFile *dat);
int datDecompress(DatFile *dat, int partIndex, BYTE *out, int outSize);

#endif /* DAT_H_INCLUDED */
