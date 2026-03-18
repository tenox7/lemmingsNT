#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include <windows.h>

#define GAME_TITLE "Lemmings NT"

#define SCREEN_W 640
#define SCREEN_H 400
#define GAME_W 320
#define GAME_H 200
#define LEVEL_W 1600
#define LEVEL_H 160
#define PANEL_H 40

#define MAX_LEMMINGS 80
#define MAX_TERRAIN 400
#define MAX_OBJECTS 32
#define MAX_TRIGGERS 32

#define TRIG_NONE 0
#define TRIG_EXIT 1
#define TRIG_TRAP 4
#define TRIG_DROWN 5
#define TRIG_KILL 6

typedef struct {
    int type;
    int x1, y1, x2, y2;
} Trigger;

#define FPS 17
#define FRAME_MS (1000 / FPS)

typedef enum {
    ACT_WALK,
    ACT_FALL,
    ACT_DIG,
    ACT_CLIMB,
    ACT_BUILD,
    ACT_BASH,
    ACT_MINE,
    ACT_FLOAT,
    ACT_BLOCK,
    ACT_EXPLODE,
    ACT_EXIT,
    ACT_SPLAT,
    ACT_DROWN,
    ACT_COUNT
} LemAction;

typedef struct {
    int x, y;
    int dx;
    int frame;
    int alive;
    int exited;
    LemAction action;
    int fallDist;
    int state;
    int bombTimer;
    int canClimb;
    int hasFloat;
} Lemming;

typedef struct {
    int releaseRate;
    int numLemmings;
    int numToSave;
    int timeLimit;
    int skills[8];
    int startX;
    int graphicSet;
    int entranceX;
    int entranceY;
    char name[33];
} LevelInfo;

typedef struct {
    int x, y;
    int objId;
    int frame;
    int animDone;
} PlacedObj;

typedef struct {
    BYTE terrain[LEVEL_W * LEVEL_H];
    BYTE visual[LEVEL_W * LEVEL_H];
    LevelInfo info;
    Trigger triggers[MAX_TRIGGERS];
    int numTriggers;
    PlacedObj objs[MAX_OBJECTS];
    int numObjs;
} Level;

typedef struct {
    Level level;
    Lemming lems[MAX_LEMMINGS];
    int numLems;
    int numExited;
    int numDead;
    int released;
    int releaseTimer;
    int cameraX;
    int frameCount;
    int paused;
    int gameOver;
    int skillSel;
    int running;
    int levelNum;
} GameState;

extern GameState game;

int gameInit(HWND hwnd);
int gameShutdown(HWND hwnd);
int gameUpdate(HWND hwnd);
int gameRender(HWND hwnd);
int gameKeyDown(int key);
int gameKeyUp(int key);
int gameClick(int mx, int my);

#endif /* GAME_H_INCLUDED */
