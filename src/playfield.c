#include "puzzle.h"
#define TIMER1_TIME 60
#define TIMER2_TIME 8

int PlayfieldWidth = 6;
int PlayfieldHeight = 13;

inline int GetTile(struct Playfield *P, int X, int Y) {
  return P->Playfield[P->Width * Y + X];
}

inline void SetTile(struct Playfield *P, int X, int Y, int Value) {
  P->Playfield[P->Width * Y + X] = Value;
}

void UpdatePlayfield(struct Playfield *P) {
  // Update keys, do key repeat
  for(int i=0; i<KEY_COUNT; i++) {
    P->KeyNew[i] = P->KeyDown[i] && !P->KeyLast[i];

    if(i < KEY_OK) {
      if(P->KeyDown[i])
        P->KeyRepeat[i]++;
      else
        P->KeyRepeat[i] = 0;

      if(P->KeyRepeat[i] > 8)
        P->KeyNew[i] = 1;
    }
  }
  memcpy(P->KeyLast, P->KeyDown, sizeof(P->KeyDown));

  int SwapMade = 0;

  // Cursor movement
  if(P->KeyNew[KEY_LEFT])
    P->CursorX -= (P->CursorX != 0);
  if(P->KeyNew[KEY_DOWN])
    P->CursorY += (P->CursorY != P->Height-2);
  if(P->KeyNew[KEY_UP])
    P->CursorY -= (P->CursorY != 1);
  if(P->KeyNew[KEY_RIGHT])
    P->CursorX += (P->CursorX != P->Width-2);
  if(P->KeyNew[KEY_SWAP]) {
    int Tile1 = GetTile(P, P->CursorX, P->CursorY);
    int Tile2 = GetTile(P, P->CursorX+1, P->CursorY);
    if((Tile1 != BLOCK_DISABLED && Tile2 != BLOCK_DISABLED) &&
       (!P->FallingColumns[P->CursorX].IsFalling || P->FallingColumns[P->CursorX].SwapLock < P->CursorY) &&
       (!P->FallingColumns[P->CursorX+1].IsFalling || P->FallingColumns[P->CursorX+1].SwapLock < P->CursorY)) {
      SetTile(P, P->CursorX, P->CursorY, Tile2);
      SetTile(P, P->CursorX+1, P->CursorY, Tile1);
    }
    SwapMade = 1;
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

  // Look for matches
  struct MatchRow *FirstMatch = NULL, *CurMatch = NULL;
  int Used[P->Width][P->Height]; 
  memset(Used, 0, sizeof(Used));
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      int Horiz = 0, Vert = 0, Color = GetTile(P, x, y);
      if(!Color || Color==BLOCK_DISABLED)
        continue;
      if(P->FallingColumns[x].IsFalling && P->FallingColumns[x].GroundLevel > y)
        continue;

      while((x+Horiz+1 < P->Width && GetTile(P, x+Horiz+1, y) == Color) &&
           ((!P->FallingColumns[x+Horiz+1].IsFalling || P->FallingColumns[x+Horiz+1].GroundLevel <= y) ||
            (SwapMade && ((x+Horiz+1 == P->CursorX) || (x+Horiz+1 == P->CursorX+1)))))
        Horiz++;
      while(y+Vert+1 < P->Height-1 && !Used[x][y+Vert+1] && GetTile(P, x, y+Vert+1) == Color)
        Vert++;

      if(Vert >= 2) {
        for(int i=0; i<=Vert; i++)
          Used[x][y+i] = 1;
      }
      if(Horiz >= 2) {
        for(int i=0; i<=Horiz; i++)
          Used[x+i][y] = 1;
        Used[x][y] = Horiz+1; // write the width
        x+=Horiz;
      }
    }

  // create match structs for the matches that are found
  for(int y=0; y<P->Height-1; y++)
    for(int x=0; x<P->Width; x++) {
      if(Used[x][y]) {
        struct MatchRow *Match = (struct MatchRow*)malloc(sizeof(struct MatchRow));
        int Width = Used[x][y];
        Match->Color = GetTile(P, x, y);
        Match->X = x;
        Match->Y = y;
        Match->DisplayX = x;
        Match->DisplayWidth = Width;
        Match->Width = Width;
        Match->Child = NULL;
        Match->Next = NULL;
        Match->Timer1 = 0;
        Match->Timer2 = TIMER2_TIME;

        for(int i=0; i<Width; i++)
          SetTile(P, x+i, y, BLOCK_DISABLED);
        if(!FirstMatch)
          FirstMatch = Match;
        if(CurMatch)
          CurMatch->Child = Match;
        CurMatch = Match;

        x+=Used[x][y]-1;
      }
    }
  if(FirstMatch) {
    if(!P->Match)
      P->Match = FirstMatch;
    else {
      FirstMatch->Next = P->Match;
      P->Match = FirstMatch;
    }
    FirstMatch->Timer1 = TIMER1_TIME;
  }
  
  // Make blocks explode
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
      Last->Timer2 = TIMER2_TIME;
      Last->DisplayX++;
      Last->DisplayWidth--;

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
      if(GetTile(P, x, y) == BLOCK_DISABLED && !Used[x][y])
        SetTile(P, x, y, BLOCK_EMPTY);

  // gravity
  for(int x=0; x< P->Width; x++) {
    int HighestGround = P->Height-1;
    while(HighestGround && GetTile(P, x, HighestGround))
      HighestGround--;
    // if the column is not all the way filled up, look for any floating blocks
    if(HighestGround) {
      int LowestFloating = HighestGround;
      while(LowestFloating && !GetTile(P, x, LowestFloating))
        LowestFloating--;
      // if floating blocks are found, decide what to do
      if(LowestFloating) {
        if(GetTile(P, x, LowestFloating) == BLOCK_DISABLED) {
          P->FallingColumns[x].IsFalling = 0;
          continue;
        }
        // if the falling just now started, set a timer
        if(!P->FallingColumns[x].IsFalling) {
          P->FallingColumns[x].Timer = 20;
          P->FallingColumns[x].IsFalling = 1;
        } else {
          if(P->FallingColumns[x].Timer)
            P->FallingColumns[x].Timer--;
          else {
            for(int y=HighestGround; y; y--)
              SetTile(P, x, y, GetTile(P, x, y-1));
            SetTile(P, x, 0, BLOCK_EMPTY);
          }
        }
        P->FallingColumns[x].SwapLock = LowestFloating;
        P->FallingColumns[x].GroundLevel = HighestGround;
      } else {
        P->FallingColumns[x].IsFalling = 0;
      }
    }
  }

  // Handle rising
  if(!(retraces & 15))
    P->Rise++;
  if(P->KeyNew[KEY_LIFT])
    P->Rise = 16;
  if(P->Rise >= 16) {
    // push playfield up
    for(int y=0; y<P->Height-1; y++)
      for(int x=0; x<P->Width; x++)
        SetTile(P, x, y, GetTile(P, x, y+1));
    P->CursorY--;
    P->Rise = 0;

    // generate new blocks
    for(int x=0; x<P->Width; x++) {
      SetTile(P, x, P->Height-1, (rand()%(BLOCK_BLUE-1))+1);
      // also update falling data
      P->FallingColumns[x].SwapLock--;
    }

    // move exploding blocks up
    for(struct MatchRow *Heads = P->Match; Heads; Heads=Heads->Next)
      for(struct MatchRow *Match = Heads; Match; Match=Match->Child)
        Match->Y--;
  }

  if(!P->CursorY)
   P->CursorY = 1;
}
