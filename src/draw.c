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

void DrawPlayfield(struct Playfield *P, int DrawX, int DrawY) {
  int VisualWidth = P->Width * TILE_W;
  int VisualHeight = (P->Height-1) * TILE_H;
  int Rise = P->Rise * (TILE_H / 16);

  // border
  SDL_SetRenderDrawColor(ScreenRenderer, 0, 0, 0, 255);
  SDL_Rect BorderRectangle = {DrawX-1, DrawY-1, VisualWidth + 2, VisualHeight + 2};
  SDL_RenderDrawRect(ScreenRenderer, &BorderRectangle);

  SDL_Rect ClipRectangle = {DrawX, DrawY, P->Width * TILE_W, (P->Height-1) * TILE_H};
  SDL_RenderSetClipRect(ScreenRenderer, &ClipRectangle);

  for(int y=0; y<VisualHeight; y++) {
    int Value = (float)y / VisualHeight * 255;
    SDL_SetRenderDrawColor(ScreenRenderer, Value, Value, Value, 255);
    SDL_RenderDrawLine(ScreenRenderer, DrawX, DrawY+y, DrawX+VisualWidth-1, DrawY+y);
  }

  // Draw tiles
  for(int x=0; x<P->Width; x++)
    for(int y=0; y<P->Height; y++) {
      int Tile = P->Playfield[P->Width * y + x];
      blit(TileSheet, ScreenRenderer, TILE_W*Tile, 0, DrawX+x*TILE_W, DrawY+y*TILE_H-Rise, TILE_W, TILE_H);
    }
  // Draw exploding blocks
  for(struct MatchRow *Heads = P->Match; Heads; Heads=Heads->Next) {
    int SourceY = Heads->Timer1?TILE_H:TILE_H*2;
    for(struct MatchRow *Match = Heads; Match; Match=Match->Child) {
      for(int i=0; i<Match->DisplayWidth; i++)
        blit(TileSheet, ScreenRenderer, TILE_W*Match->Color, SourceY, DrawX+(Match->DisplayX+i)*TILE_W, DrawY+Match->Y*TILE_H-Rise, TILE_W, TILE_H);
    }
  }

  // Draw swapping tiles
  if(P->SwapTimer) {
    int Offset = (4-P->SwapTimer)*4*ScaleFactor;
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor1, 0, DrawX+P->CursorX*TILE_W+Offset, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor2, 0, DrawX+(P->CursorX+1)*TILE_W-Offset, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
  }

  // Draw the cursor
  if(P->GameType == FRENZY) {
    blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+(P->CursorX+1)*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
  } else if(P->GameType == AVALANCHE) {
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor1, 0, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor2, 0, DrawX+(P->CursorX+DirX[P->Direction])*TILE_W, DrawY+(P->CursorY+DirY[P->Direction])*TILE_H-Rise, TILE_W, TILE_H);
  } else
    blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);

  SDL_RenderSetClipRect(ScreenRenderer, NULL);
}

void DrawText(SDL_Texture* Font, int DestX, int DestY, int Flags, const char *fmt, ...) {
  // seems to mess up when the scale is 3

  char Buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(Buffer, sizeof(Buffer), fmt, args);
  va_end(args);

  // render the text
  int FontW, FontH;
  SDL_QueryTexture(Font, NULL, NULL, &FontW, &FontH);
  int FontCharW = FontW/16, FontCharH = FontH/12;

  int BaseY = (Flags&TEXT_WHITE) ? FontH/2 : 0;

  if(Flags & TEXT_CENTERED)
    DestX -= strlen(Buffer)*FontCharW/2;

  for(const char *Text = Buffer; *Text; Text++) {
    char C = *Text - 0x20;
    blit(Font, ScreenRenderer, (C&15)*FontCharW, BaseY+(C>>4)*FontCharH, DestX, DestY, FontCharW, FontCharH);
    DestX += FontCharW;
  }
}

int DrawTextTTF(TTF_Font* Font, int DestX, int DestY, int Flags, const char *fmt, ...) {
  SDL_Color White = {  255,   255,   255, 255}; 
  SDL_Color Black = {    0,     0,     0, 255}; 

  char Buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(Buffer, sizeof(Buffer), fmt, args);
  va_end(args);

  if(Flags & TEXT_CENTERED) {
    int Width, Height;
    TTF_SizeText(Font, Buffer, &Width, &Height);
    DestX -= Width/2;
    DestY -= Height/2;
  }

  // render the text
  SDL_Surface *TextSurface2;
  if(Flags & TEXT_WRAPPED)
    TextSurface2 = TTF_RenderUTF8_Blended_Wrapped(Font, Buffer, (Flags&TEXT_WHITE)?Black:White, ScreenWidth);
  else
    TextSurface2 = TTF_RenderUTF8_Blended(Font, Buffer, (Flags&TEXT_WHITE)?Black:White);

  if(Flags & TEXT_FROM_BOTTOM)
    DestY -= TextSurface2->h;

  SDL_Surface *TextSurface = SDL_ConvertSurfaceFormat(TextSurface2, SDL_PIXELFORMAT_RGBA8888, 0);
  SDL_Texture *Texture;
  Texture = SDL_CreateTextureFromSurface(ScreenRenderer, TextSurface);
  blit(Texture, ScreenRenderer, 0, 0, DestX, DestY, TextSurface->w, TextSurface->h);
  SDL_FreeSurface(TextSurface);
  SDL_FreeSurface(TextSurface2);
  int MessageHeight;
  SDL_QueryTexture(Texture, NULL, NULL, NULL, &MessageHeight);
  SDL_DestroyTexture(Texture);

  return MessageHeight;
}
