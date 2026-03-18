#include "game.h"
#include "render.h"
#include "level.h"
#include "sprite.h"
#include <stdio.h>

GameState game;

static int levelLoaded = 0;
static RGBQUAD levelPalette[16];
static BYTE keyState[256];
static int spawnX = 0;
static int spawnY = 0;

static const char *skillNames[8] = {
    "CLIMB","FLOAT","BOMB","BLOCK","BUILD","BASH","MINE","DIG"
};

static int loadLevel(HWND hwnd, int num) {
    if (num < 0) num = 0;
    if (num > 29) num = 29;

    game.levelNum = num;
    game.numLems = 0;
    game.numExited = 0;
    game.numDead = 0;
    game.frameCount = 0;
    game.releaseTimer = 90;
    game.paused = 0;
    game.gameOver = 0;

    levelLoaded = levelLoad(num, &game.level, &game.level.info, levelPalette);
    if (!levelLoaded)
        return 0;

    renderSetPalette(levelPalette, 0, 16);
    game.cameraX = game.level.info.startX;
    if (game.cameraX > LEVEL_W - GAME_W)
        game.cameraX = LEVEL_W - GAME_W;
    if (game.cameraX < 0)
        game.cameraX = 0;

    spawnX = game.level.info.entranceX;
    spawnY = game.level.info.entranceY;
    return 1;
}

int gameInit(HWND hwnd) {
    memset(&game, 0, sizeof(GameState));
    memset(keyState, 0, sizeof(keyState));
    game.running = 1;
    game.skillSel = 7;

    if (!renderInit(hwnd))
        return 0;

    spriteInit(0);
    loadLevel(hwnd, 0);
    return 1;
}

int gameShutdown(HWND hwnd) {
    renderShutdown(hwnd);
    return 1;
}

int gameKeyDown(int key) {
    if (key >= 0 && key < 256)
        keyState[key] = 1;

    if (key >= '1' && key <= '8')
        game.skillSel = key - '1';
    if (key == 'P')
        game.paused = !game.paused;
    if (key == VK_F1)
        game.skillSel = 0;
    if (key == VK_F2)
        game.skillSel = 1;
    if (key == VK_F3)
        game.skillSel = 2;
    if (key == VK_F4)
        game.skillSel = 3;
    if (key == VK_F5)
        game.skillSel = 4;
    if (key == VK_F6)
        game.skillSel = 5;
    if (key == VK_F7)
        game.skillSel = 6;
    if (key == VK_F8)
        game.skillSel = 7;
    return 1;
}

int gameKeyUp(int key) {
    if (key >= 0 && key < 256)
        keyState[key] = 0;
    return 1;
}

static int assignSkill(Lemming *l, int skill) {
    LemAction act;

    switch (skill) {
    case 0:
        if (l->canClimb)
            return 0;
        l->canClimb = 1;
        return 1;
    case 1:
        if (l->hasFloat)
            return 0;
        l->hasFloat = 1;
        return 1;
    case 2:
        if (l->bombTimer > 0)
            return 0;
        l->bombTimer = 80;
        return 1;
    case 3: act = ACT_BLOCK; break;
    case 4: act = ACT_BUILD; break;
    case 5: act = ACT_BASH; break;
    case 6: act = ACT_MINE; break;
    case 7: act = ACT_DIG; break;
    default: return 0;
    }

    if (l->action == act)
        return 0;
    if (l->action == ACT_BLOCK || l->action == ACT_EXIT || l->action == ACT_SPLAT)
        return 0;

    l->action = act;
    l->frame = 0;
    l->state = 0;
    return 1;
}

int gameClick(int mx, int my) {
    int worldX = mx + game.cameraX;
    int worldY = my;
    int i, bestI, bestDist;

    {
        FILE *fp = fopen("click.log", "a");
        if (fp) {
            fprintf(fp, "mx=%d my=%d worldX=%d sel=%d nlems=%d\n",
                    mx, my, worldX, game.skillSel, game.numLems);
            fclose(fp);
        }
    }

    if (my >= LEVEL_H) {
        int panelIdx = mx / 16;
        if (panelIdx >= 2 && panelIdx <= 9)
            game.skillSel = panelIdx - 2;
        return 1;
    }

    bestI = -1;
    bestDist = 999999;
    for (i = 0; i < game.numLems; i++) {
        int dx, dy, dist;
        Lemming *l = &game.lems[i];
        if (!l->alive || l->exited)
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
        int ok = assignSkill(&game.lems[bestI], game.skillSel);
        {
            FILE *fp = fopen("click.log", "a");
            if (fp) {
                fprintf(fp, "click lem=%d skill=%d ok=%d act=%d bomb=%d\n",
                        bestI, game.skillSel, ok,
                        game.lems[bestI].action, game.lems[bestI].bombTimer);
                fclose(fp);
            }
        }
    }
    return 1;
}

static int drawChar(BYTE *buf, int px, int py, char ch, BYTE color);

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

static int placeTerrain(int x, int y, BYTE color) {
    int idx;
    if (x < 0 || x >= LEVEL_W || y < 0 || y >= LEVEL_H)
        return 0;
    idx = y * LEVEL_W + x;
    game.level.terrain[idx] = 1;
    game.level.visual[idx] = color;
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

static int bashColumn(int x, int y, int dir) {
    int by, count = 0;
    for (by = y - 9; by <= y - 1; by++)
        count += removeTerrain(x, by);
    return count;
}

static int updateLemming(Lemming *l) {
    int i, upDelta, downDelta;

    if (!l->alive || l->exited)
        return 0;

    l->frame++;

    if (l->bombTimer > 0) {
        l->bombTimer--;
        if (l->bombTimer <= 0 && l->action != ACT_EXPLODE) {
            l->action = ACT_EXPLODE;
            l->frame = 0;
            l->bombTimer = -50;
        }
    }
    if (l->bombTimer < 0) {
        l->bombTimer++;
        if (l->bombTimer == -25) {
            int ex, ey;
            int cx = l->x;
            int cy = l->y - 3;
            int rx = 8;
            int ry = 11;
            for (ey = cy - ry; ey <= cy + ry; ey++)
                for (ex = cx - rx; ex <= cx + rx; ex++) {
                    int dx = ex - cx;
                    int dy = (ey - cy) * rx / ry;
                    if (dx * dx + dy * dy <= rx * rx)
                        removeTerrain(ex, ey);
                }
        }
        if (l->bombTimer >= 0) {
            l->alive = 0;
            game.numDead++;
            return 0;
        }
    }

    switch (l->action) {
    case ACT_FALL:
        if (l->hasFloat && l->fallDist > 8) {
            l->action = ACT_FLOAT;
            l->frame = 0;
            break;
        }
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

    case ACT_FLOAT:
        l->frame++;
        for (i = 0; i < 2; i++) {
            if (terrainAt(l->x, l->y + 1)) {
                l->action = ACT_WALK;
                l->frame = 0;
                l->fallDist = 0;
                return 1;
            }
            l->y++;
        }
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
            if (l->canClimb) {
                l->action = ACT_CLIMB;
                l->frame = 0;
                break;
            }
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
        if (l->state == 0) {
            digRow(l->x, l->y - 2);
            digRow(l->x, l->y - 1);
            l->state = 1;
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

    case ACT_BASH: {
        int state = l->frame % 16;
        if (state >= 11) {
            l->x += l->dx;
            downDelta = getStepDown(l->x, l->y);
            l->y += downDelta;
            if (downDelta >= 3) {
                l->action = ACT_FALL;
                l->frame = 0;
                l->fallDist = 0;
                break;
            }
        }
        if (state >= 2 && state < 6) {
            int bx;
            for (bx = -1; bx <= 1; bx++)
                bashColumn(l->x + l->dx * (4 + state + bx), l->y, l->dx);
        }
        if (state == 5) {
            int checkX = l->x + l->dx * 8;
            int empty = 1;
            for (i = -2; i < 4; i++) {
                if (terrainAt(checkX, l->y + i)) {
                    empty = 0;
                    break;
                }
            }
            if (empty) {
                l->action = ACT_WALK;
                l->frame = 0;
            }
        }
        break;
    }

    case ACT_MINE: {
        int state = l->frame % 24;
        if (state >= 1 && state <= 2) {
            int mx, my;
            for (my = l->y - 3; my <= l->y + 1; my++)
                for (mx = l->x - 2; mx <= l->x + 2; mx++)
                    removeTerrain(mx + l->dx * 3, my);
        }
        if (state == 15) {
            l->x += l->dx;
            l->y++;
            if (l->y >= LEVEL_H || !terrainAt(l->x + l->dx, l->y + 1)) {
                l->action = ACT_FALL;
                l->frame = 0;
                l->fallDist = 0;
            }
        }
        break;
    }

    case ACT_BUILD: {
        int state = l->frame % 16;
        if (state == 9) {
            int bx;
            int startX = l->x + ((l->dx > 0) ? 0 : -5);
            for (bx = 0; bx < 6; bx++)
                placeTerrain(startX + bx, l->y - 1, 9);
        }
        if (state == 0 && l->frame > 1) {
            l->y--;
            l->x += l->dx * 2;
            if (terrainAt(l->x, l->y - 8)) {
                l->dx = -l->dx;
                l->action = ACT_WALK;
                l->frame = 0;
                break;
            }
            l->state++;
            if (l->state >= 12) {
                l->action = ACT_WALK;
                l->frame = 0;
                break;
            }
        }
        break;
    }

    case ACT_CLIMB:
        if (l->frame % 8 < 4) {
            if (!terrainAt(l->x, l->y - (l->frame % 4) - 7)) {
                l->y = l->y - (l->frame % 4) + 2;
                l->action = ACT_WALK;
                l->frame = 0;
                break;
            }
        } else {
            l->y--;
            if (l->y < 0) {
                l->alive = 0;
                game.numDead++;
                break;
            }
            if (terrainAt(l->x + ((l->dx > 0) ? -1 : 1), l->y - 8)) {
                l->dx = -l->dx;
                l->action = ACT_FALL;
                l->frame = 0;
                l->fallDist = 0;
            }
        }
        break;

    case ACT_BLOCK:
        break;

    case ACT_EXPLODE:
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
    if (keyState[VK_PRIOR]) {
        keyState[VK_PRIOR] = 0;
        loadLevel(hwnd, game.levelNum + 1);
    }
    if (keyState[VK_NEXT]) {
        keyState[VK_NEXT] = 0;
        loadLevel(hwnd, game.levelNum - 1);
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
    case ACT_CLIMB:
        animId = (l->dx > 0) ? ANIM_CLIMB_R : ANIM_CLIMB_L;
        break;
    case ACT_BUILD:
        animId = (l->dx > 0) ? ANIM_BUILD_R : ANIM_BUILD_L;
        break;
    case ACT_BASH:
        animId = (l->dx > 0) ? ANIM_BASH_R : ANIM_BASH_L;
        break;
    case ACT_MINE:
        animId = (l->dx > 0) ? ANIM_MINE_R : ANIM_MINE_L;
        break;
    case ACT_FLOAT:
        animId = (l->dx > 0) ? ANIM_FLOAT_R : ANIM_FLOAT_L;
        break;
    case ACT_BLOCK:
        animId = ANIM_BLOCK;
        break;
    case ACT_EXPLODE:
        if (l->bombTimer <= -25)
            animId = ANIM_OHNO;
        else
            animId = ANIM_EXPLODE;
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
        if (destY < 0 || destY >= LEVEL_H)
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

    if (l->bombTimer > 0) {
        int num = (l->bombTimer / 17) + 1;
        char tmp[4];
        if (num > 5) num = 5;
        wsprintf(tmp, "%d", num);
        drawChar(buf, l->x - camX - 2, l->y - anim->footY - 7, tmp[0], 15);
    }

    return 1;
}

static const BYTE font3x5[128][5] = {
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    {0x70,0x50,0x50,0x50,0x70},
    {0x20,0x20,0x20,0x20,0x20},
    {0x70,0x10,0x70,0x40,0x70},
    {0x70,0x10,0x70,0x10,0x70},
    {0x50,0x50,0x70,0x10,0x10},
    {0x70,0x40,0x70,0x10,0x70},
    {0x70,0x40,0x70,0x50,0x70},
    {0x70,0x10,0x10,0x10,0x10},
    {0x70,0x50,0x70,0x50,0x70},
    {0x70,0x50,0x70,0x10,0x70},
    {0},{0},{0},{0},{0},{0},{0},
    {0x20,0x50,0x70,0x50,0x50},
    {0x60,0x50,0x60,0x50,0x60},
    {0x70,0x40,0x40,0x40,0x70},
    {0x60,0x50,0x50,0x50,0x60},
    {0x70,0x40,0x60,0x40,0x70},
    {0x70,0x40,0x60,0x40,0x40},
    {0x70,0x40,0x50,0x50,0x70},
    {0x50,0x50,0x70,0x50,0x50},
    {0x70,0x20,0x20,0x20,0x70},
    {0x10,0x10,0x10,0x50,0x20},
    {0x50,0x50,0x60,0x50,0x50},
    {0x40,0x40,0x40,0x40,0x70},
    {0x50,0x70,0x70,0x50,0x50},
    {0x50,0x70,0x70,0x50,0x50},
    {0x20,0x50,0x50,0x50,0x20},
    {0x60,0x50,0x60,0x40,0x40},
    {0x20,0x50,0x50,0x50,0x30},
    {0x60,0x50,0x60,0x50,0x50},
    {0x70,0x40,0x70,0x10,0x70},
    {0x70,0x20,0x20,0x20,0x20},
    {0x50,0x50,0x50,0x50,0x70},
    {0x50,0x50,0x50,0x50,0x20},
    {0x50,0x50,0x70,0x70,0x50},
    {0x50,0x50,0x20,0x50,0x50},
    {0x50,0x50,0x20,0x20,0x20},
    {0x70,0x10,0x20,0x40,0x70}
};

static int drawChar(BYTE *buf, int px, int py, char ch, BYTE color) {
    int r, c;
    int idx = (int)(unsigned char)ch;
    if (idx >= 128) return 0;
    for (r = 0; r < 5; r++)
        for (c = 0; c < 4; c++) {
            if (font3x5[idx][r] & (0x80 >> c)) {
                int dx = px + c;
                int dy = py + r;
                if (dx >= 0 && dx < GAME_W && dy >= 0 && dy < GAME_H)
                    buf[dy * GAME_W + dx] = color;
            }
        }
    return 4;
}

static int drawString(BYTE *buf, int px, int py, const char *s, BYTE color) {
    int ox = px;
    while (*s) {
        px += drawChar(buf, px, py, *s, color);
        s++;
    }
    return px - ox;
}

static const int panelAnimIds[8] = {
    ANIM_CLIMB_R, ANIM_FLOAT_R, ANIM_OHNO, ANIM_BLOCK,
    ANIM_BUILD_R, ANIM_BASH_R, ANIM_MINE_R, ANIM_DIG
};

static int drawPanelSprite(BYTE *buf, int px, int py, int animId, int frame) {
    Animation *anim = spriteGetAnim(animId);
    SpriteFrame *sf;
    int sx, sy;

    if (anim->numFrames == 0)
        return 0;

    sf = &anim->frames[frame % anim->numFrames];
    for (sy = 0; sy < anim->height; sy++) {
        int dy = py + sy;
        if (dy < 0 || dy >= GAME_H)
            continue;
        for (sx = 0; sx < anim->width; sx++) {
            int dx = px + sx;
            BYTE pixel;
            if (dx < 0 || dx >= GAME_W)
                continue;
            pixel = sf->pixels[sy * SPRITE_W + sx];
            if (pixel == 0)
                continue;
            buf[dy * GAME_W + dx] = pixel;
        }
    }
    return 1;
}

static int drawPanel(BYTE *buf) {
    BYTE *panel = spriteGetPanel(0);
    int x, y, i;
    int panelY = LEVEL_H;

    if (panel) {
        for (y = 0; y < PANEL_H; y++)
            for (x = 0; x < GAME_W; x++) {
                BYTE px = panel[y * 320 + x];
                if (px != 0)
                    buf[(panelY + y) * GAME_W + x] = px;
            }
    }

    for (i = 0; i < 8; i++) {
        int nx = 4 + (i + 2) * 16;
        int ny = panelY + 19;
        drawString(buf, nx, ny, "99", 15);
    }

    if (game.skillSel >= 0 && game.skillSel < 8) {
        int sx = (game.skillSel + 2) * 16;
        int sy = panelY + 16;
        int sw = 16;
        int sh = 23;
        BYTE rc = 15;
        for (x = sx; x < sx + sw; x++) {
            buf[sy * GAME_W + x] = rc;
            buf[(sy + 1) * GAME_W + x] = rc;
            buf[(sy + sh - 2) * GAME_W + x] = rc;
            buf[(sy + sh - 1) * GAME_W + x] = rc;
        }
        for (y = sy; y < sy + sh; y++) {
            buf[y * GAME_W + sx] = rc;
            buf[y * GAME_W + sx + 1] = rc;
            buf[y * GAME_W + sx + sw - 2] = rc;
            buf[y * GAME_W + sx + sw - 1] = rc;
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

    if (!levelLoaded)
        return 1;

    camX = game.cameraX;
    for (y = 0; y < LEVEL_H && y < GAME_H; y++)
        for (x = 0; x < GAME_W; x++) {
            int srcX = camX + x;
            if (srcX >= 0 && srcX < LEVEL_W)
                buf[y * GAME_W + x] = game.level.visual[y * LEVEL_W + srcX];
        }

    for (i = 0; i < game.numLems; i++)
        drawLemming(buf, &game.lems[i], camX);

    drawPanel(buf);

    renderFrame(hwnd);
    return 1;
}
