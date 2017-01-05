/*
 * Net Puzzle Arena
 *
 * Copyright (C) 2016 NovaSquirrel
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PUZZLE_HEADER
#define PUZZLE_HEADER
#define NO_STDIO_REDIRECT

//#define FAST_MODE 1
#define ENABLE_AUDIO 1
#define ENABLE_SOUNDS 1
//#define ENABLE_MUSIC 1
//#define DISPLAY_CHAIN_COUNT 1
#define MAX_MODIFIERS 15

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifdef ENABLE_AUDIO
#include <SDL2/SDL_mixer.h>
#endif
#include <SDL2/SDL_ttf.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#ifdef _WIN32
#else
#include <linux/limits.h>
#endif

extern int ScreenWidth, ScreenHeight;
extern SDL_Window *window;
extern SDL_Renderer *ScreenRenderer;
extern int retraces;
extern int TILE_W, TILE_H;
extern SDL_Renderer *ScreenRenderer;
extern SDL_Texture *TileSheet;
extern SDL_Texture *GameFont;
extern TTF_Font *ChatFont;
extern int DirX[];
extern int DirY[];
extern int ScaleFactor;
extern int quit;
extern struct Playfield Player1;
extern char *PrefPath;
extern float SFXVolume;
extern float BGMVolume;
extern char TempString[1024];

#ifdef ENABLE_AUDIO
extern Mix_Chunk *SampleSwap, *SampleDrop, *SampleDisappear, *SampleMove, *SampleCombo;
#endif

enum GameTypes {
  FRENZY,
  AVALANCHE,
  PILLARS,
  DICE_MATCH,
  REVERSI_BALL,
  COOKIE,
  STACKER,
};

enum Directions {
  EAST,
  SOUTHEAST,
  SOUTH,
  SOUTHWEST,
  WEST,
  NORTHWEST,
  NORTH,
  NORTHEAST
};

enum GameKey {
  KEY_LEFT,
  KEY_DOWN,
  KEY_UP,
  KEY_RIGHT,
  KEY_OK,
  KEY_BACK,
  KEY_PAUSE,
  KEY_ACTION,
  KEY_ITEM,
  KEY_ROTATE_L, // counter clockwise, also swap
  KEY_ROTATE_R, // clockwise, also swap
  KEY_LIFT,
  KEY_COUNT,
  KEY_SWAP = KEY_ROTATE_L
};

enum BlockColor {
  BLOCK_EMPTY,
  BLOCK_RED,
  BLOCK_GREEN,
  BLOCK_YELLOW,
  BLOCK_CYAN,
  BLOCK_PURPLE,
  BLOCK_BLUE,
  BLOCK_EXTRA1,
  BLOCK_EXTRA2,
  BLOCK_METAL,
  BLOCK_GARBAGE,
  BLOCK_GRAY,
  BLOCK_DISABLED
};

enum GameplayOptions {
  SWAP_INSTANTLY = 1,
  LIFT_WHILE_CLEARING = 2,
  PULL_BLOCK_HORIZONTAL = 4,
  MOUSE_CONTROL = 8,
  NO_AUTO_REPEAT = 16,
  INSTANT_LIFT = 32,
  STYLUS_CONTROL = 64
};

struct FallingChunk {
  int Timer, X, Y, Height;
  struct FallingChunk *Next;
};

struct GarbageSlab {
  int X, Y, Width, Height;
  int Clearing;
  int Timer1;       // timer for how long until blocks disappear
  int Timer2;       // timer for making individual blocks disappear
  int DisplayWidth; // width to display
  struct GarbageSlab *Next;
};

struct ComboNumber {
  int X, Y, Number, Flags, Timer;
  int Speed;
  struct ComboNumber *Next;
};

struct MatchRow {
  int Color; // color of the row
  int X, Y;  // position in the playfield of the row
  int Width; // how many blocks are in the row
  int Timer1; // timer for how long until blocks disappear
  int Timer2; // timer for making individual blocks disappear
  int DisplayX;     // \ used to make blocks gradually disappear
  int DisplayWidth; // /
  int Chain; // chain count
  struct MatchRow *Child, *Next;
};

struct JoypadKey {
  char Type; // [k]eyboard, [b]utton, [a]xe, [h]at
  int Which; // which keyboard key, button, axe, or hat?
  int Value; // SDL_HAT_UP and such, or 1 or -1
};

struct JoypadMapping {
  int Active;
  SDL_Joystick *Joy;
  struct JoypadKey Keys[KEY_COUNT][2];
};
#define ACTIVE_JOY_MAX 7
extern struct JoypadMapping ActiveJoysticks[ACTIVE_JOY_MAX];

struct Playfield {
  int GameType;
  int Width, Height;
  int CursorX, CursorY;
  int Paused;
  uint32_t Flags;
  int KeyDown[KEY_COUNT];
  int KeyLast[KEY_COUNT];
  int KeyNew[KEY_COUNT];
  int KeyRepeat[KEY_COUNT];
  int *Playfield;
  int ChainCounter;
  int ChainResetTimer;
  uint32_t Score;
  int MouseX, MouseY;

  // Frenzy
  int Rise;
  int LiftKeyOn, RiseStopTimer;
  int SwapTimer, SwapColor1, SwapColor2;
  struct MatchRow *Match;
  struct FallingChunk *FallingData;
  struct ComboNumber *ComboNumbers;
  struct GarbageSlab *GarbageSlabs;
  int SwapX, SwapY;

  // Falling blocks
  int FallTimer;
  int LockTimer;
  int Active;
  int Direction; // for Avalanche
  int SwapColor3; // for pillars
  int PieceCount; // number of pieces the player has placed so far

  // Game rules
  int MinMatchSize; // how many matching tiles are needed
  int ColorCount;
  int Difficulty; // easy, normal, difficult
  int GameSpeed; // rising speed and falling speed
  int Modifiers[MAX_MODIFIERS][2]; // 0 is type, 1 is value

  int Joystick;
};

// different parts of the playfield int
#define PF_COLOR        0x000ff
#define PF_CHAIN        0x0ff00
#define PF_CHAIN_ONE    0x00100
#define PF_JUST_LANDED  0x10000
enum TextDrawFlags {
  TEXT_CENTERED = 1,
  TEXT_WHITE = 2,
  TEXT_WRAPPED = 4,
  TEXT_FROM_BOTTOM = 8,
  TEXT_CHAIN = 16
};

void INIConfigHandler(const char *Group, const char *Item, const char *Value);
int ParseINI(FILE *File, void (*Handler)(const char *Group, const char *Item, const char *Value));
void SaveConfigINI();
void SDL_MessageBox(int Type, const char *Title, SDL_Window *Window, const char *fmt, ...);
void strlcpy(char *Destination, const char *Source, int MaxLength);
SDL_Surface *SDL_LoadImage(const char *FileName, int Flags);
SDL_Texture *LoadTexture(const char *FileName, int Flags);
void rectfill(SDL_Renderer *Bmp, int X1, int Y1, int X2, int Y2);
void rect(SDL_Renderer *Bmp, int X1, int Y1, int X2, int Y2);
void sblit(SDL_Surface* SrcBmp, SDL_Surface* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height);
void blit(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height);
void blitf(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height, SDL_RendererFlip Flip);
void blitz(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height, int Width2, int Height2);
void blitfull(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int DestX, int DestY);
void DrawText(SDL_Texture* Font, int DestX, int DestY, int Flags, const char *fmt, ...);
int DrawTextTTF(TTF_Font* Font, int DestX, int DestY, int Flags, const char *fmt, ...);
void DrawTallInteger(int DestX, int DestY, int Flags, int Number);
void UpdatePlayfield(struct Playfield *P);
int GetTile(struct Playfield *P, int X, int Y);
int GetColor(struct Playfield *P, int X, int Y);
void SetTile(struct Playfield *P, int X, int Y, int Value);
void RandomizeRow(struct Playfield *P, int y);
void LogMessage(const char *fmt, ...);

int RandomTileColor(struct Playfield *P);
void UpdatePuzzleFrenzy(struct Playfield *P);
void UpdateAvalanche(struct Playfield *P);
void UpdatePillars(struct Playfield *P);
void UpdateCookie(struct Playfield *P);
void UpdateReversiBall(struct Playfield *P);
void UpdateDiceMatch(struct Playfield *P);
void UpdateStacker(struct Playfield *P);
int Random(int Choices);

void TriggerGarbageClear(struct Playfield *P, int x, int y, int *IsGarbage);
int CountConnected(struct Playfield *P, int X, int Y, int *Used);
void ClearConnected(struct Playfield *P, int X, int Y);
int MakeBlocksFall(struct Playfield *P);
int TestBlocksFall(struct Playfield *P);
int ClearAvalancheStyle(struct Playfield *P);
void UpdateKeys(struct Playfield *P);
int CombinedUpdateKeys(struct Playfield *P);
void UpdateKeysFromMap(struct JoypadMapping *Map, int *Out);
void UpdateVolumes();
void GetConfigPath();
void RemoveLineEndings(char *buffer);

struct GameModifier {
  const char *Name;
  const char *Description;
  int Flags;
  int Min;
  int Max;
  const char **Names;
};

enum ModifierFlags {
  MOD_REQUIRED,
  MOD_FRENZY
};

enum ModifierIDs {
  MOD_NULL,
  MOD_GAME_TYPE,
  MOD_GAME_DIFFICULTY,
  MOD_GAME_SPEED,
  MOD_EXPLODING_LIFT,
  MOD_COLOR_COUNT,
  MOD_MINIMUM_MATCH,
  MOD_INSTANT_SWAP,
  MOD_INSTANT_LIFT,
  MOD_TOUCH_CONTROL,
  MOD_MOUSE_CONTROL,
  MOD_PLAYFIELD_WIDTH,
  MOD_PLAYFIELD_HEIGHT,
};

extern struct GameModifier ModifierList[];
