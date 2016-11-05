#ifndef PUZZLE_HEADER
#define PUZZLE_HEADER
#define NO_STDIO_REDIRECT
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

extern int ScreenWidth, ScreenHeight;
extern SDL_Window *window;
extern SDL_Renderer *ScreenRenderer;
extern int retraces;

enum GameKey {
  KEY_LEFT,
  KEY_DOWN,
  KEY_UP,
  KEY_RIGHT,
  KEY_OK,
  KEY_BACK,
  KEY_PAUSE,
  KEY_SWAP,
  KEY_LIFT,
  KEY_COUNT
};

enum BlockColor {
  BLOCK_EMPTY,
  BLOCK_RED,
  BLOCK_GREEN,
  BLOCK_YELLOW,
  BLOCK_CYAN,
  BLOCK_PURPLE,
  BLOCK_BLUE,
  BLOCK_GRAY,
  BLOCK_DISABLED
};

struct MatchRow {
  int Color, X, Y, Width, Timer1, Timer2, DisplayX, DisplayWidth;
  struct MatchRow *Child, *Next;
};

struct Playfield {
  int Width, Height;
  int Rise;
  int CursorX, CursorY;
  int KeyDown[KEY_COUNT];
  int KeyLast[KEY_COUNT];
  int KeyNew[KEY_COUNT];
  int KeyRepeat[KEY_COUNT];
  int *Playfield;
  struct MatchRow *Match;
};

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
void UpdatePlayfield(struct Playfield *P);
int GetTile(struct Playfield *P, int X, int Y);
void SetTile(struct Playfield *P, int X, int Y, int Value);
