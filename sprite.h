#ifndef SPRITE_H_INCLUDED
#define SPRITE_H_INCLUDED

#include <windows.h>
#include "game.h"

#define ANIM_WALK_R   0
#define ANIM_WALK_L   1
#define ANIM_FALL_R   2
#define ANIM_FALL_L   3
#define ANIM_DIG      4
#define ANIM_SPLAT    5
#define ANIM_EXIT     6
#define ANIM_CLIMB_R  7
#define ANIM_CLIMB_L  8
#define ANIM_BUILD_R  9
#define ANIM_BUILD_L  10
#define ANIM_BASH_R   11
#define ANIM_BASH_L   12
#define ANIM_MINE_R   13
#define ANIM_MINE_L   14
#define ANIM_FLOAT_R  15
#define ANIM_FLOAT_L  16
#define ANIM_BLOCK    17
#define ANIM_OHNO     18
#define ANIM_EXPLODE  19
#define ANIM_COUNT    20

#define SPRITE_W 32
#define SPRITE_H 32
#define MAX_ANIM_FRAMES 32

typedef struct {
    BYTE pixels[SPRITE_W * SPRITE_H];
} SpriteFrame;

typedef struct {
    SpriteFrame frames[MAX_ANIM_FRAMES];
    int numFrames;
    int width;
    int height;
    int footX;
    int footY;
} Animation;

#define MASK_BASH_R   0
#define MASK_BASH_L   1
#define MASK_MINE_R   2
#define MASK_MINE_L   3
#define MASK_EXPLODE  4

int spriteInit(int dummy);
Animation *spriteGetAnim(int animId);
BYTE *spriteGetPanel(int dummy);
BYTE *spriteGetMask(int type, int frame);

#endif /* SPRITE_H_INCLUDED */
