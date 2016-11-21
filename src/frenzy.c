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

void UpdatePuzzleFrenzy(struct Playfield *P) {
  int IsFalling[P->Width][P->Height];
  memset(IsFalling, 0, sizeof(IsFalling));
  for(struct FallingChunk *Fall = P->FallingData; Fall; Fall = Fall->Next)
    for(int h=0; h<Fall->Height; h++)
      IsFalling[Fall->X][Fall->Y+h] = 1;


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
        && !IsFalling[P->CursorX][P->CursorY] && !IsFalling[P->CursorX+1][P->CursorY]
        && !IsFalling[P->CursorX][P->CursorY-1] && !IsFalling[P->CursorX+1][P->CursorY-1]) {
        P->SwapColor1 = Tile1;
        P->SwapColor2 = Tile2;
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
  memset(Used, 0, sizeof(Used));
  memset(UsedV, 0, sizeof(UsedV));
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      int Horiz = 0, Vert = 0, Color = GetTile(P, x, y);
      if(!Color || Color==BLOCK_DISABLED || IsFalling[x][y])
        continue;
      if(y < P->Height-2 && !GetTile(P, x, y+1))
        continue;

      if(!Used[x][y])
        while((x+Horiz+1 < P->Width && GetTile(P, x+Horiz+1, y) == Color) &&
              !IsFalling[x+Horiz+1][y] && (y!=P->Height-2 || GetTile(P, x+Horiz+1, y+1)))
          Horiz++;
      if(!UsedV[x][y])
        while(y+Vert+1 < P->Height-1 && !UsedV[x][y+Vert+1] && GetTile(P, x, y+Vert+1) == Color)
          Vert++;

      if(Vert >= P->MinMatchSize-1) {
        for(int i=0; i<=Vert; i++)
          UsedV[x][y+i] = 1;
      }
      if(Horiz >= P->MinMatchSize-1) {
        for(int i=0; i<=Horiz; i++)
          Used[x+i][y] = 1;
        Used[x][y] = Horiz+1; // write the width
      }
    }

  // create match structs for the matches that are found
  int ComboSize = 0;
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      if(Used[x][y] || UsedV[x][y]) {
        struct MatchRow *Match = (struct MatchRow*)malloc(sizeof(struct MatchRow));
        int Width = Used[x][y];
        if(!Width)
          Width = 1;
        ComboSize += Width;
        Match->Color = GetTile(P, x, y);
        Match->X = x;
        Match->Y = y;
        Match->DisplayX = x;
        Match->DisplayWidth = Width;
        Match->Width = Width;
        Match->Child = NULL;
        Match->Next = NULL;
        Match->Timer1 = 0;
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
  if(ComboSize)
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Combo of size %i", ComboSize);

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
      if(GetTile(P, x, y) == BLOCK_DISABLED && !Used[x][y]
      && (!P->SwapTimer || (y != P->CursorY && (x != P->CursorX && x != P->CursorX+1))))
        SetTile(P, x, y, BLOCK_EMPTY);

/////////////////// GRAVITY ///////////////////

  // gravity
  for(int x=0; x< P->Width; x++) {
    for(int y=0; y<P->Height-1; y++) {
      int Color = GetTile(P, x, y);
      if(!Color || Color == BLOCK_DISABLED || IsFalling[x][y])
        continue;

      // find the bottom of this stack
      int Bottom = y+1;
      while(Bottom < P->Height-1) {
        int Color2 = GetTile(P, x, Bottom);
        if(!Color2 || Color2 == BLOCK_DISABLED || IsFalling[x][Bottom])
          break;
        Bottom++;
      }

      if(Bottom == P->Height-1)
        break;
      if(GetTile(P, x, Bottom) != BLOCK_DISABLED) {
        struct FallingChunk *Fall = (struct FallingChunk*)malloc(sizeof(struct FallingChunk));
        Fall->X = x;
        Fall->Y = y;
        Fall->Height = Bottom-y;
        Fall->Timer = 12;
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
        struct FallingChunk *Prev = P->FallingData;
        while(Prev->Next != F)
          Prev = Prev->Next;
        Prev->Next = F->Next;
      }
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
  if((!P->Match || P->Flags&LIFT_WHILE_CLEARING) && P->KeyNew[KEY_LIFT])
    P->Rise = 16;
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
