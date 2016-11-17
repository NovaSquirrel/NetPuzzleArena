#include "puzzle.h"

int PlayfieldWidth = 6;
int PlayfieldHeight = 13;

inline int GetTile(struct Playfield *P, int X, int Y) {
  return P->Playfield[P->Width * Y + X];
}

inline void SetTile(struct Playfield *P, int X, int Y, int Value) {
  P->Playfield[P->Width * Y + X] = Value;
}

int RandomTileColor(struct Playfield *P) {
  return (rand()%(BLOCK_BLUE-1))+1;
}

void RandomizeRow(struct Playfield *P, int y) {
  // do not naturally create matches
  for(int x=0; x<P->Width; x++) {
    while(1) {
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

void UpdateReversiBall(struct Playfield *P) {

}

void UpdateCookie(struct Playfield *P) {

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
  }
}
