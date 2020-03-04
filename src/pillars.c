/*
 * Net Puzzle Arena
 *
 * Copyright (C) 2017 NovaSquirrel
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

void UpdatePillars(struct Playfield *P) {
  int GoDown = 0;

  if(P->KeyNew[KEY_PAUSE])
    P->Paused ^= 1;
  if(P->Paused)
    return;

  if(P->SwapColor1 == BLOCK_EMPTY && P->SwapColor2 == BLOCK_EMPTY && P->SwapColor3 == BLOCK_EMPTY) {
    ClearMatchAnimation(P, 0);
    if(ClearPillarsStyle(P, 1))
      return;

    P->SwapColor1 = random_tile_color(P);
    P->SwapColor2 = random_tile_color(P);
    P->SwapColor3 = random_tile_color(P);
    P->CursorX = P->width/2;
    P->CursorY = 0;
    P->LockTimer = 0;
  }

  if(P->KeyNew[KEY_LEFT]) {
    P->CursorX -= P->CursorX != 0 && !GetTile1(P, -1, 0) && !GetTile1(P, -1, 1) && !GetTile1(P, -1, 2);
    P->LockTimer = 0;
  }

  if(P->KeyNew[KEY_RIGHT]) {
    P->CursorX += P->CursorX != P->width-1 && !GetTile1(P, 1, 0) && !GetTile1(P, 1, 1) && !GetTile1(P, 1, 2);
    P->LockTimer = 0;
  }

  if(P->KeyNew[KEY_ROTATE_L]) {
    int Temp = P->SwapColor1;
    P->SwapColor1 = P->SwapColor2;
    P->SwapColor2 = P->SwapColor3;
    P->SwapColor3 = Temp;
  }

  if(P->KeyNew[KEY_ROTATE_R]) {
    int Temp = P->SwapColor3;
    P->SwapColor3 = P->SwapColor2;
    P->SwapColor2 = P->SwapColor1;
    P->SwapColor1 = Temp;
  }

  if(P->KeyDown[KEY_DOWN] && !(retraces & 3))
    GoDown = 1;

  P->FallTimer++;
  if(P->FallTimer == 60) {
    P->FallTimer = 0;
    GoDown = 1;
  }

  if(GoDown) {
    if(!GetTile1(P, 0, 3) && P->CursorY != P->height-4)
      P->CursorY++;
  }

  if(P->CursorY >= (P->height-4) || GetTile1(P, 0, 3)) {
    P->LockTimer++;
    if(P->LockTimer > 16) {
      SetTile(P, P->CursorX, P->CursorY, P->SwapColor1);
      SetTile(P, P->CursorX, P->CursorY+1, P->SwapColor2);
      SetTile(P, P->CursorX, P->CursorY+2, P->SwapColor3);
      P->SwapColor1 = 0;
      P->SwapColor2 = 0;
      P->SwapColor3 = 0;
    }
  }

}
