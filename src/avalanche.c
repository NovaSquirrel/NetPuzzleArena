/*
 * Net Puzzle Arena
 *
 * Copyright (C) 2016-2020 NovaSquirrel
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.	If not, see <http://www.gnu.org/licenses/>.
 */
#include "puzzle.h"

int GetTile2(struct Playfield *P, int x, int y) {
	return GetTile(P, P->CursorX + DirX[P->Direction] + x, P->CursorY + DirY[P->Direction] + y);
}

void UpdateAvalanche(struct Playfield *P) {
	int GoDown = 0;
	int OldX = P->CursorX;
	int OldY = P->CursorY;

	if(P->KeyNew[KEY_PAUSE])
		P->Paused ^= 1;
	if(P->Paused)
		return;

	if(P->SwapColor1 == BLOCK_EMPTY && P->SwapColor2 == BLOCK_EMPTY) {
		ClearMatchAnimation(P, 0);
		if(ClearAvalancheStyle(P))
			return;

		P->SwapColor1 = random_tile_color(P);
		P->SwapColor2 = random_tile_color(P);
		P->Direction = NORTH;
		P->CursorX = P->Width/2;
		P->CursorY = 1;
		P->LockTimer = 0;
	}

	int OldDirection = P->Direction;

	if(P->KeyNew[KEY_LEFT]) {
		P->CursorX -= (P->CursorX != 0) && (P->CursorX + DirX[P->Direction] != 0) && !GetTile1(P, -1, 0) && !GetTile2(P, -1, 0);
		P->LockTimer = 0;
	}

	if(P->KeyNew[KEY_RIGHT]) {
		P->CursorX += (P->CursorX != P->Width-1) && (P->CursorX + DirX[P->Direction] != P->Width-1) && !GetTile1(P, 1, 0) && !GetTile2(P, 1, 0);
		P->LockTimer = 0;
	}

	if(P->KeyNew[KEY_ROTATE_L]) {
		P->Direction = (P->Direction - 2) & 7;
		P->LockTimer = 0;
	}

	if(P->KeyNew[KEY_ROTATE_R]) {
		P->Direction = (P->Direction + 2) & 7;
		P->LockTimer = 0;
	}

	if(P->KeyNew[KEY_ROTATE_L] || P->KeyNew[KEY_ROTATE_R]) {
		if(P->Direction == EAST && GetTile2(P, 0, 0)) {
			if(!GetTile1(P, 1, 0))
				P->CursorX--;
			else {
				P->Direction = OldDirection ^= 4;
				P->CursorY -= DirY[P->Direction];
			}
		}
		else if(P->Direction == WEST && GetTile2(P, 0, 0)) {
			if(!GetTile1(P, -1, 0))
				P->CursorX++;
			else {
				P->Direction = OldDirection ^= 4;
				P->CursorY -= DirY[P->Direction];
			}
		}
		else if(P->Direction == SOUTH && GetTile2(P, 0, 0))
			P->CursorY--;
	}

	if(P->CursorX + DirX[P->Direction] >= P->Width)
		P->CursorX--;
	if(P->CursorX + DirX[P->Direction] < 0)
		P->CursorX++;
	if(P->CursorY + DirY[P->Direction] >= P->Height - 1)
		P->CursorY--;

	if(P->KeyDown[KEY_DOWN] && !(retraces & 3))
		GoDown = 1;

	P->FallTimer++;
	if(P->FallTimer == 60) {
		P->FallTimer = 0;
		GoDown = 1;
	}

	if(GoDown) {
		if(!GetTile1(P, 0, 1) && !GetTile2(P, 0, 1) && P->CursorY != P->Height-2 && P->CursorY + DirY[P->Direction] != P->Height - 2)
			P->CursorY++;
	}

	// revert to the old direction and position if we're overlapping another thing now
	if(GetTile1(P, 0, 0) || GetTile2(P, 0, 0)) {
		P->CursorX = OldX;
		P->CursorY = OldY;
		P->Direction = OldDirection;
	}

	if(P->CursorY == P->Height-2 || (P->CursorY + DirY[P->Direction]) == P->Height - 2 || GetTile1(P, 0, 1) || GetTile2(P, 0, 1)) {
		P->LockTimer++;
		if(P->LockTimer > 16) {
			SetTile(P, P->CursorX, P->CursorY, P->SwapColor1);
			SetTile(P, P->CursorX+DirX[P->Direction], P->CursorY+DirY[P->Direction], P->SwapColor2);
			P->SwapColor1 = 0;
			P->SwapColor2 = 0;
		}
	}

}
