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

#define HOVER_TIME 12

int BasePointsForCombo(int Size) {
  // https://www.gamefaqs.com/n64/913924-pokemon-puzzle-league/faqs/16679
  const static int Table[] = {  30,    50,   150,   190,    230,   270,   310,   400,
                               450,   500,   550,   700,    760,   850,   970,  1120,
                              1300,  1510,  1750,  2020,   2320,  2650,  3010,  3400,
                              3820,  4270,  4750,  5260,  15000, 15570, 16170, 16800,
                             17460, 18150, 18870, 19620,  20400};
  if(Size < 4)
    return 0;
  if(Size >= 4 && Size <= 40)
    return Table[Size-4];
  return 20400 + ((Size - 40) * 800);
}

int PointsForChainPart(int Size) {
// Size is the chain number from the original game, minus 1
// so if you clear blocks and cause a chain, size is 1
  const static int Table[] = {50, 80, 150, 300, 400, 500, 700, 900, 1100, 1300, 1500, 1800};
  if(Size <= 1)
    return 0;
  if(Size <= 12)
    return Table[Size-1];
  return 6980 + ((Size+1 - 12) * 1800);
}

void UpdatePuzzleFrenzy(struct Playfield *P) {
  int IsFalling[P->Width][P->Height];
  int MaxActiveChain = 0;
  memset(IsFalling, 0, sizeof(IsFalling));
  for(struct FallingChunk *Fall = P->FallingData; Fall; Fall = Fall->Next)
    for(int h=0; h<Fall->Height; h++) {
      IsFalling[Fall->X][Fall->Y+h] = 1+!Fall->Timer;
      int Tile = GetTile(P, Fall->X, Fall->Y+h);
      if(MaxActiveChain < ((Tile&PF_CHAIN)>>8))
        MaxActiveChain = (Tile&PF_CHAIN)>>8;
    }
  // Also look at clearing blocks for chain counts
  for(struct MatchRow *Match = P->Match; Match; Match=Match->Next) {
    if(MaxActiveChain < (Match->Chain>>8))
      MaxActiveChain = (Match->Chain>>8);
  }

  // Cursor movement
  if(P->SwapTimer) {
    P->SwapTimer--;
    if(!P->SwapTimer) {
      SetTile(P, P->CursorX, P->CursorY, P->SwapColor2);
      SetTile(P, P->CursorX+1, P->CursorY, P->SwapColor1);
    }
  }

  if(!P->SwapTimer) {
    int OldX = P->CursorX; //, OldY = P->CursorY;

    if(P->KeyNew[KEY_LEFT])
      P->CursorX -= (P->CursorX != 0);
    if(P->KeyNew[KEY_DOWN])
      P->CursorY += (P->CursorY != P->Height-2);
    if(P->KeyNew[KEY_UP])
      P->CursorY -= (P->CursorY != 1);
    if(P->KeyNew[KEY_RIGHT])
      P->CursorX += (P->CursorX != P->Width-2);

    if(P->Flags & PULL_BLOCK_HORIZONTAL && OldX != P->CursorX && P->KeyDown[KEY_SWAP])
      P->KeyNew[KEY_SWAP] = 1;
//    if((OldX != P->CursorX || OldY != P->CursorY) && !UsedAutorepeat)
//      Mix_PlayChannel(-1, SampleMove, 0);

    if(P->KeyNew[KEY_SWAP]) {
      int Tile1 = GetTile(P, P->CursorX, P->CursorY);
      int Tile2 = GetTile(P, P->CursorX+1, P->CursorY);
      if(Tile1 != BLOCK_DISABLED && Tile2 != BLOCK_DISABLED
        // to do: you CAN catch a tile as it's falling.
        // this should probably split the falling column into two parts
        && !IsFalling[P->CursorX][P->CursorY] && !IsFalling[P->CursorX+1][P->CursorY]
//        && !IsFalling[P->CursorX][P->CursorY-1] && !IsFalling[P->CursorX+1][P->CursorY-1]
        ) {
        P->SwapColor1 = Tile1 & PF_COLOR; // strip out chain information for now
        P->SwapColor2 = Tile2 & PF_COLOR;
        if(!(P->Flags & SWAP_INSTANTLY)) {
          SetTile(P, P->CursorX, P->CursorY, BLOCK_DISABLED);
          SetTile(P, P->CursorX+1, P->CursorY, BLOCK_DISABLED);
          P->SwapTimer = 3;
#ifdef ENABLE_AUDIO
          Mix_PlayChannel(-1, SampleSwap, 0);
#endif
        } else {
          SetTile(P, P->CursorX, P->CursorY, Tile2);
          SetTile(P, P->CursorX+1, P->CursorY, Tile1);
#ifdef ENABLE_AUDIO
          Mix_PlayChannel(-1, SampleSwap, 0);
#endif
        }
      }
    }
  }

  if(P->KeyNew[KEY_PAUSE])
    P->Paused ^= 1;
  if(P->Paused) {
    // allow editing the playfield for debugging stuff
    if(P->KeyNew[KEY_OK])
      SetTile(P, P->CursorX, P->CursorY, (GetTile(P, P->CursorX, P->CursorY)+1)%BLOCK_BLUE);
    if(P->KeyNew[KEY_BACK]) {
      SetTile(P, P->CursorX, P->CursorY, BLOCK_EMPTY);
      SetTile(P, P->CursorX+1, P->CursorY, BLOCK_EMPTY);
    }
    return;
  }

/////////////////// MATCHES ///////////////////

  // Look for matches
  struct MatchRow *FirstMatch = NULL, *CurMatch = NULL;
  int Used[P->Width][P->Height];  // horizontal
  int UsedV[P->Width][P->Height]; // vertical
  int ChainMap[P->Width][P->Height];
  memset(Used, 0, sizeof(Used));
  memset(UsedV, 0, sizeof(UsedV));
  memset(ChainMap, 0, sizeof(ChainMap));
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      int Horiz = 0, Vert = 0, Color = GetColor(P, x, y);
      if(!Color || Color==BLOCK_DISABLED || IsFalling[x][y])
        continue;
      if(y < P->Height-2 && !GetTile(P, x, y+1))
        continue;

      int MaxChain = 0;

      if(!Used[x][y])
        while((x+Horiz+1 < P->Width && GetColor(P, x+Horiz+1, y) == Color) &&
              !IsFalling[x+Horiz+1][y] && (y!=P->Height-2 || GetColor(P, x+Horiz+1, y+1)))
          Horiz++;
      if(!UsedV[x][y])
        while(y+Vert+1 < P->Height-1 && !UsedV[x][y+Vert+1] && GetColor(P, x, y+Vert+1) == Color)
          Vert++;

      // mark tiles vertically that were used in the combo, also look for max chain
      if(Vert >= P->MinMatchSize-1) {
        for(int i=0; i<=Vert; i++) {
          UsedV[x][y+i] = 1;
          int Chain = GetTile(P, x, y+i)&PF_CHAIN;
          if(Chain > MaxChain)
            MaxChain = Chain;
        }
      }
      // look at chain size first
      for(int i=0; i<=Horiz; i++) {
        int Chain = GetTile(P, x+i, y)&PF_CHAIN;
        if(Chain > MaxChain)
          MaxChain = Chain;
      }

      if(Vert >= P->MinMatchSize-1 || Horiz >= P->MinMatchSize-1)
        if(MaxChain) {
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Chain %i", MaxChain>>8);
          P->Score += PointsForChainPart(MaxChain>>8);
        }

      if(Horiz >= P->MinMatchSize-1) {
        for(int i=0; i<=Horiz; i++)
          Used[x+i][y] = 1;
        Used[x][y] = Horiz+1; // write the width
      }
    }

  // create match structs for the matches that are found
  int ComboSize = 0, ComboChainSize = 0;
  // look for chains first
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      int Chain = GetTile(P, x, y)&PF_CHAIN;
      if(Chain > ComboChainSize)
        ComboChainSize = Chain;
    }

  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      if(Used[x][y] || UsedV[x][y]) {
        struct MatchRow *Match = (struct MatchRow*)malloc(sizeof(struct MatchRow));
        int Width = Used[x][y];
        if(!Width)
          Width = 1;
        ComboSize += Width;
        Match->Color = GetColor(P, x, y);
        Match->X = x;
        Match->Y = y;
        Match->DisplayX = x;
        Match->DisplayWidth = Width;
        Match->Width = Width;
        Match->Child = NULL;
        Match->Next = NULL;
        Match->Timer1 = 0;
        Match->Chain = ComboChainSize;
        if(!FirstMatch)
          Match->Timer2 = 26;
        else
          Match->Timer2 = 10;

        for(int i=0; i<Width; i++)
          SetTile(P, x+i, y, BLOCK_DISABLED);
        if(!FirstMatch)
          FirstMatch = Match;
        if(CurMatch)
          CurMatch->Child = Match;
        CurMatch = Match;

        x+=Width-1;
      }
    }
  if(ComboSize) {
    P->Score += BasePointsForCombo(ComboSize);
    // chain bonus
    if(MaxActiveChain)
      P->Score += PointsForChainPart(MaxActiveChain-1);
  }
  if(ComboSize >= 4 || (ComboSize == 3 && ComboChainSize))
    Mix_PlayChannel(-1, SampleCombo, 0);

//    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Combo of size %i", ComboSize);

  if(FirstMatch) {
    if(!P->Match)
      P->Match = FirstMatch;
    else {
      FirstMatch->Next = P->Match;
      P->Match = FirstMatch;
    }
    FirstMatch->Timer1 = 46;
  }
  
  // Do animation for clearing blocks
  for(struct MatchRow *Match = P->Match; Match; Match=Match->Next) {
    // Stay white for a moment
    if(Match->Timer1) {
      Match->Timer1--;
      continue;
    }

    // Start erasing blocks
    struct MatchRow *Last = Match;
    while(!Last->DisplayWidth)
      Last = Last->Child;

    Last->Timer2--;
    if(!Last->Timer2) {
      Last->Timer2 = 10;
      Last->DisplayX++;
      Last->DisplayWidth--;
      P->Score += 10;
#ifdef ENABLE_AUDIO
      Mix_PlayChannel(-1, SampleDisappear, 0);
#endif

      // did the last one finish clearing out?
      if(!Last->DisplayWidth && !Last->Child) {

        // adjust pointers
        if(P->Match == Match) {
          P->Match = Match->Next;
        } else {
          struct MatchRow *Find = P->Match;
          while(Find->Next != Match)
            Find = Find->Next;
          Find->Next = Match->Next;
        }

        // free, and also erase all those blocks
        for(Last = Match; Last;) {
          // mark chain counters
         for(int x=Last->X; x<(Last->X+Last->Width); x++) {
            for(int y=Last->Y; GetColor(P, x, y) && y; y--) {
              int Chain = GetTile(P, x, y) & PF_CHAIN;
              if(Chain < Last->Chain+PF_CHAIN_ONE)
                SetTile(P, x, y, GetColor(P, x, y) | (Last->Chain+PF_CHAIN_ONE));
            }
          }

          struct MatchRow *Next = Last->Child;
          free(Last);
          Last = Next;
        }
      }
    }
  }

  // Change disabled blocks back to regular ones
  memset(Used, 0, sizeof(Used));
  for(struct MatchRow *Heads = P->Match; Heads; Heads=Heads->Next)
    for(struct MatchRow *Match = Heads; Match; Match=Match->Child)
      for(int i=0; i<Match->Width; i++)
        Used[Match->X+i][Match->Y] = 1;
  for(int x=0; x<P->Width; x++)
    for(int y=0; y<P->Height; y++)
      if(GetColor(P, x, y) == BLOCK_DISABLED && !Used[x][y]
      && (!P->SwapTimer || (y != P->CursorY && (x != P->CursorX && x != P->CursorX+1))))
        SetTile(P, x, y, BLOCK_EMPTY);

/////////////////// GRAVITY ///////////////////

  // gravity
  for(int x=0; x< P->Width; x++) {
    for(int y=0; y<P->Height-1; y++) {
      int Color = GetColor(P, x, y);
      if(!Color || Color == BLOCK_DISABLED || IsFalling[x][y])
        continue;

      // find the bottom of this stack
      int Bottom = y+1;
      while(Bottom < P->Height-1) {
        int Color2 = GetColor(P, x, Bottom);
        if(!Color2 || Color2 == BLOCK_DISABLED || IsFalling[x][Bottom])
          break;
        Bottom++;
      }

      if(Bottom == P->Height-1)
        break;
      if(GetColor(P, x, Bottom) != BLOCK_DISABLED) {
        struct FallingChunk *Fall = (struct FallingChunk*)malloc(sizeof(struct FallingChunk));
        Fall->X = x;
        Fall->Y = y;
        Fall->Height = Bottom-y;
        Fall->Timer = HOVER_TIME;
        Fall->Next = NULL;

        // add to the list of falling chunks
        if(!P->FallingData)
          P->FallingData = Fall;
        else {
          Fall->Next = P->FallingData;
          P->FallingData = Fall;
        }
      }

      y = Bottom-1;
    }
  }

  // remove Just Landed flag
  for(int x=0; x<P->Width; x++)
    for(int y=0; y<P->Height-1; y++) {
      int Tile = GetTile(P, x, y);
      if(Tile & PF_JUST_LANDED)
        SetTile(P, x, y, Tile & PF_COLOR);
    }

  // move falling things down
  int PlayDropSound = 0;
  for(struct FallingChunk *F = P->FallingData; F; ) {
    int Free = 0;
    struct FallingChunk *Next = F->Next;

    if(F->Timer) {
      F->Timer--;
      F = Next;
      continue;
    }

    // if you're at the bottom, stop falling
    int ColorAtBottom = GetTile(P, F->X, F->Y+F->Height);

    if(F->Y + F->Height >= P->Height-1) {
      PlayDropSound = 1;
      Free = 1;
    } else if(ColorAtBottom == BLOCK_EMPTY){
      for(int y = F->Y+F->Height; y > F->Y; y--)
        SetTile(P, F->X, y, GetTile(P, F->X, y-1));
      SetTile(P, F->X, F->Y, BLOCK_EMPTY);
      F->Y++;
    } else if(ColorAtBottom == BLOCK_DISABLED) {
      F->Timer = HOVER_TIME;
      F = Next;
      continue;
    } else if(ColorAtBottom && !IsFalling[F->X][F->Y + F->Height]) {
      PlayDropSound = 1;
      Free = 1;
    }

    // free up the falling chunk and update pointers
    if(Free) {
      if(P->FallingData == F)
        P->FallingData = F->Next;
      else {
        // find the previous one and make it point to the next
        struct FallingChunk *Prev = P->FallingData;
        while(Prev->Next != F)
          Prev = Prev->Next;
        Prev->Next = F->Next;
      }

      for(int y=F->Y; y < (F->Y+F->Height); y++)
        SetTile(P, F->X, y, GetTile(P, F->X, y) | PF_JUST_LANDED);
      free(F);
    }

    F = Next;
  }
#ifdef ENABLE_AUDIO
  if(PlayDropSound)
    Mix_PlayChannel(-1, SampleDrop, 0);
#endif

/////////////////// RISING ///////////////////

  // Handle rising
  if(!P->Match && !(retraces & 15))
    P->Rise++;
  if((!P->Match || P->Flags&LIFT_WHILE_CLEARING) && P->KeyNew[KEY_LIFT]) {
    P->Rise = 16;
    P->Score++;
  }
  if(P->Rise >= 16) {
    // push playfield up
    for(int y=0; y<P->Height-1; y++)
      for(int x=0; x<P->Width; x++)
        SetTile(P, x, y, GetTile(P, x, y+1));
    P->CursorY--;
    P->Rise = 0;

    // generate new blocks
    RandomizeRow(P, P->Height-1);
    // also update falling data
    for(struct FallingChunk *Fall = P->FallingData; Fall; Fall = Fall->Next)
      Fall->Y--;

    // move exploding blocks up
    for(struct MatchRow *Heads = P->Match; Heads; Heads=Heads->Next)
      for(struct MatchRow *Match = Heads; Match; Match=Match->Child)
        Match->Y--;
  }

  if(!P->CursorY)
   P->CursorY = 1;
}
