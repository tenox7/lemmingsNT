#ifndef SPRITE_H_INCLUDED
#define SPRITE_H_INCLUDED

#include <windows.h>
#include "game.h"

#define ANIM_WALK_R  0
#define ANIM_WALK_L  1
#define ANIM_FALL_R  2
#define ANIM_FALL_L  3
#define ANIM_DIG     4
#define ANIM_SPLAT   5
#define ANIM_EXIT    6
#define ANIM_COUNT   7

#define SPRITE_W 16
#define SPRITE_H 16
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

int spriteInit(int dummy);
Animation *spriteGetAnim(int animId);

#endif /* SPRITE_H_INCLUDED */
