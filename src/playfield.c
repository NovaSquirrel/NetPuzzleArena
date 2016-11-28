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

int PlayfieldWidth = 6;
int PlayfieldHeight = 13;

inline int GetColor(struct Playfield *P, int X, int Y) {
  return P->Playfield[P->Width * Y + X]&PF_COLOR;
}

inline int GetTile(struct Playfield *P, int X, int Y) {
  return P->Playfield[P->Width * Y + X];
}

inline void SetTile(struct Playfield *P, int X, int Y, int Value) {
  P->Playfield[P->Width * Y + X] = Value;
}

int RandomTileColor(struct Playfield *P) {
  return Random(P->ColorCount)+1;
}

int CountConnected(struct Playfield *P, int X, int Y, int *Used) {
  int Color = GetTile(P, X, Y);
  int Sum = 1;
  Used[P->Width * Y + X] = 1;

  if(X-1 >= 0 && !Used[P->Width * Y + (X-1)] && GetTile(P, X-1, Y) == Color)
    Sum += CountConnected(P, X-1, Y, Used);
  if(X+1 <= P->Width-1 && !Used[P->Width * Y + (X+1)] && GetTile(P, X+1, Y) == Color) 
    Sum += CountConnected(P, X+1, Y, Used);
  if(Y-1 >= 0 && !Used[P->Width * (Y-1) + X] && GetTile(P, X, Y-1) == Color) 
    Sum += CountConnected(P, X, Y-1, Used);
  if(Y+1 <= P->Height-2 && !Used[P->Width * (Y+1) + X] && GetTile(P, X, Y+1) == Color)
    Sum += CountConnected(P, X, Y+1, Used);

  return Sum;
}

void ClearConnected(struct Playfield *P, int X, int Y) {
  int Color = GetTile(P, X, Y);
  SetTile(P, X, Y, BLOCK_EMPTY);

  if(X-1 >= 0 && GetTile(P, X-1, Y) == Color)
    ClearConnected(P, X-1, Y);
  if(X+1 <= P->Width-1 && GetTile(P, X+1, Y) == Color) 
    ClearConnected(P, X+1, Y);
  if(Y-1 >= 0 && GetTile(P, X, Y-1) == Color) 
    ClearConnected(P, X, Y-1);
  if(Y+1 <= P->Height-2 && GetTile(P, X, Y+1) == Color)
    ClearConnected(P, X, Y+1);
}

int MakeBlocksFall(struct Playfield *P) {
  int BlocksFell = 0;
  for(int x=0; x<P->Width; x++)
    for(int y=P->Height-2; y; y--)
      if(!GetTile(P, x, y) && GetTile(P, x, y-1)) {
        SetTile(P, x, y, GetTile(P, x, y-1));
        SetTile(P, x, y-1, BLOCK_EMPTY);
        BlocksFell = 1;
      }
  return BlocksFell;
}

int TestBlocksFall(struct Playfield *P) {
  for(int x=0; x<P->Width; x++)
    for(int y=P->Height-2; y; y--)
      if(!GetTile(P, x, y) && GetTile(P, x, y-1)) {
        return 1;
      }
  return 0;
}

int ClearAvalancheStyle(struct Playfield *P) {
  if(MakeBlocksFall(P))
    return 1;

  // look for groups and clear them
  int Used[P->Width * P->Height];
  memset(Used, 0, sizeof(Used));    
  for(int y=0; y<P->Height; y++)
    for(int x=0; x<P->Width; x++) {
      if(!GetTile(P, x, y))
        continue;

      int Sum = CountConnected(P, x, y, Used);
      if(Sum >= P->MinMatchSize)
        ClearConnected(P, x, y);
    }

  if(TestBlocksFall(P))
    return 1;
  return 0;
}

void SetGameDefaults(struct Playfield *P, int Game) {
  P->GameType = Game;
  switch(Game) {
    case FRENZY:
      P->MinMatchSize = 3;
      P->ColorCount = 5;
      break;
    case AVALANCHE:
      P->MinMatchSize = 4;
      P->ColorCount = 5;
      break;
    case REVERSI_BALL:
      P->MinMatchSize = 10;
      P->ColorCount = 2;
      break;
    case PILLARS:
      P->MinMatchSize = 3;
      P->ColorCount = 6;
      break;
  }
}

void InitPlayfield(struct Playfield *P) {
  P->Width = 6;
  P->Height = 13;
  P->Playfield = calloc(P->Width * P->Height, sizeof(int));

  switch(P->GameType) {
    case FRENZY:
      for(int j=8; j<13; j++)
        RandomizeRow(P, j);
  }
}

void RandomizeRow(struct Playfield *P, int y) {
  // do not naturally create matches
  int Tries = 50;

  for(int x=0; x<P->Width; x++) {
    while(Tries-->0) { // give up if it can't make an acceptable row after too many tries
      int Color = RandomTileColor(P);
      if(x >= 2 && GetTile(P, x-1, y) == Color && GetTile(P, x-2, y) == Color)
        continue;
      if(y >= 2 && GetTile(P, x, y-1) == Color && GetTile(P, x, y-2) == Color)
        continue;
      SetTile(P, x, y, Color);
      break;
    }
  }
}

void UpdatePillars(struct Playfield *P) {

}

void UpdateDiceMatch(struct Playfield *P) {

}

void UpdateCookie(struct Playfield *P) {

}

void UpdateStacker(struct Playfield *P) {

}

void UpdatePlayfield(struct Playfield *P) {
  for(struct ComboNumber *Num = P->ComboNumbers; Num;) {
    struct ComboNumber *Next = Num->Next;

    Num->Timer--;
    if(!Num->Timer) {
      if(P->ComboNumbers == Num)
        P->ComboNumbers = Next;
      else {
        // find the previous one and make it point to the next
        struct ComboNumber *Prev = P->ComboNumbers;
        while(Prev->Next != Num)
          Prev = Prev->Next;
        Prev->Next = Next;
      }
      free(Num);
    }
    Num = Next;
  }

  switch(P->GameType) {
    case FRENZY:
      UpdatePuzzleFrenzy(P);
      break;
    case AVALANCHE:
      UpdateAvalanche(P);
      break;
    case PILLARS:
      UpdatePillars(P);
      break;
    case DICE_MATCH:
      UpdateDiceMatch(P);
      break;
    case REVERSI_BALL:
      UpdateReversiBall(P);
      break;
    case COOKIE:
      UpdateCookie(P);
      break;
    case STACKER:
      UpdateStacker(P);
      break;
  }
}
