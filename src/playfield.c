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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.	If not, see <http://www.gnu.org/licenses/>.
 */
#include "puzzle.h"

void InitPuzzleFrenzy(struct Playfield *P);
void UpdatePuzzleFrenzy(struct Playfield *P);
void UpdateAvalanche(struct Playfield *P);
void UpdatePillars(struct Playfield *P);
void UpdateCookie(struct Playfield *P);
void UpdateReversiBall(struct Playfield *P);
void UpdateDiceMatch(struct Playfield *P);
void UpdateStacker(struct Playfield *P);

int PlayfieldWidth = 6;
int PlayfieldHeight = 13;

struct MatchRow *FirstMatch, *CurMatch; // globals to make recursive stuff easier

// Gets only the color of a tile and nothing else
inline int GetColor(struct Playfield *P, int X, int Y) {
	return P->playfield[X][Y]&PF_COLOR;
}

// Gets a tile from the playfield, with all information included
inline int GetTile(struct Playfield *P, int X, int Y) {
	return P->playfield[X][Y];
}

// Set a tile on the playfield
inline void SetTile(struct Playfield *P, int X, int Y, int Value) {
	P->playfield[X][Y] = Value;
}

// Generate a random color
int random_tile_color(struct Playfield *P) {
	return random(P->ColorCount)+1; // +1 because the empty tile is 0
}

// Count the number of same-colored blocks that are touching each other in a group, from a starting point
// "Used" is an int array that should be the size of P->width*P->height, and should be zero'd before the first CountConnected call
int CountConnected(struct Playfield *P, int X, int Y, int *Used) {
	int Color = GetTile(P, X, Y);
	int Sum = 1;
	Used[P->width * Y + X] = 1;

	if(X-1 >= 0 && !Used[P->width * Y + (X-1)] && GetTile(P, X-1, Y) == Color)
		Sum += CountConnected(P, X-1, Y, Used);
	if(X+1 <= P->width-1 && !Used[P->width * Y + (X+1)] && GetTile(P, X+1, Y) == Color) 
		Sum += CountConnected(P, X+1, Y, Used);
	if(Y-1 >= 0 && !Used[P->width * (Y-1) + X] && GetTile(P, X, Y-1) == Color) 
		Sum += CountConnected(P, X, Y-1, Used);
	if(Y+1 <= P->height-2 && !Used[P->width * (Y+1) + X] && GetTile(P, X, Y+1) == Color)
		Sum += CountConnected(P, X, Y+1, Used);

	return Sum;
}

void AddClearingTile(int X, int Y, int Color) {
	struct MatchRow *Match = (struct MatchRow*)calloc(1, sizeof(struct MatchRow));
	Match->Color = Color;
	Match->X = X;
	Match->Y = Y;
	Match->DisplayX = X;
	Match->DisplayWidth = 1;
	Match->Width = 1;
	Match->Child = NULL;
	Match->Next = NULL;
	Match->Timer1 = 0;
	Match->Chain = 0;
	Match->Timer2 = 10;

	// add this match struct to the list
	if(!FirstMatch)
		FirstMatch = Match;
	if(CurMatch)
		CurMatch->Child = Match;
	CurMatch = Match;
}

// Clear all of the same-colored blocks that are touching each other in a group, from a starting point
void ClearConnected(struct Playfield *P, int X, int Y) {
	int Color = GetTile(P, X, Y);
	SetTile(P, X, Y, BLOCK_EMPTY);

	AddClearingTile(X, Y, Color);

	if(X-1 >= 0 && GetTile(P, X-1, Y) == Color)
		ClearConnected(P, X-1, Y);
	if(X+1 <= P->width-1 && GetTile(P, X+1, Y) == Color) 
		ClearConnected(P, X+1, Y);
	if(Y-1 >= 0 && GetTile(P, X, Y-1) == Color) 
		ClearConnected(P, X, Y-1);
	if(Y+1 <= P->height-2 && GetTile(P, X, Y+1) == Color)
		ClearConnected(P, X, Y+1);
}

// Look for any blocks that are above an empty tile and move them down
int MakeBlocksFall(struct Playfield *P) {
	int BlocksFell = 0;
	for(int x=0; x<P->width; x++)
		for(int y=P->height-2; y; y--)
			if(!GetTile(P, x, y) && GetTile(P, x, y-1)) {
				SetTile(P, x, y, GetTile(P, x, y-1));
				SetTile(P, x, y-1, BLOCK_EMPTY);
				BlocksFell = 1;
			}
	return BlocksFell;
}

// Are there any tiles that can fall?
int TestBlocksFall(struct Playfield *P) {
	for(int x=0; x<P->width; x++)
		for(int y=P->height-2; y; y--)
			if(!GetTile(P, x, y) && GetTile(P, x, y-1)) {
				return 1;
			}
	return 0;
}

// Checks if a given tile contains garbage, and if so, start clearing it
void TriggerGarbageClear(struct Playfield *P, int x, int y, int *IsGarbage) {
	if(x < 0 || y < 0)
		return;
	if(x > P->width || y > P->height-1)
		return;
	if(!IsGarbage[P->width*y + x])
		return;
	if(GetColor(P, x, y) == BLOCK_GARBAGE) {
	// Is this just a single garbage tile? We can just clear it
		SetTile(P, x, y, 0);
	} else {
	// If it's a bigger garbage slab we need to go through the slab list
		struct GarbageSlab *FoundSlab = NULL;
		for(struct GarbageSlab *Slab = P->GarbageSlabs; Slab; Slab = Slab->Next)
			if(x >= Slab->X && x < (Slab->X+Slab->Width)
			&& y >= Slab->Y && y < (Slab->Y+Slab->Height)) {
				FoundSlab = Slab;
				break;
			}
		if(!FoundSlab->Clearing)
			FoundSlab->Clearing = 1;
	}
}

// Do animation for clearing blocks
void ClearMatchAnimation(struct Playfield *P, int ChainMarkers) {
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

				// adjust pointers so I can free this MatchRow in a little bit
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
					// go up vertically and set chain flags on everything above
					for(int x=Free->X; x<(Free->X+Free->Width); x++) {
						for(int y=Free->Y; y; y--) {
							// skip empty tiles
							if(!GetColor(P, x, y))
								continue;
							// set the chain flag
							if(ChainMarkers)
								SetTile(P, x, y, GetColor(P, x, y) | PF_CHAIN);
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
}

// Clears groups of blocks of the same color, but only if the groups are big enough.
// Also has those blocks fall.
// Returns 1 if blocks are currently falling, or 0 if it's time for the player to drop another piece.
int ClearAvalancheStyle(struct Playfield *P) {
	if(P->Match || MakeBlocksFall(P)) // clearing and falling blocks everything else 
		return 1;

	// make an array for CountConnected to use
	int Used[P->width * P->height];
	memset(Used, 0, sizeof(Used));

	int SoundPlayed = 0;

	// look for groups and clear them
	for(int y=0; y<P->height; y++)
		for(int x=0; x<P->width; x++) {
			if(!GetTile(P, x, y))
				continue;

			int Sum = CountConnected(P, x, y, Used);
			if(Sum >= P->MinMatchSize) {
#ifdef ENABLE_AUDIO
				if(!SoundPlayed) {
					Mix_PlayChannel(-1, SampleCombo, 0);
					SoundPlayed = 1;
				}
#endif
				// clear out the tiles
				FirstMatch = NULL;
				CurMatch = NULL;
				ClearConnected(P, x, y);
				// attach the matches made to the playfield
				if(FirstMatch) {
					if(P->Match)
						FirstMatch->Next = P->Match;
					P->Match = FirstMatch;
					FirstMatch->Timer1 = 10;
				}
			}
		}

	if(TestBlocksFall(P) || P->Match)
		return 1;
	return 0;
}

int ClearPillarsStyle(struct Playfield *P, int Diagonals) {
	if(P->Match || MakeBlocksFall(P)) // clearing and falling blocks everything else
		return 1;

	int Used[P->width][P->height];
	memset(Used, 0, sizeof(Used));

	int SoundPlayed = 0;

	FirstMatch = NULL;
	CurMatch = NULL;

	// Search for matches (lines of the same color)
	for(int y=0; y<P->height; y++) {
		for(int x=0; x<P->width; x++) {
			for(int dir=EAST; dir<WEST; dir+=((Diagonals)?1:2)) {
				 int Count = 0;
				 int Color = GetColor(P, x, y);
				 if(!Color)
					 continue;
				 // See how far the line goes
				 while(1) {
					 int TestX = x + DirX[dir] * Count;
					 int TestY = y + DirY[dir] * Count;
					 if(TestX < 0 || TestY < 0 || TestX >= P->width || TestY >= P->height)
						 break;
					 if(GetColor(P, TestX, TestY) != Color)
						 break;
					 Count++;
				 }
				 // It's a match if it's over the minimum size
				 if(Count >= P->MinMatchSize) {
#ifdef ENABLE_AUDIO
					 if(!SoundPlayed) {
						 Mix_PlayChannel(-1, SampleCombo, 0);
						 SoundPlayed = 1;
					 }
#endif

					 for(int i=0; i<Count; i++) {
						 int SetX = x + DirX[dir] * i;
						 int SetY = y + DirY[dir] * i;
						 if(!Used[SetX][SetY])
							 AddClearingTile(SetX, SetY, Color);
						 Used[SetX][SetY] = 1;
					 }
				 }
			}
		}
	}

	for(int y=0; y<P->height; y++) {
		for(int x=0; x<P->width; x++) {
			if(Used[x][y])
				SetTile(P, x, y, BLOCK_EMPTY);
		}
	}

	// Attach the list of clearing tiles to the playfield
	if(FirstMatch) {
		if(P->Match)
			FirstMatch->Next = P->Match;
		P->Match = FirstMatch;
		FirstMatch->Timer1 = 10;
	}

	if(TestBlocksFall(P) || P->Match)
		return 1;
	return 0;
}

// Sets the defaults for each type of game.
void SetGameDefaults(struct Playfield *P, int Game) {
	P->GameType = Game;
	switch(Game) {
		case FRENZY:
			P->MinMatchSize = 3;
			P->ColorCount = 5;
			P->width = 6;
			P->height = 13;
			break;
		case AVALANCHE:
			P->MinMatchSize = 4;
			P->ColorCount = 5;
			P->width = 6;
			P->height = 13;
			break;
		case REVERSI_BALL:
			P->MinMatchSize = 10;
			P->ColorCount = 2;
			P->width = 20;
			P->height = 13;
			break;
		case PILLARS:
			P->MinMatchSize = 3;
			P->ColorCount = 6;
			P->width = 6;
			P->height = 13;
			break;
	}
}

// Allocates space for a playfield
void InitPlayfield(struct Playfield *P) {
	memset(P->playfield, 0, sizeof(P->playfield));
	memset(P->panel_extra, 0, sizeof(P->panel_extra));

	// make random rows if playing Puzzle Frenzy
	switch(P->GameType) {
		case FRENZY:
			InitPuzzleFrenzy(P);
			break;
	}
}

void FreePlayfield(struct Playfield *P) {

}

// Randomizes a given playfield row, and attempts to avoid making lines of more than 3 of the same color in a row
void RandomizeRow(struct Playfield *P, int y) {
	// do not naturally create matches
	int Tries = 50;

	for(int x=0; x<P->width; x++) {
		while(Tries-->0) { // give up if it can't make an acceptable row after too many tries
			int Color = random_tile_color(P);
			if(x >= 2 && GetTile(P, x-1, y) == Color && GetTile(P, x-2, y) == Color)
				continue;
			if(y >= 2 && GetTile(P, x, y-1) == Color && GetTile(P, x, y-2) == Color)
				continue;
			SetTile(P, x, y, Color);
			break;
		}
	}
}

// Offset from cursor
int GetTile1(struct Playfield *P, int x, int y) {
	return GetTile(P, P->CursorX + x, P->CursorY + y);
}

void UpdateDiceMatch(struct Playfield *P) {

}

void UpdateCookie(struct Playfield *P) {

}

void UpdateStacker(struct Playfield *P) {

}

// Does updates that are relevant to all puzzle games, and runs the specific one for each game.
void UpdatePlayfield(struct Playfield *P) {
	// Display numbers for combos and chains that display for a bit and then disappear
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

		// Make combo numbers move up and then stop
		if(Num->Timer < 10)
			Num->Speed = 0;
		Num->Y-=Num->Speed>>2;
		Num->Speed++;

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
