/*
 * Net Puzzle Arena
 *
 * Copyright (C) 2016-2020 NovaSquirrel
 * This file contains guidance from Panel Attack's source
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

void InitPuzzleFrenzy(struct Playfield *P) {
	for(int j=P->height-5; j>0 && j<P->height; j++)
		RandomizeRow(P, j);
	P->CursorX = 2;
	P->CursorY = 6;
}

// Takes a combo size (in number of tiles) and figures out how much garbage it amounts to
int garbage_for_combo(struct Playfield *P, int combo_size, int list_size, int *list) {
	int width = P->width;
	combo_size--; // garbage tiles is combo tiles minus 1

	int count = 0;
	// Keep generating garbage slabs until enough have been made
	while(combo_size > 0 && count <= list_size) {
		int piece_size;

		// If the next one is small enough then just go for it
		if(combo_size <= width)
			piece_size = combo_size;
		// if it's not, make sure the one after this is big enough
		else {
			piece_size = width;
			if((combo_size - piece_size) < 3)
				piece_size -= 3-(combo_size-piece_size);
		}
		combo_size -= piece_size;

		list[count++] = piece_size;
	}
	return count;
}

// Calculates the number of points to award for a combo of a given size
int base_points_for_combo(int size) {
	// https://www.gamefaqs.com/n64/913924-pokemon-puzzle-league/faqs/16679
	const static int table[] = {30,    50,    150,	 190,   230,   270,   310,   400,
								450,   500,   550,	 700,   760,   850,   970,   1120,
								1300,  1510,  1750,	 2020,  2320,  2650,  3010,  3400,
								3820,  4270,  4750,	 5260,  15000, 15570, 16170, 16800,
								17460, 18150, 18870, 19620, 20400};
	if(size < 4)
		return 0;
	if(size >= 4 && size <= 40)
		return table[size-4];
	return 20400 + ((size - 40) * 800);
}

// Calculates the number of points to award for a part of a chain
int points_for_chain_part(int size) {
// Size is the chain number from the original game, minus 1
// so if you clear blocks and cause a chain, size is 1
	const static int table[] = {50, 80, 150, 300, 400, 500, 700, 900, 1100, 1300, 1500, 1800};
	if(size <= 1)
		return 0;
	if(size <= 12)
		return table[size-1];
	return 6980 + ((size+1 - 12) * 1800);
}

int can_swap(struct Playfield *P, int right) {
	int x = P->CursorX + right, y = P->CursorY;
	switch(P->panel_extra[x][y].state) {
		case STATE_POPPING:
		case STATE_POPPED:
		case STATE_MATCHED:
		case STATE_DIMMED:
			return 0;
		case STATE_HOVERING:
			if(!(P->panel_extra[x][y].flags & FLAG_MATCH_ANYWAY))
				return 0;
	}
	if(P->panel_extra[x][y].flags & (FLAG_DONT_SWAP|FLAG_GARBAGE))
		return 0;
	// TODO: Panel Attack's vertical stack check thing?
	return 1;
}

int has_flags(struct Playfield *P, int x, int y) {
	return (P->panel_extra[x][y].state != STATE_NORMAL)
	|| (P->panel_extra[x][y].flags & (FLAG_SWAPPING_FROM_LEFT|FLAG_DONT_SWAP|FLAG_CHAINING));
}

void clear_flags(struct Playfield *P, int x, int y) {
	P->panel_extra[x][y].state = STATE_NORMAL;
	P->panel_extra[x][y].flags = 0;
}

int exclude_hover(struct Playfield *P, int x, int y) {
	if(P->panel_extra[x][y].flags & FLAG_GARBAGE)
		return 1;
	switch(P->panel_extra[x][y].state) {
		case STATE_POPPING:
		case STATE_POPPED:
		case STATE_MATCHED:
		case STATE_HOVERING:
		case STATE_FALLING:
			return 1;
	}
	return 0;
}

void swap(struct Playfield *P) {
	int x = P->CursorX, y = P->CursorY;
	int Temp = P->playfield[x][y];
	P->playfield[x][y] = P->playfield[x+1][y];
	P->playfield[x+1][y] = Temp;

	// Clear all flags except chaining
	// Panel Attack does clear_flags here, not necessary maybe?
	P->panel_extra[x][y].flags &= ~FLAG_CHAINING;
	P->panel_extra[x+1][y].flags &= ~FLAG_CHAINING;
	P->panel_extra[x+1][y].flags |= FLAG_SWAPPING_FROM_LEFT;

	P->panel_extra[x][y].state = STATE_SWAPPING;
	P->panel_extra[x+1][y].state = STATE_SWAPPING;
	P->panel_extra[x][y].timer = 4;
	P->panel_extra[x+1][y].timer = 4;

	/*	If you're swapping a panel into a position
		above an empty space or above a falling piece
		then you can't take it back since it will start falling. */
	if(y != P->height-2) {
		if(P->playfield[x][y] &&
		(!P->playfield[x][y+1] || P->panel_extra[x][y+1].state==STATE_FALLING))
			P->panel_extra[x][y].flags |= FLAG_DONT_SWAP;
		if(P->playfield[x+1][y] &&
		(!P->playfield[x+1][y+1] || P->panel_extra[x+1][y+1].state==STATE_FALLING))
			P->panel_extra[x+1][y].flags |= FLAG_DONT_SWAP;
	}

	/*	If you're swapping a blank space under a panel,
		then you can't swap it back since the panel should
		start falling. */
	if(y != 0) {
		if(!P->playfield[x][y] && P->playfield[x][y-1])
			P->panel_extra[x][y].flags |= FLAG_DONT_SWAP;
		if(!P->playfield[x+1][y] && P->playfield[x+1][y-1])
			P->panel_extra[x+1][y].flags |= FLAG_DONT_SWAP;
	}
}

void look_for_matches(struct Playfield *P) {

}

int block_garbage_fall(struct Playfield *P, int x, int y) {
	return 0;
}

void set_hoverers(struct Playfield *P, int x, int y, int hover_time, int add_chaining, int extra_tick, int match_anyway) {
	/*
		the extra_tick flag is for use during Phase 1&2,
		when panels above the first should be given an extra tick of hover time.
		This is because their timers will be decremented once on the same tick
		they are set, as Phase 1&2 iterates backward through the stack.
	*/
	int not_first = 0;
	int hovers_time = hover_time;

	while(y >= 0) {
		if(!P->playfield[x][y]
		|| exclude_hover(P, x, y)
		|| (P->panel_extra[x][y].state == STATE_HOVERING && P->panel_extra[x][y].timer <= hover_time)) {
			break;
		}
		if(P->panel_extra[x][y].state == STATE_SWAPPING) {
			hovers_time += P->panel_extra[x][y].timer - 1;
		} else {
			int chaining = P->panel_extra[x][y].flags & FLAG_CHAINING;
			clear_flags(P, x, y);
			P->panel_extra[x][y].state = STATE_HOVERING;
			P->panel_extra[x][y].flags = match_anyway ? FLAG_MATCH_ANYWAY : 0;
			// not sure what panel color 9 is for, it's in the code
			int adding_chaining = !chaining && add_chaining;
			if(chaining || adding_chaining) {
				P->panel_extra[x][y].flags |= FLAG_CHAINING;
			}
			P->panel_extra[x][y].timer = hovers_time;
			if(extra_tick) {
				P->panel_extra[x][y].timer += not_first;
			}
			if(adding_chaining) {
				P->n_chain_panels++;
			}
		}
		not_first = 1;
		y--;
	}
}

// Run the playfield for one tick
void UpdatePuzzleFrenzy(struct Playfield *P) {
	int swapped_this_frame = 0;

	if(P->KeyNew[KEY_PAUSE])
		P->Paused ^= 1;
	if(!FrameAdvance && P->Paused) {
		// allow editing the playfield for debugging stuff
		if(P->KeyNew[KEY_OK] | P->KeyNew[KEY_ITEM])
			SetTile(P, P->CursorX, P->CursorY, (GetTile(P, P->CursorX, P->CursorY)+1)%BLOCK_BLUE);
		if(P->KeyNew[KEY_ACTION])
			SetTile(P, P->CursorX+1, P->CursorY, (GetTile(P, P->CursorX+1, P->CursorY)+1)%BLOCK_BLUE);
		if(P->KeyNew[KEY_BACK]) {
			SetTile(P, P->CursorX, P->CursorY, BLOCK_EMPTY);
			SetTile(P, P->CursorX+1, P->CursorY, BLOCK_EMPTY);
		}
		return;
	}


	// Phase 0 //////////////////////////////////////////////////////////////
	// Stack automatic rising

	// TODO

	// Begin the swap we input last frame
	if(P->do_swap) {
		swap(P);
		P->do_swap = 0;
		swapped_this_frame = 1;
	}

	look_for_matches(P);

	// Clean up the value we're using to match newly hovering panels
	for(int x=0; x<P->width; x++)
		for(int y=0; y<P->height; y++)
			P->panel_extra[x][y].flags &= ~FLAG_MATCH_ANYWAY;


	// Phase 2. /////////////////////////////////////////////////////////////
	// Timer-expiring actions + falling
	char propogate_fall[PLAYFIELD_MAX_WIDTH];
	memset(propogate_fall, 0, sizeof(propogate_fall));
	int skip_col = 0;
	for(int y=P->height-1; y>=0; y--) {
		for(int x=0; x<P->width; x++) {
			if(skip_col > 0) {
				skip_col--;
				continue;
			}
			int panel = P->playfield[x][y];
			struct panel_extra *extra = &P->panel_extra[x][y];
			if(extra->flags & FLAG_GARBAGE) {
				if(extra->state == STATE_MATCHED) {
					// TODO
				}
				continue;
			}
			if(propogate_fall[x]) {
				if(block_garbage_fall(P, x, y)) {
					propogate_fall[x] = 0;
				} else {
					extra->state = STATE_FALLING;
					extra->timer = 0;
				}
			}
			if(extra->state == STATE_FALLING) {
				// Something on the bottom row will definitely land
				if(y == P->height-2) {
					extra->state = STATE_LANDING;
					extra->timer = 12;
				} else if(P->playfield[x][y+1] && P->panel_extra[x][y+1].state != STATE_FALLING) {
					// If it lands on a hovering panel, it inherits that panel's falling time
					if(P->panel_extra[x][y+1].state == STATE_HOVERING) {
						extra->state = STATE_NORMAL;
						set_hoverers(P, x, y, P->panel_extra[x][y+1].timer, 0, 0, 0);
					} else {
						extra->state = STATE_LANDING;
						extra->timer = 12;
					}
				} else {
					// Move the panel down
					P->playfield[x][y+1] = P->playfield[x][y];
					P->panel_extra[x][y+1] = P->panel_extra[x][y];
					P->playfield[x][y] = 0;
					clear_flags(P, x, y);
				}
			} else if(has_flags(P, x, y) && extra->timer) {
				extra->timer--;
				if(!extra->timer) {
					int from_left;
					switch(extra->state) {
						case STATE_SWAPPING:
							extra->state = STATE_NORMAL;
							from_left = extra->flags & FLAG_SWAPPING_FROM_LEFT;
							extra->flags &= ~(FLAG_DONT_SWAP|FLAG_SWAPPING_FROM_LEFT);

							if(!panel) {
								// an empty space finished swapping...
								// panels above it hover
								set_hoverers(P, x, y-1, HOVER_TIME+1, 0, 0, 0);
								break;
							}
							if(y != P->height-2)
								break;
							if(!P->playfield[x][y+1]) {
								set_hoverers(P, x, y, HOVER_TIME, 0, 1, 0);
								if(from_left) {
									if(P->panel_extra[x][y+1].state == STATE_FALLING)
										set_hoverers(P, x-1, y, HOVER_TIME, 0, 1, 0);
								} else {
									if(P->panel_extra[x][y-1].state == STATE_FALLING)
										set_hoverers(P, x+1, y, HOVER_TIME+1, 0, 0, 0);
								}
							} else if (P->panel_extra[x][y+1].state == STATE_HOVERING) {
								set_hoverers(P, x, y, HOVER_TIME, 0, 1, P->panel_extra[x][y+1].flags & FLAG_MATCH_ANYWAY);
							}
							break;
						case STATE_HOVERING:
							if(P->panel_extra[x][y+1].state == STATE_HOVERING) {
								extra->timer = P->panel_extra[x][y+1].timer;
							} else if(P->playfield[x][y+1]) {
								extra->state = STATE_LANDING;
								extra->timer = 12;
							} else {
								// Move the panel down
								extra->state = STATE_FALLING;
								P->playfield[x][y+1] = P->playfield[x][y];
								P->panel_extra[x][y+1] = P->panel_extra[x][y];
								P->playfield[x][y] = 0;
								clear_flags(P, x, y);
							}
							break;
						case STATE_LANDING:
							extra->state = STATE_NORMAL;
							break;
						case STATE_MATCHED:
							extra->state = STATE_POPPING;
							extra->timer = extra->combo_index * 12; // self.FRAMECOUNT_POP
							break;
						case STATE_POPPING:
							P->Score += 10;

							if(extra->combo_size == extra->combo_index) {
								P->panels_cleared++;
								// TODO: metal panels
								P->popped_panel_index = extra->combo_index;
								P->playfield[x][y] = 0;
								if(extra->flags & FLAG_CHAINING) {
									P->n_chain_panels--;
								}
								clear_flags(P, x, y);
								set_hoverers(P, x, y-1, HOVER_TIME+1, 1, 0, 1);
							} else {
								extra->state = STATE_POPPED;
								extra->timer = (extra->combo_size - extra->combo_index) * 12; // self.FRAMECOUNT_POP
								P->panels_cleared++;
								// TODO: metal panels
								P->popped_panel_index = extra->combo_index;
							}
							break;
						case STATE_POPPED:
							if(P->panels_to_speedup)
								P->panels_to_speedup--;
							if(extra->flags & FLAG_CHAINING) {
								P->n_chain_panels--;
							}
							P->playfield[x][y] = 0;
							clear_flags(P, x, y);
							set_hoverers(P, x, y-1, HOVER_TIME+1, 1, 0, 1);
							break;
						default:
							LogMessage("Call Grian");
					}
				}
			}
		}
	}

	// Phase 3. /////////////////////////////////////////////////////////////
	// Actions performed according to player input

	// CURSOR MOVEMENT
	if(P->KeyNew[KEY_LEFT])
		P->CursorX -= (P->CursorX != 0);
	if(P->KeyNew[KEY_DOWN])
		P->CursorY += (P->CursorY != P->height-2);
	if(P->KeyNew[KEY_UP])
		P->CursorY -= (P->CursorY != 1);
	if(P->KeyNew[KEY_RIGHT])
		P->CursorX += (P->CursorX != P->width-2);
	if(!P->CursorY)
		P->CursorY = 1;

	// Attempt a swap if either rotate key is pressed
	if(can_swap(P, 0) && can_swap(P, 1) && !swapped_this_frame
		&& (GetColor(P, P->CursorX, P->CursorY) || GetColor(P, P->CursorX+1, P->CursorY))
		&& (P->KeyNew[KEY_ROTATE_L] || P->KeyNew[KEY_ROTATE_R])) {
		P->do_swap = 1;
	}

	// MANUAL STACK RAISING
	// TODO

	// If at the end of the routine there are no chain panels, the chain ends.
	if(P->chain_counter && !P->n_chain_panels) {
		LogMessage("Chain of length %i ended", P->chain_counter);
		P->chain_counter = 0;
	}

	P->prev_active_panels = P->n_active_panels;
	P->n_active_panels = 0;
	for(int x=0; x<P->width; x++) {
		for(int y=0; y<P->height; y++) {
			if(
				((P->panel_extra[x][y].flags & FLAG_GARBAGE) && P->panel_extra[x][y].state)
				|| (P->playfield[x][y] && P->panel_extra[x][y].state != STATE_LANDING &&
					(exclude_hover(P, x, y) || P->panel_extra[x][y].state == STATE_SWAPPING)
					&& !(P->panel_extra[x][y].flags & FLAG_GARBAGE))
				|| P->panel_extra[x][y].state == STATE_SWAPPING
			)
				P->n_active_panels++;
		}
	}

}
