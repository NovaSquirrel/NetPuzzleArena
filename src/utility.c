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

  if(ScaleFactor != 1) {
    // See if there is a better way to do this
    SDL_Surface *Surface2 = SDL_CreateRGBSurface(0, Surface->w*ScaleFactor, Surface->h*ScaleFactor, 32, 0, 0, 0, 0);
    SDL_FillRect(Surface2, NULL, SDL_MapRGB(Surface2->format, 255, 0, 255));
    SDL_SetColorKey(Surface2, SDL_TRUE, SDL_MapRGB(Surface2->format, 255, 0, 255));
    SDL_BlitScaled(Surface, NULL, Surface2, NULL);
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
  SDL_Rect Dst = {DestX, DestY};
  SDL_RenderCopy(DstBmp,  SrcBmp, NULL, &Dst);
}

