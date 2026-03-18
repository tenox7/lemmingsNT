#include "game.h"
#include "render.h"
#include "level.h"
#include "sprite.h"

GameState game;

static int levelLoaded = 0;
static RGBQUAD levelPalette[16];
static BYTE keyState[256];
static int spawnX = 0;
static int spawnY = 0;

int gameInit(HWND hwnd) {
    memset(&game, 0, sizeof(GameState));
    memset(keyState, 0, sizeof(keyState));
    game.running = 1;
    game.cameraX = 0;

    if (!renderInit(hwnd))
        return 0;

    spriteInit(0);

    levelLoaded = levelLoad(0, &game.level, &game.level.info, levelPalette);
    if (levelLoaded) {
        renderSetPalette(levelPalette, 0, 16);
        game.cameraX = game.level.info.startX;
        if (game.cameraX > LEVEL_W - GAME_W)
            game.cameraX = LEVEL_W - GAME_W;
        if (game.cameraX < 0)
            game.cameraX = 0;

        spawnX = game.level.info.entranceX;
        spawnY = game.level.info.entranceY;
        game.releaseTimer = 90;
    }

    return 1;
}

int gameShutdown(HWND hwnd) {
    renderShutdown(hwnd);
    return 1;
}

int gameKeyDown(int key) {
    if (key >= 0 && key < 256)
        keyState[key] = 1;
    return 1;
}

int gameKeyUp(int key) {
    if (key >= 0 && key < 256)
        keyState[key] = 0;
    return 1;
}

int gameClick(int mx, int my) {
    int worldX = mx + game.cameraX;
    int worldY = my;
    int i, bestI, bestDist;

    if (my >= LEVEL_H)
        return 0;

    bestI = -1;
    bestDist = 999999;
    for (i = 0; i < game.numLems; i++) {
        int dx, dy, dist;
        Lemming *l = &game.lems[i];
        if (!l->alive || l->exited)
            continue;
        if (l->action == ACT_DIG || l->action == ACT_EXIT)
            continue;
        dx = l->x - worldX;
        dy = (l->y - 5) - worldY;
        if (dx < -5 || dx > 5 || dy < -6 || dy > 7)
            continue;
        dist = dx * dx + dy * dy;
        if (dist < bestDist) {
            bestDist = dist;
            bestI = i;
        }
    }

    if (bestI >= 0) {
        Lemming *l = &game.lems[bestI];
        if (game.level.info.skills[7] > 0) {
            l->action = ACT_DIG;
            l->frame = 0;
            l->countdown = 0;
            game.level.info.skills[7]--;
        }
    }
    return 1;
}

static int removeTerrain(int x, int y) {
    int idx;
    if (x < 0 || x >= LEVEL_W || y < 0 || y >= LEVEL_H)
        return 0;
    idx = y * LEVEL_W + x;
    if (game.level.terrain[idx] == 0)
        return 0;
    if (game.level.terrain[idx] == 2)
        return 0;
    game.level.terrain[idx] = 0;
    game.level.visual[idx] = 0;
    return 1;
}

static int digRow(int cx, int y) {
    int x, count = 0;
    for (x = cx - 4; x <= cx + 4; x++)
        count += removeTerrain(x, y);
    return count;
}

static int terrainAt(int x, int y) {
    if (x < 0 || x >= LEVEL_W || y < 0 || y >= LEVEL_H)
        return 0;
    return game.level.terrain[y * LEVEL_W + x];
}

static int spawnLemming(int dummy) {
    Lemming *l;

    if (game.numLems >= game.level.info.numLemmings)
        return 0;
    if (game.numLems >= MAX_LEMMINGS)
        return 0;

    l = &game.lems[game.numLems];
    memset(l, 0, sizeof(Lemming));
    l->x = spawnX;
    l->y = spawnY;
    l->dx = 1;
    l->alive = 1;
    l->action = ACT_FALL;
    l->frame = 0;
    l->fallDist = 0;
    game.numLems++;
    return 1;
}

static int checkTrigger(Lemming *l) {
    int i;
    for (i = 0; i < game.level.numTriggers; i++) {
        Trigger *t = &game.level.triggers[i];
        if (l->x >= t->x1 && l->x < t->x2 &&
            l->y >= t->y1 && l->y < t->y2) {
            switch (t->type) {
            case TRIG_EXIT:
                l->action = ACT_EXIT;
                l->frame = 0;
                return 1;
            case TRIG_KILL:
            case TRIG_TRAP:
            case TRIG_DROWN:
                l->alive = 0;
                game.numDead++;
                return 1;
            }
        }
    }
    return 0;
}

static int getStepUp(int x, int y) {
    int i;
    for (i = 0; i < 8; i++) {
        if (!terrainAt(x, y - i))
            return i;
    }
    return 8;
}

static int getStepDown(int x, int y) {
    int i;
    for (i = 1; i < 4; i++) {
        if (terrainAt(x, y + i))
            return i;
    }
    return 4;
}

static int updateLemming(Lemming *l) {
    int i, upDelta, downDelta;

    if (!l->alive || l->exited)
        return 0;

    l->frame++;

    switch (l->action) {
    case ACT_FALL:
        for (i = 0; i < 3; i++) {
            if (terrainAt(l->x, l->y + i)) {
                l->y += i;
                if (l->fallDist > 60) {
                    l->action = ACT_SPLAT;
                    l->frame = 0;
                } else {
                    l->action = ACT_WALK;
                    l->frame = 0;
                }
                l->fallDist = 0;
                return 1;
            }
        }
        l->y += 3;
        l->fallDist += 3;
        if (l->y >= LEVEL_H + 10) {
            l->alive = 0;
            game.numDead++;
            return 0;
        }
        break;

    case ACT_WALK:
        l->x += l->dx;
        upDelta = getStepUp(l->x, l->y);

        if (upDelta == 8) {
            l->dx = -l->dx;
            l->x += l->dx;
            l->frame = 0;
            break;
        }

        if (upDelta > 0) {
            l->y -= (upDelta - 1);
        } else {
            downDelta = getStepDown(l->x, l->y);
            l->y += downDelta;
            if (downDelta >= 4) {
                l->action = ACT_FALL;
                l->frame = 0;
                l->fallDist = 0;
                break;
            }
        }

        checkTrigger(l);
        break;

    case ACT_DIG:
        if (l->countdown == 0) {
            digRow(l->x, l->y - 2);
            digRow(l->x, l->y - 1);
            l->countdown = 1;
        }
        if ((l->frame & 0x07) == 0) {
            l->y++;
            if (l->y >= LEVEL_H) {
                l->action = ACT_FALL;
                l->frame = 0;
                l->fallDist = 0;
                break;
            }
            if (digRow(l->x, l->y - 1) == 0) {
                l->action = ACT_FALL;
                l->frame = 0;
                l->fallDist = 0;
            }
        }
        break;

    case ACT_EXIT:
        if (l->frame > 16) {
            l->exited = 1;
            game.numExited++;
        }
        break;

    case ACT_SPLAT:
        if (l->frame > 32) {
            l->alive = 0;
            game.numDead++;
        }
        break;

    default:
        break;
    }

    return 1;
}

int gameUpdate(HWND hwnd) {
    int scrollSpeed = 4;
    int i;

    if (keyState[VK_LEFT]) {
        game.cameraX -= scrollSpeed;
        if (game.cameraX < 0)
            game.cameraX = 0;
    }
    if (keyState[VK_RIGHT]) {
        game.cameraX += scrollSpeed;
        if (game.cameraX > LEVEL_W - GAME_W)
            game.cameraX = LEVEL_W - GAME_W;
    }

    if (game.paused || game.gameOver)
        return 1;

    game.releaseTimer++;
    if (game.releaseTimer >= (104 - game.level.info.releaseRate)) {
        game.releaseTimer = 0;
        spawnLemming(0);
    }

    for (i = 0; i < game.numLems; i++)
        updateLemming(&game.lems[i]);

    game.frameCount++;

    return 1;
}

static int drawLemming(BYTE *buf, Lemming *l, int camX) {
    Animation *anim;
    SpriteFrame *sf;
    int animId;
    int screenX, screenY;
    int sx, sy;

    if (!l->alive || l->exited)
        return 0;

    switch (l->action) {
    case ACT_WALK:
        animId = (l->dx > 0) ? ANIM_WALK_R : ANIM_WALK_L;
        break;
    case ACT_FALL:
        animId = (l->dx > 0) ? ANIM_FALL_R : ANIM_FALL_L;
        break;
    case ACT_SPLAT:
        animId = ANIM_SPLAT;
        break;
    case ACT_EXIT:
        animId = ANIM_EXIT;
        break;
    case ACT_DIG:
        animId = ANIM_DIG;
        break;
    default:
        animId = ANIM_WALK_R;
        break;
    }

    anim = spriteGetAnim(animId);
    if (anim->numFrames == 0)
        return 0;

    sf = &anim->frames[(l->frame / 2) % anim->numFrames];
    screenX = l->x - anim->footX - camX;
    screenY = l->y - anim->footY;

    for (sy = 0; sy < anim->height; sy++) {
        int destY = screenY + sy;
        if (destY < 0 || destY >= GAME_H)
            continue;
        for (sx = 0; sx < anim->width; sx++) {
            int destX = screenX + sx;
            BYTE pixel;
            if (destX < 0 || destX >= GAME_W)
                continue;
            pixel = sf->pixels[sy * SPRITE_W + sx];
            if (pixel == 0)
                continue;
            buf[destY * GAME_W + destX] = pixel;
        }
    }

    return 1;
}

int gameRender(HWND hwnd) {
    BYTE *buf;
    int x, y, i;
    int camX;

    renderClear(0);
    buf = renderGetBuffer(0);

    if (!levelLoaded) {
        for (x = 0; x < GAME_W; x++)
            for (y = GAME_H - 30; y < GAME_H; y++)
                buf[y * GAME_W + x] = 1;
    } else {
        camX = game.cameraX;
        for (y = 0; y < LEVEL_H && y < GAME_H; y++)
            for (x = 0; x < GAME_W; x++) {
                int srcX = camX + x;
                if (srcX >= 0 && srcX < LEVEL_W)
                    buf[y * GAME_W + x] = game.level.visual[y * LEVEL_W + srcX];
            }

        for (i = 0; i < game.numLems; i++)
            drawLemming(buf, &game.lems[i], camX);
    }

    renderFrame(hwnd);
    return 1;
}
