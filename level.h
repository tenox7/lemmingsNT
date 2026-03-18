#ifndef LEVEL_H_INCLUDED
#define LEVEL_H_INCLUDED

#include <windows.h>
#include "game.h"

#define MAX_GROUND_OBJECTS 16
#define MAX_GROUND_TERRAINS 64

typedef struct {
    int width;
    int height;
    int imageLoc;
    int maskLoc;
} TerrainMeta;

typedef struct {
    int flags;
    int firstFrame;
    int frameCount;
    int width;
    int height;
    int frameDataSize;
    int maskLoc;
    int imageLoc;
    int triggerLeft;
    int triggerTop;
    int triggerWidth;
    int triggerHeight;
    int triggerEffect;
    int trapSoundId;
} ObjectMeta;

typedef struct {
    ObjectMeta objects[MAX_GROUND_OBJECTS];
    TerrainMeta terrains[MAX_GROUND_TERRAINS];
    int numObjects;
    int numTerrains;
    RGBQUAD palette[16];
} GroundData;

typedef struct {
    int x, y;
    int id;
    int isUpsideDown;
    int noOverwrite;
    int isErase;
} LevelTerrain;

typedef struct {
    int x, y;
    int id;
    int isUpsideDown;
    int noOverwrite;
    int onlyOverwrite;
} LevelObject;

typedef struct {
    int x, y;
    int width, height;
} LevelSteel;

#define MAX_LEVEL_TERRAIN 400
#define MAX_LEVEL_OBJECTS 32
#define MAX_LEVEL_STEEL 32

#define MAX_OBJ_DIM 64
#define MAX_OBJ_AFRAMES 16

#define OBJ_ANIM_NONE       0
#define OBJ_ANIM_TRIGGERED  1
#define OBJ_ANIM_CONTINUOUS 2
#define OBJ_ANIM_ONCE       3

typedef struct {
    BYTE pixels[MAX_OBJ_AFRAMES][MAX_OBJ_DIM * MAX_OBJ_DIM];
    int width, height;
    int numFrames;
    int animType;
} ObjSpriteData;

int levelLoad(int levelNumber, Level *level, LevelInfo *info, RGBQUAD *palette);
ObjSpriteData *levelGetObjSprite(int objId);

#endif /* LEVEL_H_INCLUDED */
