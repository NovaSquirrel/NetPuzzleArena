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
extern int FrameAdvance;
extern int FrameAdvanceMode;

// Takes a combo size (in number of tiles) and figures out how much garbage it amounts to
int GarbageForCombo(struct Playfield *P, int ComboSize, int *List, int ListSize) {
  int Width = P->Width;
  ComboSize--; // garbage tiles is combo tiles minus 1

  int Count = 0;
  // Keep generating garbage slabs until enough have been made
  while(ComboSize > 0 && Count <=ListSize) {
    int PieceSize;

    // If the next one is small enough then just go for it
    if(ComboSize <= Width)
      PieceSize = ComboSize;
    // if it's not, make sure the one after this is big enough
    else {
      PieceSize = Width;
      if((ComboSize - PieceSize) < 3)
        PieceSize -= 3-(ComboSize-PieceSize);
    }
    ComboSize -= PieceSize;

    List[Count++] = PieceSize;
  }
  return Count;
}

// Calculates the number of points to award for a combo of a given size
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

// Calculates the number of points to award for a part of a chain
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

// Run the playfield for one tick
void UpdatePuzzleFrenzy(struct Playfield *P) {
  int IsFalling[P->Width][P->Height];
  int IsGarbage[P->Width * P->Height];
  int MaxActiveChain = 0;
  int IsChainActive = 0;

  // Mark the tiles that are currently falling for easy checking
  memset(IsFalling, 0, sizeof(IsFalling));
  for(struct FallingChunk *Fall = P->FallingData; Fall; Fall = Fall->Next)
    for(int h=0; h<Fall->Height; h++)
      IsFalling[Fall->X][Fall->Y+h] = 1+!Fall->Timer;

  // Mark the garbage tiles for easy checking
  memset(IsGarbage, 0, sizeof(IsGarbage));
  for(struct GarbageSlab *Slab = P->GarbageSlabs; Slab; Slab = Slab->Next)
    for(int w=0; w<Slab->Width; w++)
      for(int h=0; h<Slab->Height; h++)
        IsGarbage[(P->Width*Slab->Y+h) + Slab->X+w] = 1;

  // Also look at clearing blocks for chain counts
  for(struct MatchRow *Match = P->Match; Match; Match=Match->Next) {
    if(Match->Chain)
      IsChainActive = 1;
    if(MaxActiveChain < (Match->Chain>>8))
      MaxActiveChain = (Match->Chain>>8);
  }
  // Apparently I need to look at the playfield for chain counts too
  for(int x=0; x<P->Width; x++)
    for(int y=0; y<P->Height-2; y++) {
      int Chain = GetTile(P, x, y)&PF_CHAIN;
      if(Chain)
        IsChainActive = 1;
    }

  // Cursor movement
  if(!FrameAdvanceMode || !P->Paused || FrameAdvance) {
    if(P->SwapTimer) {
      P->SwapTimer--;
      if(!P->SwapTimer) {
        SetTile(P, P->CursorX, P->CursorY, P->SwapColor2);
        SetTile(P, P->CursorX+1, P->CursorY, P->SwapColor1);
      }
    }
  }

  // If the player isn't currently swapping, allow them to move
  if(!P->SwapTimer) {
    if(P->KeyNew[KEY_LEFT])
      P->CursorX -= (P->CursorX != 0);
    if(P->KeyNew[KEY_DOWN])
      P->CursorY += (P->CursorY != P->Height-2);
    if(P->KeyNew[KEY_UP])
      P->CursorY -= (P->CursorY != 1);
    if(P->KeyNew[KEY_RIGHT])
      P->CursorX += (P->CursorX != P->Width-2);

//    if(P->Flags & PULL_BLOCK_HORIZONTAL && OldX != P->CursorX && P->KeyDown[KEY_SWAP])
//      P->KeyNew[KEY_SWAP] = 1;
//    if((OldX != P->CursorX || OldY != P->CursorY) && !UsedAutorepeat)
//      Mix_PlayChannel(-1, SampleMove, 0);

    // Attempt a swap if either rotate key is pressed
    if(P->KeyNew[KEY_ROTATE_L] || P->KeyNew[KEY_ROTATE_R]) {
      int Tile1 = GetTile(P, P->CursorX, P->CursorY);
      int Tile2 = GetTile(P, P->CursorX+1, P->CursorY);
      if(Tile1 != BLOCK_DISABLED && Tile2 != BLOCK_DISABLED
        // to do: you CAN catch a tile as it's falling.
        // this should probably split the falling column into two parts
        && !IsFalling[P->CursorX][P->CursorY] && !IsFalling[P->CursorX+1][P->CursorY]
//        && !IsFalling[P->CursorX][P->CursorY-1] && !IsFalling[P->CursorX+1][P->CursorY-1]
        ) {
        P->SwapColor1 = Tile1; // yes, keep the chain count in the tiles!
        P->SwapColor2 = Tile2; // see https://youtu.be/m1sNm62gCR0?t=1m48s

        if(!(P->Flags & SWAP_INSTANTLY)) {
          // regular swap, takes 4 frames
          SetTile(P, P->CursorX, P->CursorY, BLOCK_DISABLED);
          SetTile(P, P->CursorX+1, P->CursorY, BLOCK_DISABLED);
          P->SwapTimer = 3;
        } else {
          // instantly swap, no timer
          SetTile(P, P->CursorX, P->CursorY, Tile2);
          SetTile(P, P->CursorX+1, P->CursorY, Tile1);
        }
#ifdef ENABLE_AUDIO
        Mix_PlayChannel(-1, SampleSwap, 0);
#endif
      }
    }
  }

  if(P->KeyNew[KEY_PAUSE])
    P->Paused ^= 1;
  if(!FrameAdvance && P->Paused) {
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
      // Don't trigger any matches if there are any empty tiles below. Should this be kept?
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
          P->ChainCounter++;
          // was originally using MaxChain>>8 here but that's not accurate to the original game
          // which I think can only handle one chain at a time
          // see https://www.youtube.com/watch?v=2GwvWqrhp4o
          IsChainActive = 1;
//          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Chain (maxchain) %i", MaxChain>>8);
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Chain %i", P->ChainCounter);
          P->Score += PointsForChainPart(P->ChainCounter);
        }

      if(Horiz >= P->MinMatchSize-1) {
        for(int i=0; i<=Horiz; i++)
          Used[x+i][y] = 1;
        Used[x][y] = Horiz+1; // write the width
      }
    }

  if(P->ChainResetTimer) {
    P->ChainResetTimer--;
    if(!P->ChainResetTimer) {
      LogMessage("Chain of length %i ended", P->ChainCounter);
      P->ChainCounter = 0;
    }
  }
  if(IsChainActive)
    P->ChainResetTimer = 0;
  if(!IsChainActive && P->ChainCounter && !P->ChainResetTimer)
    P->ChainResetTimer = 2;

  // create match structs for the matches that are found, and do related tasks
  int ComboSize = 0, ComboChainSize = 0;
  // look for chains first
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      int Chain = GetTile(P, x, y)&PF_CHAIN;
      if(Chain > ComboChainSize)
        ComboChainSize = Chain;
    }
  // now actually go make those structs
  int MatchULX = -1, MatchULY = -1;
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      if(Used[x][y] || UsedV[x][y]) {
        // Tetris Attack displays the combo number next to the upper left most tile in the match
        if(MatchULX < 0) {
          MatchULX = x;
          MatchULY = y;
        }
        // trigger garbage clears next to every cleared tile
        TriggerGarbageClear(P, x-1, y, IsGarbage);
        TriggerGarbageClear(P, x+1, y, IsGarbage);
        TriggerGarbageClear(P, x, y-1, IsGarbage);
        TriggerGarbageClear(P, x, y+1, IsGarbage);

        // allocate match struct, fill everything in
        struct MatchRow *Match = (struct MatchRow*)calloc(1, sizeof(struct MatchRow));
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

        // replace the clearing tiles with the disabled tile type
        for(int i=0; i<Width; i++)
          SetTile(P, x+i, y, BLOCK_DISABLED);

        // add this match struct to the list
        if(!FirstMatch)
          FirstMatch = Match;
        if(CurMatch)
          CurMatch->Child = Match;
        CurMatch = Match;

        x+=Width-1;
      }
    }

  // give points if a combo was made
  if(ComboSize) {
    P->Score += BasePointsForCombo(ComboSize);
    // chain bonus
    if(MaxActiveChain)
      P->Score += PointsForChainPart(MaxActiveChain-1);

    // play sound effects and make visual effects
    if(ComboSize >= 4 || (ComboSize == 3 && ComboChainSize)) {
      Mix_PlayChannel(-1, SampleCombo, 0);

      struct ComboNumber *Num = (struct ComboNumber*)malloc(sizeof(struct ComboNumber));
      Num->X = MatchULX*TILE_W + TILE_W/2;
      Num->Y = MatchULY*TILE_H + TILE_H/2;
      if(P->ChainCounter) {
        Num->Number = P->ChainCounter+1; //(ComboChainSize>>8)+1;
        Num->Flags = TEXT_CHAIN|TEXT_CENTERED;
      } else {
        Num->Number = ComboSize;
        Num->Flags = TEXT_CENTERED;
      }
      Num->Timer = 30;
      Num->Next = P->ComboNumbers;
      Num->Speed = 0;
      P->ComboNumbers = Num;
    }
  }

  // Add the list of matches to the playfield struct
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
  for(struct MatchRow *Match = P->Match; Match;) {
    struct MatchRow *NextMatch = Match->Next;

    // Stay white for a moment
    if(Match->Timer1) {
      Match->Timer1--;
      Match = NextMatch;
      continue;
    }

    // Start erasing blocks.
    // Find a match that still has blocks to erase
    struct MatchRow *Last = Match;
    while(!Last->DisplayWidth)
      Last = Last->Child;

    // Wait a delay before actually erasing a block
    Last->Timer2--;
    if(!Last->Timer2) {
      Last->Timer2 = 10; // Reset the timer

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
          P->Match = P->Match->Next;
        } else {
          struct MatchRow *Find = P->Match;
          while(Find->Next != Match)
            Find = Find->Next;
          Find->Next = Match->Next;
        }

        // free, and also erase all those blocks
        for(struct MatchRow *Free = Match; Free;) {
          // mark chain counters
          for(int x=Free->X; x<(Free->X+Free->Width); x++) {
            for(int y=Free->Y; GetColor(P, x, y) && y && !IsGarbage[y*P->Width + x]; y--) {
              int Chain = GetTile(P, x, y) & PF_CHAIN;
              if(Chain < Free->Chain+PF_CHAIN_ONE)
                SetTile(P, x, y, GetColor(P, x, y) | (Free->Chain+PF_CHAIN_ONE));
            }
          }

          struct MatchRow *Next = Free->Child;
          free(Free);
          Free = Next;
        }
      }
    }
    Match = NextMatch;
  }

  // Change disabled blocks back to regular ones
  memset(Used, 0, sizeof(Used));
  for(struct GarbageSlab *Slab = P->GarbageSlabs; Slab; Slab=Slab->Next)
    for(int i=0; i<Slab->Width; i++)
      for(int j=0; j<Slab->Height; j++)
        Used[Slab->X+i][Slab->Y+j] = 1;
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

  // Look for tiles that need to start falling, and make a FallingChunk so they start falling
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
        // create the struct
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
//    } else if(ColorAtBottom == BLOCK_DISABLED) {
//      F->Timer = HOVER_TIME;
//      F = Next;
//      continue;
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

  // Make garbage slabs fall too
  for(struct GarbageSlab *Slab = P->GarbageSlabs; Slab; Slab=Slab->Next) {
    if(Slab->Clearing)
      continue;
    int OkayToFall = 1;
    int Bottom = Slab->Y + Slab->Height;
    if(Bottom >= P->Height-1)
      OkayToFall = 0;
    else for(int x=Slab->X; x<(Slab->X+Slab->Width); x++)
      if(GetTile(P, x, Bottom) & PF_COLOR)
        OkayToFall = 0;
    if(OkayToFall) {
      for(int x=Slab->X; x<(Slab->X+Slab->Width); x++) {
        SetTile(P, x, Slab->Y, 0);
        SetTile(P, x, Bottom, BLOCK_DISABLED);
      }
      Slab->Y++;
    }
  }


/////////////////// RISING ///////////////////

  // Handle rising
  if(!P->LiftKeyOn && !P->RiseStopTimer && (!P->Match || P->Flags&LIFT_WHILE_CLEARING) && P->KeyDown[KEY_LIFT]) {
    P->Score++;
    if(P->Flags & INSTANT_LIFT)
      P->Rise = 16;
    else {
      P->LiftKeyOn = 1;
      P->RiseStopTimer = 10;
    }
  }

  if(P->LiftKeyOn)
    P->Rise++;
  else if(P->RiseStopTimer)
    P->RiseStopTimer--;
  else if(!P->Match && !(retraces & 15))
    P->Rise++;
  if(P->Rise >= 16) {
    P->LiftKeyOn = 0;

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

    // move garbage slabs up
    for(struct GarbageSlab *Slab = P->GarbageSlabs; Slab; Slab=Slab->Next)
      Slab->Y--;

    for(struct ComboNumber *Num = P->ComboNumbers; Num; Num=Num->Next)
      Num->Y -= TILE_H;

    // move exploding blocks up
    for(struct MatchRow *Heads = P->Match; Heads; Heads=Heads->Next)
      for(struct MatchRow *Match = Heads; Match; Match=Match->Child)
        Match->Y--;
  }

  if(!P->CursorY)
   P->CursorY = 1;
}
