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

#ifdef FAST_MODE
  SDL_SetRenderDrawColor(ScreenRenderer, 8, 8, 8, 255);
  SDL_RenderFillRect(ScreenRenderer, NULL);
#else
  SDL_SetRenderDrawBlendMode(ScreenRenderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(ScreenRenderer, 8, 8, 8, 128);
  SDL_RenderFillRect(ScreenRenderer, NULL);
  SDL_SetRenderDrawBlendMode(ScreenRenderer, SDL_BLENDMODE_NONE);
#endif

  // Draw tiles
  for(int x=0; x<P->Width; x++)
    for(int y=0; y<P->Height-1; y++) {
      int Tile = P->Playfield[P->Width * y + x]&PF_COLOR;
      if(!Tile)
        continue;
      blit(TileSheet, ScreenRenderer, TILE_W*Tile, 0, DrawX+x*TILE_W, DrawY+y*TILE_H-Rise, TILE_W, TILE_H);
#ifdef DISPLAY_CHAIN_COUNT
      DrawText(GameFont, DrawX+x*TILE_W, DrawY+y*TILE_H-Rise, 0, "%i", (P->Playfield[P->Width * y + x]&PF_CHAIN)>>8);
#endif
    }
  // draw the bottom row
  for(int x=0; x<P->Width; x++) {
    int Tile = P->Playfield[P->Width * (P->Height-1) + x]&PF_COLOR;
    blit(TileSheet, ScreenRenderer, TILE_W*Tile, TILE_H*2, DrawX+x*TILE_W, DrawY+(P->Height-1)*TILE_H-Rise, TILE_W, TILE_H);
  }

  // Draw exploding blocks
  for(struct MatchRow *Heads = P->Match; Heads; Heads=Heads->Next) {
    int SourceY = Heads->Timer1?TILE_H:TILE_H*2;
    for(struct MatchRow *Match = Heads; Match; Match=Match->Child) {
#ifdef DISPLAY_CHAIN_COUNT
      DrawText(GameFont, DrawX+Match->X*TILE_W, DrawY+Match->Y*TILE_H-Rise, 0, "!%i", Match->Chain);
#endif
      for(int i=0; i<Match->DisplayWidth; i++)
        blit(TileSheet, ScreenRenderer, TILE_W*Match->Color, SourceY, DrawX+(Match->DisplayX+i)*TILE_W, DrawY+Match->Y*TILE_H-Rise, TILE_W, TILE_H);
    }
  }

  // Draw garbage slabs
  for(struct GarbageSlab *Slab = P->GarbageSlabs; Slab; Slab=Slab->Next) {
    if(!Slab->Clearing) {
      if(Slab->Height == 1) {
        blit(TileSheet, ScreenRenderer, TILE_W*13, TILE_H*3, DrawX+(Slab->X)*TILE_W,               DrawY+Slab->Y*TILE_H-Rise, TILE_W, TILE_H);
        for(int i=1; i<Slab->Width-1; i++)
          blit(TileSheet, ScreenRenderer, TILE_W*14, TILE_H*3, DrawX+(Slab->X+i)*TILE_W,           DrawY+Slab->Y*TILE_H-Rise, TILE_W, TILE_H);
        blit(TileSheet, ScreenRenderer, TILE_W*15, TILE_H*3, DrawX+(Slab->X+Slab->Width-1)*TILE_W, DrawY+Slab->Y*TILE_H-Rise, TILE_W, TILE_H);

      } else {
        // corners
        blit(TileSheet, ScreenRenderer, TILE_W*13, TILE_H*0, DrawX+(Slab->X)*TILE_W,               DrawY+Slab->Y*TILE_H-Rise, TILE_W, TILE_H);
        blit(TileSheet, ScreenRenderer, TILE_W*15, TILE_H*0, DrawX+(Slab->X+Slab->Width-1)*TILE_W, DrawY+Slab->Y*TILE_H-Rise, TILE_W, TILE_H);
        blit(TileSheet, ScreenRenderer, TILE_W*13, TILE_H*2, DrawX+(Slab->X)*TILE_W,               DrawY+(Slab->Y+Slab->Height-1)*TILE_H-Rise, TILE_W, TILE_H);
        blit(TileSheet, ScreenRenderer, TILE_W*15, TILE_H*2, DrawX+(Slab->X+Slab->Width-1)*TILE_W, DrawY+(Slab->Y+Slab->Height-1)*TILE_H-Rise, TILE_W, TILE_H);
        // top/bottom
        for(int i=1; i<Slab->Width-1; i++) {
          blit(TileSheet, ScreenRenderer, TILE_W*14, TILE_H*0, DrawX+(Slab->X+i)*TILE_W,             DrawY+Slab->Y*TILE_H-Rise, TILE_W, TILE_H);
          blit(TileSheet, ScreenRenderer, TILE_W*14, TILE_H*2, DrawX+(Slab->X+i)*TILE_W,             DrawY+(Slab->Y+Slab->Height-1)*TILE_H-Rise, TILE_W, TILE_H);
        }
        // left/right
        for(int i=1; i<Slab->Height-1; i++) {
          blit(TileSheet, ScreenRenderer, TILE_W*13, TILE_H*1, DrawX+(Slab->X)*TILE_W,                 DrawY+(Slab->Y+i)*TILE_H-Rise, TILE_W, TILE_H);
          blit(TileSheet, ScreenRenderer, TILE_W*15, TILE_H*1, DrawX+(Slab->X+Slab->Width-1)*TILE_W,   DrawY+(Slab->Y+i)*TILE_H-Rise, TILE_W, TILE_H);
        }
        // inside
        for(int i=1; i<Slab->Width-1; i++)
          for(int j=1; j<Slab->Height-1; j++)
            blit(TileSheet, ScreenRenderer, TILE_W*14, TILE_H*1, DrawX+(Slab->X+i)*TILE_W,   DrawY+(Slab->Y+j)*TILE_H-Rise, TILE_W, TILE_H);
      }
    } else {
      for(int i=0; i<Slab->Width; i++)
        for(int j=0; j<Slab->Height; j++)
          blit(TileSheet, ScreenRenderer, TILE_W*10, TILE_H*1, DrawX+(Slab->X+i)*TILE_W,   DrawY+(Slab->Y+j)*TILE_H-Rise, TILE_W, TILE_H);
    }
  }

  // Draw swapping tiles
  if(P->SwapTimer) {
    int Offset = (4-P->SwapTimer)*4*ScaleFactor;
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor1, 0, DrawX+P->SwapX*TILE_W+Offset, DrawY+P->SwapY*TILE_H-Rise, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor2, 0, DrawX+(P->SwapX+1)*TILE_W-Offset, DrawY+P->SwapY*TILE_H-Rise, TILE_W, TILE_H);
  }

  // Draw the cursor
  if(P->GameType == FRENZY || P->GameType == FRENZY_CT) {
    blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+(P->CursorX+1)*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
  } else if(P->GameType == AVALANCHE) {
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor1, 0, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor2, 0, DrawX+(P->CursorX+DirX[P->Direction])*TILE_W, DrawY+(P->CursorY+DirY[P->Direction])*TILE_H-Rise, TILE_W, TILE_H);
  } else if(P->GameType == PILLARS) {
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor1, 0, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor2, 0, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise+TILE_H, TILE_W, TILE_H);
    blit(TileSheet, ScreenRenderer, TILE_W*P->SwapColor3, 0, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise+TILE_H*2, TILE_W, TILE_H);
  } else
    blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-Rise, TILE_W, TILE_H);

  // Draw floating numbers
  for(struct ComboNumber *Num = P->ComboNumbers; Num; Num=Num->Next)
    DrawTallInteger(DrawX+Num->X, DrawY+Num->Y-Rise, Num->Flags, Num->Number);

  SDL_RenderSetClipRect(ScreenRenderer, NULL);

  // Draw "paused"
  if(P->Paused) {
    DrawText(GameFont, ScreenWidth/2, ScreenHeight-16*ScaleFactor, TEXT_CENTERED, "Paused");
  }
}

void DrawText(SDL_Texture* Font, int DestX, int DestY, int Flags, const char *fmt, ...) {
  // seems to mess up when the scale is 3

  char Buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(Buffer, sizeof(Buffer), fmt, args);
  va_end(args);
  if(!*Buffer)
    return;

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
  if(!*Buffer)
    return 0;

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

void DrawTallInteger(int DestX, int DestY, int Flags, int Number) {
  const int CharWidth = 8*ScaleFactor;
  const int CharHeight = 16*ScaleFactor;
  char Buffer[20];
  sprintf(Buffer, "%i", Number);
  int Length = strlen(Buffer) + (0 != (Flags&TEXT_CHAIN));

  if(Flags&TEXT_CENTERED) {
    DestX -= Length*CharWidth/2;
    DestY -= CharHeight/2;
  }

  if(Flags&TEXT_CHAIN) {
    blit(TileSheet, ScreenRenderer, 10*CharWidth, TILE_H*5, DestX, DestY, CharWidth, CharHeight);
    DestX += CharWidth;
  }

  for(char *Pointer = Buffer; *Pointer; Pointer++) {
    blit(TileSheet, ScreenRenderer, (*Pointer-'0')*CharWidth, TILE_H*5, DestX, DestY, CharWidth, CharHeight);
    DestX += CharWidth;
  }
}
