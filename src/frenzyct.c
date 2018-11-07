// Original Tetris Attack implementation by Colour Thief, adapted here
#include "puzzle.h"

#define HOVER 12
#define BURST 9
#define FLASH 16
#define MATCH 61
#define SWAP 3

#define HOVER_LEAN 12
#define BURST_LEAN 9
#define FLASH_LEAN 16
#define MATCH_LEAN 61
#define SWAP_LEAN 3

struct CT_state {
    int swap_x;
    int swap_y;
    int check_matches;
};

#define Extra(X,Y) (&P->PanelExtra[P->Width * (Y) + (X)])

void physics();
void panel_swap (struct Playfield*, struct CT_state *C, int, int, int, int);
int is_active();
int is_cleared();
void endofswap(struct Playfield*, struct CT_state *C);

void physics(struct Playfield *P) {
    if(P->KeyNew[KEY_PAUSE])
      P->Paused ^= 1;
    if(P->Paused)
      return;

    struct CT_state *C = (struct CT_state*)P->Extra;

    for (int i = P->Height-2; i >= 0; i--) {
        for (int j = 0; j < P->Width; j++) {
            if (Extra(j,i)->fall < 0) {
              Extra(j,i)->fall++;
            }
            if (Extra(j,i)->fall == 1 && Extra(j,i)->swap == 0) {
                if (Extra(j,i)->hover > 0) {
                  Extra(j,i)->hover--;
                }
                if (Extra(j,i)->hover == 0) {
                    //falling through a void
                    if (i < P->Height-1 && !GetColor(P, j, i+1)) {
                        panel_swap(P, C, j, i, j, i+1);
                    }
                    //landing on a hovering panel
                    else if (i < P->Height-1 && Extra(j, i+1)->hover > 0) {
                        Extra(j,i)->hover = Extra(j,i+1)->hover;
                    }
                    //landing properly
                    else {
                        Extra(j,i)->fall = -3;
                        C->check_matches = 1;
                    }
                }
            }
        }
    }

    //Moving
    if(P->KeyNew[KEY_LEFT])
        P->CursorX -= (P->CursorX != 0);
    if(P->KeyNew[KEY_DOWN])
        P->CursorY += (P->CursorY != P->Height-2);
    if(P->KeyNew[KEY_UP])
        P->CursorY -= (P->CursorY != 1);
    if(P->KeyNew[KEY_RIGHT])
        P->CursorX += (P->CursorX != P->Width-2);

    //Swapping in progress
    if (Extra(C->swap_x,   C->swap_y)->swap != 0
     || Extra(C->swap_x+1, C->swap_y)->swap != 0) {
        // Keep swapping
        if (Extra(C->swap_x,   C->swap_y)->swap != 0)
            Extra(C->swap_x,   C->swap_y)->swap -= 4;
        if (Extra(C->swap_x+1, C->swap_y)->swap != 0)
            Extra(C->swap_x+1, C->swap_y)->swap += 4;

        // Swap finished?
        if (Extra(C->swap_x,   C->swap_y)->swap == 0
         && Extra(C->swap_x+1, C->swap_y)->swap == 0) {
            endofswap(P, C);
            C->check_matches = 1;
        }
    }

    // Initiate a swap
    if (P->KeyNew[KEY_ROTATE_L] || P->KeyNew[KEY_ROTATE_R]) {
        //End the current swap.
        if (Extra(C->swap_x, C->swap_y)->swap != 0 || Extra(C->swap_x+1, C->swap_y)->swap != 0) {
            Extra(C->swap_x, C->swap_y)->swap = 0;
            Extra(C->swap_x+1, C->swap_y)->swap = 0;
            endofswap(P, C);
            C->check_matches = 1;
        }

        //Swap request must contain a panel.
        if (!GetColor(P, P->CursorX, P->CursorY) && !GetColor(P, P->CursorX+1, P->CursorY));
        //Swap request must not contain a hovering panel.
        else if (Extra(P->CursorX, P->CursorY)->hover > 0
              || Extra(P->CursorX+1, P->CursorY)->hover > 0);
        //Swap request must not contain a hovering panel directly above the cursor.
        else if (P->CursorY < (P->Height-1) && (Extra(P->CursorX, P->CursorY+1)->hover > 0 || Extra(P->CursorX+1, P->CursorY+1)->hover > 0));
        //Swap request must not contain a matching panel.
        else if (Extra(P->CursorX, P->CursorY)->matched > 0 || Extra(P->CursorX+1, P->CursorY)->matched > 0);
        //Swap request must not contain garbage.

        //Initiate the swap.
        else {
#ifdef ENABLE_AUDIO
            Mix_PlayChannel(-1, SampleSwap, 0);
#endif
            C->swap_x = P->CursorX;
            C->swap_y = P->CursorY;
            // Immediately move the tiles over
            panel_swap(P, C, C->swap_x, C->swap_y, C->swap_x+1, C->swap_y);
            // Start animating the tiles
            if (GetColor(P, C->swap_x, C->swap_y))
              Extra(C->swap_x, C->swap_y)->swap = 12;
            if (GetColor(P, C->swap_x+1, C->swap_y))
              Extra(C->swap_x+1, C->swap_y)->swap = -12;
        }
    }

    if (C->check_matches == 1) {
        C->check_matches = 0;
        int is_chain = 0;
        int combo = 0;
        int match_buffer[P->Width][P->Height];
        memset(match_buffer, 0, sizeof(match_buffer));
            
        //Horizontal Matches
        for (int i=0; i<=11; i++) {
            for (int j=0; j<=3; j++) {
                //no voids allowed
                if (!GetColor(P, j+2, i)) {
                  j += 2; continue;
                }
                if (!GetColor(P, j+1, i)) {
                  j += 1; continue;
                }
                if (!GetColor(P, j+0, i)) {
                  continue;
                }

                //no falling panels allowed
                if (Extra(j+2, i)->fall == 1) {
                  j += 2; continue;
                }
                if (Extra(j+1, i)->fall == 1) {
                  j += 1; continue;
                }
                if (Extra(j+0, i)->fall == 1) {
                  continue;
                }

                //no swapping panels
                if (Extra(j+2, i)->swap) {
                  j += 2; continue;
                }
                if (Extra(j+1, i)->swap) {
                  j += 1; continue;
                }
                if (Extra(j+0, i)->swap) {
                  continue;
                }

                //no matched panels
                if (Extra(j+2, i)->matched > 0) {
                  j += 2; continue;
                }
                if (Extra(j+1, i)->matched > 0) {
                  j += 1; continue;
                }
                if (Extra(j+0, i)->matched > 0) {
                  continue;
                }

                //no garbage

                if (GetColor(P, j+1, i) == GetColor(P, j+2, i)) {
                   if (GetColor(P, j, i) == GetColor(P, j+1, i)) {
                       match_buffer[j+0][i] = 1;
                       match_buffer[j+1][i] = 1;
                       match_buffer[j+2][i] = 1;
                       if (Extra(j,i)->chain || Extra(j+1, i)->chain || Extra(j+2, i)->chain)
                         is_chain = 1;
                   }
                }
                else
                    j++;
            }
        }

        //Vertical Matches
        for (int j=0; j < P->Width; j++) {
            for (int i=0; i<=9; i++) {

                //no voids allowed
                if (!GetColor(P, j, i+2)) {
                  i += 2; continue;
                }
                if (!GetColor(P, j, i+1)) {
                  i += 1; continue;
                }
                if (!GetColor(P, j, i+0)) {
                  continue;
                }

                //no falling panels allowed
                if (Extra(j, i+2)->fall == 1) {
                  i += 2; continue;
                }
                if (Extra(j, i+1)->fall == 1) {
                  i += 1; continue;
                }
                if (Extra(j, i+0)->fall == 1) {
                  continue;
                }

                //no swapping panels
                if (Extra(j, i+2)->swap) {
                  i += 2; continue;
                }
                if (Extra(j, i+1)->swap) {
                  i += 1; continue;
                }
                if (Extra(j, i+0)->swap) {
                  continue;
                }

                //no matched panels
                if (Extra(j, i+2)->matched > 0) {
                  i += 2; continue;
                }
                if (Extra(j, i+1)->matched > 0) {
                  i += 1; continue;
                }
                if (Extra(j, i+0)->matched > 0) {
                  continue;
                }

                //no garbage
                    
                if (GetColor(P, j, i+1) == GetColor(P, j, i+2)) {
                    if (GetColor(P, j, i) == GetColor(P, j, i+1)) {
                        match_buffer[j][i+0] = 1;
                        match_buffer[j][i+1] = 1;
                        match_buffer[j][i+2] = 1;
                        if (Extra(j,i)->chain || Extra(j, i+1)->chain || Extra(j, i+2)->chain)
                            is_chain = 1;
                    }
                }
                else
                    i++;
            }
        }

        //giving out timers for the match animation
        for (int i=0; i < P->Height-1; i++) {
            for (int j=0; j < P->Width; j++) {
               if (match_buffer[j][i] == 1) {
                 combo++;
                 Extra(j,i)->flash = MATCH - FLASH + 1;
                 Extra(j,i)->burst = MATCH + BURST*combo + 1;
               }
            }
        }

        for (int i=0; i < P->Height-1; i++) {
            for (int j=0; j < P->Width; j++) {
                if (match_buffer[j][i] == 1) {
                    Extra(j,i)->matched = MATCH + BURST*combo + 1;
                }
            }
        }

        //giving out cards for the match
        int placed = 0;
        for (int i=0; i < P->Height-1; i++) {
            for (int j=0; j < P->Width; j++) {
                if (match_buffer[j][i] == 1 && (is_chain || combo > 3)) {
#ifdef ENABLE_AUDIO
                    Mix_PlayChannel(-1, SampleCombo, 0);
#endif
                    struct ComboNumber *Num = (struct ComboNumber*)malloc(sizeof(struct ComboNumber));
                    Num->X = j*TILE_W + TILE_W/2;
                    Num->Y = i*TILE_H + TILE_H/2;
                    if(is_chain) {
                        P->ChainCounter++;
                        Num->Number = P->ChainCounter+1;
                        Num->Flags = TEXT_CHAIN|TEXT_CENTERED;
                    } else {
                        Num->Number = combo;
                        Num->Flags = TEXT_CENTERED;
                    }
                    Num->Timer = 30;
                    Num->Next = P->ComboNumbers;
                    Num->Speed = 0;
                    P->ComboNumbers = Num;
                    placed = 1;
                    break;
                }
            }
            if (placed)
                break;
        }
    }

    //Chain Flag Maintenance
    int fanfare = 1;
    for (int i=0; i< P->Height-1; i++) {
        for (int j=0; j < P->Width; j++) {
            if (Extra(j,i)->chain > 0) {
                fanfare = 0;
                //Falling/Hovering panels can keep their chain flag
                if (Extra(j,i)->fall == 1)
                    continue;
                //Matched panels can keep their chain flag
                if (Extra(j,i)->matched > 0)
                    continue;
                //Panels directly over a swapping panel can keep their chain flag
                if (i > 0 && Extra(j, i-1)->swap != 0)
                    continue;
                //other panels will lose their chain flag
                else
                    Extra(j,i)->chain = 0;
                }
            else if (Extra(j,i)->fall == 1)
                fanfare = 0;
            }
        }
    if (fanfare == 1)
        P->ChainCounter = 0;
    
    //Clearing
    for (int i=0; i < P->Height-1; i++) {
        for (int j=0; j < P->Width; j++) {
            if (Extra(j,i)->flash > 0)
              Extra(j,i)->flash--;
            if (Extra(j,i)->burst > 0) {
              Extra(j,i)->burst--;
              if(Extra(j,i)->burst == 0) {
#ifdef ENABLE_AUDIO
                Mix_PlayChannel(-1, SampleDisappear, 0);
#endif
              }
            }
            if (Extra(j,i)->matched > 0) {
                Extra(j,i)->matched--;
                if (Extra(j,i)->matched == 0) {
                    SetTile(P, j, i, 0);
                    Extra(j,i)->fall = 0;
                    Extra(j,i)->hover = 0;
                    Extra(j,i)->swap = 0;
                    Extra(j,i)->burst = 0;
                    Extra(j,i)->chain = 0;

                    // A match is done clearing and will now set the above panels to hover
                    for (int x=i-1; x >= 0; x--) {
                        if (!GetColor(P, j, x)) //But not voids
                            continue;
                        if (Extra(j,x)->fall == 1)   //Or falling panels
                            continue; 
                        if (Extra(j,x)->swap != 0)   //Or swapping panels
                            continue; 
                        if (Extra(j,x)->matched > 0) //Or matching panels
                            break; 
                        Extra(j,x)->fall = 1;
                        Extra(j,x)->hover = 12;
                        Extra(j,x)->chain = 1;
                    }
                }
            }
        }
    }

/////////////////// RISING ///////////////////
  // Handle rising
  if(!P->LiftKeyOn && !P->RiseStopTimer && (!P->Match || P->Flags&LIFT_WHILE_CLEARING) && P->KeyDown[KEY_LIFT]) {
    if(P->Flags & INSTANT_LIFT) {
      if(P->KeyNew[KEY_LIFT]) {
        P->Rise = 16;
        P->Score++;
      }
    } else {
      P->Score++;
      P->LiftKeyOn = 1;
      P->RiseStopTimer = 10;
    }
  }

  if(P->LiftKeyOn)
    P->Rise++;
  else if(P->RiseStopTimer)
    P->RiseStopTimer--;
  else if(!P->Match /*&& P->ChainCounter*/ && !(retraces & 15)) {
    int NoMatches = 1;
    for (int i=0; i < P->Height-1; i++) {
      for (int j=0; j < P->Width; j++) {
        if(Extra(j,i)->matched) {
          NoMatches = 0;
          break;
        }
      }
    }
    if(NoMatches)
      P->Rise++;
  }
  if(P->Rise >= 16) {
    P->LiftKeyOn = 0;

    // push playfield up
    for(int y=0; y<P->Height-1; y++)
      for(int x=0; x<P->Width; x++) {
        SetTile(P, x, y, GetTile(P, x, y+1));
        P->PanelExtra[P->Width*y + x] = P->PanelExtra[P->Width*(y+1) + x];
      }
    memset(&P->PanelExtra[P->Width*(P->Height-1)], 0, sizeof(struct panel_extra)*P->Width);

    P->CursorY--;
    P->Rise = 0;
    P->SwapY--;

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

void panel_swap(struct Playfield *P, struct CT_state *C, int srcx,int srcy, int dstx, int dsty) {
    int Color1 = GetColor(P, srcx, srcy);
    int Color2 = GetColor(P, dstx, dsty);
    SetTile(P, srcx, srcy, Color2);
    SetTile(P, dstx, dsty, Color1);

    Extra(dstx,dsty)->fall ^= Extra(srcx,srcy)->fall;
    Extra(srcx,srcy)->fall ^= Extra(dstx,dsty)->fall;
    Extra(dstx,dsty)->fall ^= Extra(srcx,srcy)->fall;
    
    Extra(dstx,dsty)->chain ^= Extra(srcx,srcy)->chain;
    Extra(srcx,srcy)->chain ^= Extra(dstx,dsty)->chain;
    Extra(dstx,dsty)->chain ^= Extra(srcx,srcy)->chain;
}

void endofswap(struct Playfield* P, struct CT_state *C) {
    for (int x = C->swap_x; x <= C->swap_x+1; x++) {
        //If the cursor space is void, the game considers the space above the cursor.
        int y = C->swap_y;
        if (!GetColor(P, x, y))
            y--;
        if (!GetColor(P, x, y))
            continue;

        for (; y >= 0; y--) {
            //Landing at the bottom is always supported.
            if (y == P->Height-2) {
                C->check_matches = 1;
                break;
            }

            //Hovers are given until a void or matched panel is encountered.
            //Note: garbage will stop it too
            if (!GetColor(P, x, y) || Extra(x,y)->matched > 0)
                break;

            //Voids and falling/hovering panels do not provide support
            if (Extra(x, y+1)->fall == 1 || !GetColor(P, x, y+1)) {
                Extra(x,y)->hover = HOVER_LEAN;
                Extra(x,y)->fall = 1;
            }

            //Otherwise it must be supported.
            else {
                  C->check_matches = 1;
                  break;
            }
        }
    }
}

void InitPuzzleFrenzyCT(struct Playfield *P) {
  for(int j=P->Height-5; j<P->Height; j++)
    RandomizeRow(P, j);

  // Allocate extra state data
  P->Extra = calloc(1, sizeof(struct CT_state));
  // struct CT_state *C = (struct CT_state*)P->Extra;

  // Allocate the extra panel data
  P->PanelExtra = calloc(P->Width * P->Height, sizeof(struct panel_extra));
}

void FreePuzzleFrenzyCT(struct Playfield *P) {
  // Free extra state information allocated
  // struct CT_state *C = (struct CT_state*)P->Extra;
  if(P->PanelExtra) {
    free(P->PanelExtra);
    P->PanelExtra = NULL;
  }

  if(P->Extra) {
    free(P->Extra);
    P->Extra = NULL;
  }
}

void UpdatePuzzleFrenzyCT(struct Playfield *P) {
  physics(P);
}
