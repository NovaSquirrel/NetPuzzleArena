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
#include "puzzle.h"
#include <math.h>

int DirX[] = {1, 1, 0, -1, -1, -1, 0, 1};
int DirY[] = {0, 1, 1, 1, 0, -1, -1, -1};

void SDL_MessageBox(int Type, const char *Title, SDL_Window *Window, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  char Buffer[512];
  vsprintf(Buffer, fmt, argp);
  SDL_ShowSimpleMessageBox(Type, Title, Buffer, Window);
  va_end(argp);
}

int Random(int Choices) {
  int Out, Mask;
  if(Choices == 2)
    return rand()&1;
  else if(Choices <= 4)
    Mask = 3;
  else if(Choices <= 8)
    Mask = 7;
  else if(Choices <= 16)
    Mask = 15;
  else if(Choices <= 32)
    Mask = 31;
  else if(Choices <= 64)
    Mask = 63;
  else
    return rand()%Choices;

  while(1) {
    Out = rand()&Mask;
    if(Out < Choices)
      return Out;
  }
}

void strlcpy(char *Destination, const char *Source, int MaxLength) {
  // MaxLength is directly from sizeof() so it includes the zero
  int SourceLen = strlen(Source);
  if((SourceLen+1) < MaxLength)
    MaxLength = SourceLen + 1;
  memcpy(Destination, Source, MaxLength-1);
  Destination[MaxLength-1] = 0;
}

SDL_Surface *SDL_LoadImage(const char *FileName, int Flags) {
  SDL_Surface* loadedSurface = IMG_Load(FileName);
  if(loadedSurface == NULL) {
    SDL_MessageBox(SDL_MESSAGEBOX_ERROR, "Error", window, "Unable to load image %s! SDL Error: %s", FileName, SDL_GetError());
    return NULL;
  }
  if(Flags & 1)
    SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 255, 0, 255));
  return loadedSurface;
}

SDL_Texture *LoadTexture(const char *FileName, int Flags) {
  SDL_Surface *Surface = SDL_LoadImage(FileName, Flags);
  if(!Surface) return NULL;

  // Do integer scaling if needed
  if(ScaleFactor != 1) {
    // See if there is a better way to do this
    SDL_Surface *Surface2 = SDL_CreateRGBSurface(0, Surface->w*ScaleFactor, Surface->h*ScaleFactor,
      Surface->format->BitsPerPixel, Surface->format->Rmask, Surface->format->Gmask, Surface->format->Bmask, Surface->format->Amask);
    SDL_FillRect(Surface2, NULL, SDL_MapRGBA(Surface2->format, 0, 0, 0, 0));

    // Yes I know this is ridiculous but accessing the pixels directly seems very error prone
    for(int w=0; w<Surface->w; w++)
      for(int h=0; h<Surface->h; h++) {
        SDL_Rect Rect1 = {w, h, 1, 1};
        for(int w2=0; w2<ScaleFactor; w2++)
          for(int h2=0; h2<ScaleFactor; h2++) {
            SDL_Rect Rect2 = {w*ScaleFactor+w2, h*ScaleFactor+h2, 1, 1};
            SDL_LowerBlit(Surface, &Rect1, Surface2, &Rect2);
          }
      }

    SDL_FreeSurface(Surface);
    Surface = Surface2;
  }

  SDL_Texture *Texture = SDL_CreateTextureFromSurface(ScreenRenderer, Surface);
  SDL_FreeSurface(Surface);
  return Texture;
}

// drawing functions
void rectfill(SDL_Renderer *Bmp, int X1, int Y1, int X2, int Y2) {
  SDL_Rect Temp = {X1, Y1, abs(X2-X1)+1, abs(Y2-Y1)+1};
  SDL_RenderFillRect(Bmp,  &Temp);
}

void rect(SDL_Renderer *Bmp, int X1, int Y1, int X2, int Y2) {
  SDL_Rect Temp = {X1, Y1, abs(X2-X1)+1, abs(Y2-Y1)+1};
  SDL_RenderDrawRect(Bmp, &Temp);
}

void sblit(SDL_Surface* SrcBmp, SDL_Surface* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height) {
  SDL_Rect Src = {SourceX, SourceY, Width, Height};
  SDL_Rect Dst = {DestX, DestY};
  SDL_BlitSurface(SrcBmp, &Src, DstBmp, &Dst);
}

void blit(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height) {
  SDL_Rect Src = {SourceX, SourceY, Width, Height};
  SDL_Rect Dst = {DestX, DestY, Width, Height};
  SDL_RenderCopy(DstBmp,  SrcBmp, &Src, &Dst);
}

void blitf(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height, SDL_RendererFlip Flip) {
  SDL_Rect Src = {SourceX, SourceY, Width, Height};
  SDL_Rect Dst = {DestX, DestY, Width, Height};
  SDL_RenderCopyEx(DstBmp,  SrcBmp, &Src, &Dst, 0, NULL, Flip);
}

void blitz(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int SourceX, int SourceY, int DestX, int DestY, int Width, int Height, int Width2, int Height2) {
  SDL_Rect Src = {SourceX, SourceY, Width, Height};
  SDL_Rect Dst = {DestX, DestY, Width2, Height2};
  SDL_RenderCopy(DstBmp,  SrcBmp, &Src, &Dst);
}

void blitfull(SDL_Texture* SrcBmp, SDL_Renderer* DstBmp, int DestX, int DestY) {
  int Width, Height;
  SDL_QueryTexture(SrcBmp, NULL, NULL, &Width, &Height);
  SDL_Rect Dst = {DestX, DestY, Width, Height};
  SDL_RenderCopy(DstBmp,  SrcBmp, NULL, &Dst);
}

void UpdateKeys(struct Playfield *P) {
  const Uint8 *Keyboard = SDL_GetKeyboardState(NULL);
  P->KeyDown[KEY_LEFT] = Keyboard[SDL_SCANCODE_LEFT];
  P->KeyDown[KEY_DOWN] = Keyboard[SDL_SCANCODE_DOWN];
  P->KeyDown[KEY_UP] = Keyboard[SDL_SCANCODE_UP];
  P->KeyDown[KEY_RIGHT] = Keyboard[SDL_SCANCODE_RIGHT];
  P->KeyDown[KEY_OK] = Keyboard[SDL_SCANCODE_RETURN];
  P->KeyDown[KEY_BACK] = Keyboard[SDL_SCANCODE_BACKSPACE];
  P->KeyDown[KEY_SWAP] = Keyboard[SDL_SCANCODE_SPACE];
  P->KeyDown[KEY_LIFT] = Keyboard[SDL_SCANCODE_Z];
  P->KeyDown[KEY_PAUSE] = Keyboard[SDL_SCANCODE_P];
  P->KeyDown[KEY_ROTATE_L] = Keyboard[SDL_SCANCODE_X];
  P->KeyDown[KEY_ROTATE_R] = Keyboard[SDL_SCANCODE_C];
  if(Keyboard[SDL_SCANCODE_ESCAPE])
    quit = 1;

  // Update keys, do key repeat
  for(int i=0; i<KEY_COUNT; i++) {
    P->KeyNew[i] = P->KeyDown[i] && !P->KeyLast[i];

    if(i < KEY_OK && !(P->Flags&NO_AUTO_REPEAT)) {
      if(P->KeyDown[i])
        P->KeyRepeat[i]++;
      else
        P->KeyRepeat[i] = 0;

      if(P->KeyRepeat[i] > 8)
        P->KeyNew[i] = 1;
    }
  }
  memcpy(P->KeyLast, P->KeyDown, sizeof(P->KeyDown));
}
