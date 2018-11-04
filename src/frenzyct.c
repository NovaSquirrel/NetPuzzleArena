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

void draw_screen();
void physics();
void physics_nodes();
void endofswap();
void endofswap_nodes();
void panel_swap (struct Playfield*, int, int, int, int);
int is_active();
int is_cleared();

struct panel {
    int colour;
    int fall;
    int hover;
    int swap;
    int matched;
    int burst;
    int flash;
    int chain;
};

struct node {
    int x;
    int y;
    struct node *right;
};

struct coordinate {
    int x;
    int y;
};

struct CT_state {

};

struct coordinate swap;
struct panel panel[6][12];
int move_state[5][6][12];
int solution[5][2];
int card[6][12];
float card_offset[6][12];
int chain = 1;
int is_active_flag = 0;
int is_cleared_flag = 0;
int upper_limit = 11;
int fall_count = 0;
int match_count = 0;
int fall_left = 5;
int fall_right = 0;
int match_bottom = 11;
int match_left = 5;
int match_right = 0;
int mvert_left = 6;
int mvert_right = 0;
int mvert_upper = 0;
int mvert_bottom = 11;
int mhori_left = 5;
int mhori_right = 0;
int mhori_upper = 0;
int mhori_bottom = 11;

struct node fall[72];
struct node *fall_start = &fall[0];
struct node *fall_end = &fall[0];
struct node match[72];
struct node *match_start = &match[0];
struct node *match_end = &match[0];

SDL_Surface *screen;
SDL_Surface *panels;
SDL_Surface *cards;
SDL_Surface *cursors;
SDL_Surface *colourswap;
int picked_colour = 1;
float gLastTick = 0;

int check_matches = 0;


void physics(struct Playfield *P)
{
    //Falling
    for (int i=0; i<=11; i++) {
        for (int j=0; j<=5; j++) {
            if (panel[j][i].fall < 0) panel[j][i].fall++;
            if (panel[j][i].fall == 1  && panel[j][i].swap == 0) {
                if (panel[j][i].hover > 0) panel[j][i].hover--;
                if (panel[j][i].hover == 0) {
                    //falling through a void
                    if (i > 0 && panel[j][i-1].colour == 0)
                        panel_swap(P, j, i, j, i-1);
                    //landing on a hovering panel
                    else if (i > 0 && panel[j][i-1].hover > 0)
                        panel[j][i].hover = panel[j][i-1].hover;
                    //landing properly
                    else {
                        panel[j][i].fall = -3;
                        check_matches = 1;
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

    //Swapping
    if (panel[swap.x][swap.y].swap != 0 || panel[swap.x+1][swap.y].swap != 0) {
        if (panel[swap.x][swap.y].swap != 0) panel[swap.x][swap.y].swap -= 4;
        if (panel[swap.x+1][swap.y].swap != 0) panel[swap.x+1][swap.y].swap += 4;
        if (panel[swap.x][swap.y].swap == 0 && panel[swap.x+1][swap.y].swap == 0) {
            endofswap();
            check_matches = 1;
            }
        }
    if (P->KeyNew[KEY_ROTATE_L] || P->KeyNew[KEY_ROTATE_R]) {
        //End the current swap.
        if (panel[swap.x][swap.y].swap != 0 || panel[swap.x+1][swap.y].swap != 0) {
            panel[swap.x][swap.y].swap = 0;
            panel[swap.x+1][swap.y].swap = 0;
            endofswap();
            check_matches = 1;
            }

        //Swap request must contain a panel.
        if (panel[P->CursorX][P->CursorY].colour == 0 && panel[P->CursorX+1][P->CursorY].colour == 0);
        //Swap request must not contain a hovering panel.
        else if (panel[P->CursorX][P->CursorY].hover > 0 || panel[P->CursorX+1][P->CursorY].hover > 0);
        //Swap request must not contain a hovering panel directly above the cursor.
        else if (P->CursorY+1 <= 11 && (panel[P->CursorX][P->CursorY+1].hover > 0 || panel[P->CursorX+1][P->CursorY+1].hover > 0));
        //Swap request must not contain a matching panel.
        else if (panel[P->CursorX][P->CursorY].matched > 0 || panel[P->CursorX+1][P->CursorY].matched > 0);
        //Swap request must not contain garbage.

        //Initiate the swap.
        else {
            swap.x = P->CursorX;
            swap.y = P->CursorY;
            panel_swap (P, swap.x, swap.y, swap.x+1, swap.y);
            if (panel[swap.x][swap.y].colour != 0) panel[swap.x][swap.y].swap = 12;
            if (panel[swap.x+1][swap.y].colour != 0) panel[swap.x+1][swap.y].swap = -12;
            }
        }

    //Match Checking
    //animating currently active cards
    for (int i=0; i<=11; i++) {
        for (int j=0; j<=5; j++) {
            if (card[j][i] != 0) {
                if (card_offset[j][i] > 8) { card[j][i] = 0; card_offset[j][i] = 0; }
                else if (card_offset[j][i] >= 7) card_offset[j][i] += 0.0625;
                else if (card_offset[j][i] >= 6) card_offset[j][i] += 0.125;
                else if (card_offset[j][i] >= 4) card_offset[j][i] += 0.25;
                else if (card_offset[j][i] >= 0) card_offset[j][i] += 0.5;
                }
            }
        }
    if (check_matches == 1) {
        check_matches = 0;
        int is_chain = 0;
        int combo = 0;
        int match_buffer[6][12];
        for (int i=0; i<=11; i++) for (int j=0; j<=5; j++) match_buffer[j][i] = 0;
            
        //Horizontal Matches
        for (int i=0; i<=11; i++) {
            for (int j=0; j<=3; j++) {

                //no voids allowed
                if (panel[j+2][i].colour == 0) { j+=2; continue; }
                if (panel[j+1][i].colour == 0) { j++; continue; }
                if (panel[j+0][i].colour == 0) continue;
                //no falling panels allowed
                if (panel[j+2][i].fall == 1) { j+=2; continue; }
                if (panel[j+1][i].fall == 1) { j++; continue; }
                if (panel[j+0][i].fall == 1) continue;
                //no swapping panels
                if (panel[j+2][i].swap != 0) { j+=2; continue; }
                if (panel[j+1][i].swap != 0) { j++; continue; }
                if (panel[j+0][i].swap != 0) continue;
                //no matched panels
                if (panel[j+2][i].matched > 0) { j+=2; continue; }
                if (panel[j+1][i].matched > 0) { j++; continue; }
                if (panel[j+0][i].matched > 0) continue;
                //no garbage

                if (panel[j+1][i].colour == panel[j+2][i].colour) {
                   if (panel[j][i].colour == panel[j+1][i].colour) {
                       match_buffer[j+0][i] = 1;
                       match_buffer[j+1][i] = 1;
                       match_buffer[j+2][i] = 1;
                       if (panel[j][i].chain || panel[j+1][i].chain || panel[j+2][i].chain) is_chain = 1;
                       }
                   } else j++;
                }
            }

        //Vertical Matches
        for (int j=0; j<=5; j++) {
            for (int i=0; i<=9; i++) {

                //no voids allowed
                if (panel[j][i+2].colour == 0) { i+=2; continue; }
                if (panel[j][i+1].colour == 0) { i++; continue; }
                if (panel[j][i+0].colour == 0) continue;
                //no falling panels allowed
                if (panel[j][i+1].fall == 1) { i++; continue; }
                if (panel[j][i+2].fall == 1) { i+=2; continue; }
                if (panel[j][i+0].fall == 1) continue;
                //no swapping panels
                if (panel[j][i+2].swap != 0) { i+=2; continue; }
                if (panel[j][i+1].swap != 0) { i++; continue; }
                if (panel[j][i+0].swap != 0) continue;
                //no matched panels
                if (panel[j][i+2].matched > 0) { i+=2; continue; }
                if (panel[j][i+1].matched > 0) { i++; continue; }
                if (panel[j][i+0].matched > 0) continue;
                //no garbage
                    
                if (panel[j][i+1].colour == panel[j][i+2].colour) {
                    if (panel[j][i].colour == panel[j][i+1].colour) {
                        match_buffer[j][i+0] = 1;
                        match_buffer[j][i+1] = 1;
                        match_buffer[j][i+2] = 1;
                        if (panel[j][i].chain || panel[j][i+1].chain || panel[j][i+2].chain) is_chain = 1;
                        }
                    } else i++;
                }
            }
        //giving out timers for the match animation
        for (int i=11; i>=0; i--) for (int j=0; j<=5; j++) if (match_buffer[j][i] == 1) {
            combo++;
            panel[j][i].flash = MATCH - FLASH + 1;
            panel[j][i].burst = MATCH + BURST*combo + 1;
            }
        for (int i=11; i>=0; i--) for (int j=0; j<=5; j++) if (match_buffer[j][i] == 1) {
            panel[j][i].matched = MATCH + BURST*combo + 1;
            }
        //giving out cards for the match
        int placed = 0;
        for (int i=11; i>=0; i--) {
                for (int j=0; j<=5; j++) if (match_buffer[j][i] == 1) {
                card[j][i] = combo;
                if (is_chain) {
                    chain++;
                    card[j][i] = -chain;
                    }
                if (combo > 3 && is_chain) {
                    card[j][i] = combo;
                    card[j][i+1] = -chain;
                    }
                placed = 1;
                break;
                }
            if (placed == 1) break;
            }
        }

    //Chain Flag Maintenance
    int fanfare = 1;
    for (int i=0; i<=11; i++) {
        for (int j=0; j<=5; j++) {
            if (panel[j][i].chain > 0) {
                fanfare = 0;
                //Falling/Hovering panels can keep their chain flag
                if (panel[j][i].fall == 1) continue;
                //Matched panels can keep their chain flag
                if (panel[j][i].matched > 0) continue;
                //Panels directly over a swapping panel can keep their chain flag
                if (i > 0 && panel[j][i-1].swap != 0) continue;
                //other panels will lose their chain flag
                else panel[j][i].chain = 0;
                }
            else if (panel[j][i].fall == 1) fanfare = 0;
            }
        }
    if (fanfare == 1) chain = 1;
    
    //Clearing
    for (int i=0; i<=11; i++) {
        for (int j=0; j<=5; j++) {
            if (panel[j][i].flash > 0) panel[j][i].flash--;
            if (panel[j][i].burst > 0) panel[j][i].burst--;
            if (panel[j][i].matched > 0) {
                panel[j][i].matched--;
                if (panel[j][i].matched == 0) {
                    panel[j][i].colour = 0;
                    panel[j][i].fall = 0;
                    panel[j][i].hover = 0;
                    panel[j][i].swap = 0;
                    panel[j][i].burst = 0;
                    panel[j][i].chain = 0;
                    // A match is done clearing and will now set the above panels to hover
                    for (int x=i+1; x<=11; x++) {
                        if (panel[j][x].colour == 0) continue; //But not voids
                        if (panel[j][x].fall == 1) continue; //Or falling panels
                        if (panel[j][x].swap != 0) continue; //Or swapping panels
                        if (panel[j][x].matched > 0) break; //Or matching panels
                        panel[j][x].fall = 1;
                        panel[j][x].hover = 12;
                        panel[j][x].chain = 1;
                        }
                    }
                }
            }
        }
    return;
}


void physics_nodes(struct Playfield *P)
{
    //swapping falling panels
    // Falling
    if (fall_count > 0) {
        struct node *temp = fall_start;
        struct node *prev_temp = fall_start;
        int i;
        int j;

        for (int fall_found = 0; fall_found < fall_count;) {
            i = temp->y;
            j = temp->x;
            //do nothing to swapped falling panels
            if (panel[j][i].swap != 0) { fall_found++; }
            //advance the hover counter
            else if (panel[j][i].hover > 0) { panel[j][i].hover--; fall_found++; }
            //falling through a void
            else if (i > 0 && panel[j][i-1].colour == 0) {
                panel_swap(P, j, i, j, i-1);
                temp->y--;
                fall_found++;
            //landing on a hovering panel
            } else if (i > 0 && panel[j][i-1].hover > 0) {
                panel[j][i].hover = panel[j][i-1].hover;
                fall_found++;
            //landing properly
            } else {
                panel[j][i].fall = 0;
                check_matches = 1;
                fall_count--;
 
                if (fall_count == 0) {}
                else if (fall_found == 0) {
                    if (fall_start != fall_end) fall_start = temp->right;
                    temp->right = fall_end->right;
                    fall_end->right = temp;
                    temp = fall_start;
                    prev_temp = fall_start;
                } else if (temp == fall_end) fall_end = prev_temp;
                else {
                    prev_temp->right = temp->right;
                    temp->right = fall_end->right;
                    fall_end->right = temp;
                    temp = prev_temp->right;
                    }

                // match-checking box boundaries
                if (j < mvert_left) {
                    mvert_left = j;
                    mhori_left = j-2;
                    if (mhori_left < 0) mhori_left = 0;
                    }
                if (mvert_right < j) {
                    mvert_right = j;
                    mhori_right = j;
                    if (3 < mhori_right) mhori_right = 3;
                    }
                if (mhori_upper < i) {
                    mhori_upper = i;
                    mvert_upper = i;
                    if (upper_limit-2 < mvert_upper) mvert_upper = upper_limit-2;
                    }
                if (i < mhori_bottom) {
                    mhori_bottom = i;
                    mvert_bottom = i-2;
                    if (mvert_bottom < 0) mvert_bottom = 0;
                    }
                continue;
                }
            prev_temp = temp;
            temp = temp->right;
            }
        }

    //Swapping
    if (panel[swap.x][swap.y].swap != 0 || panel[swap.x+1][swap.y].swap != 0) {
        if (panel[swap.x][swap.y].swap != 0) panel[swap.x][swap.y].swap -= 1;
        if (panel[swap.x+1][swap.y].swap != 0) panel[swap.x+1][swap.y].swap += 1;
        if (panel[swap.x][swap.y].swap == 0 && panel[swap.x+1][swap.y].swap == 0) {
            endofswap_nodes();
            }
        }
    if (P->KeyNew[KEY_SWAP] == 1) {
        //End the current swap.
        if (panel[swap.x][swap.y].swap != 0 || panel[swap.x+1][swap.y].swap != 0) {
            panel[swap.x][swap.y].swap = 0;
            panel[swap.x+1][swap.y].swap = 0;
            endofswap_nodes();
            }

        //Swap request must contain a panel.
        if (panel[P->CursorX][P->CursorY].colour == 0 && panel[P->CursorX+1][P->CursorY].colour == 0);
        //Swap request must not contain a hovering panel.
        else if (panel[P->CursorX][P->CursorY].hover > 0 || panel[P->CursorX+1][P->CursorY].hover > 0);
        //Swap request must not contain a hovering panel directly above the cursor.
        else if (P->CursorY+1 <= 11 && (panel[P->CursorX][P->CursorY+1].hover > 0 || panel[P->CursorX+1][P->CursorY+1].hover > 0));
        //Swap request must not contain a matching panel.
        else if (panel[P->CursorX][P->CursorY].matched > 0 || panel[P->CursorX+1][P->CursorY].matched > 0);
        //Swap request must not contain garbage.

        //Initiate the swap.
        else {
            swap.x = P->CursorX;
            swap.y = P->CursorY;
            panel_swap (P, swap.x, swap.y, swap.x+1, swap.y);
            if (panel[swap.x][swap.y].colour != 0) panel[swap.x][swap.y].swap = SWAP_LEAN;
            if (panel[swap.x+1][swap.y].colour != 0) panel[swap.x+1][swap.y].swap = -SWAP_LEAN;

//            //fall boundary bookkeeping
//            if (panel[swap.x][swap.y].fall == 1 && fall_left > swap.x) fall_left = swap.x;
//            if (panel[swap.x+1][swap.y].fall == 1 && fall_right < swap.x) fall_right = swap.x;
            }
        }

    //Match Checking
    if (check_matches == 1) {
        check_matches = 0;
        int combo = 0;

        //Horizontal Matches
        for (int i=mhori_bottom; i<=mhori_upper; i++) {
            for (int j=mhori_left; j<=mhori_right; j++) {

                //no voids allowed
                if (panel[j+2][i].colour == 0) { j+=2; continue; }
                if (panel[j+1][i].colour == 0) { j++; continue; }
                if (panel[j+0][i].colour == 0) continue;
                //no falling panels allowed
                if (panel[j+2][i].fall == 1) { j+=2; continue; }
                if (panel[j+1][i].fall == 1) { j++; continue; }
                if (panel[j+0][i].fall == 1) continue;
                //no swapping panels
                if (panel[j+2][i].swap != 0) { j+=2; continue; }
                if (panel[j+1][i].swap != 0) { j++; continue; }
                if (panel[j+0][i].swap != 0) continue;
                //no matched panels
                if (panel[j+2][i].matched > 0) { j+=2; continue; }
                if (panel[j+1][i].matched > 0) { j++; continue; }
                if (panel[j+0][i].matched > 0) continue;
                //no garbage

                if (panel[j+1][i].colour == panel[j+2][i].colour) {
                   if (panel[j][i].colour == panel[j+1][i].colour) {
                       if (panel[j+0][i].matched == 0) {
                           if (match_count != 0) match_end = match_end->right;
                           match_end->x = j;
                           match_end->y = i;
                           match_count++;
                           combo++;
                           panel[j+0][i].matched = -1;
                           }
                       if (panel[j+1][i].matched == 0) {
                           if (match_count != 0) match_end = match_end->right;
                           match_end->x = j+1;
                           match_end->y = i;
                           match_count++;
                           combo++;
                           panel[j+1][i].matched = -1;
                           }
                       if (panel[j+2][i].matched == 0) {
                           if (match_count != 0) match_end = match_end->right;
                           match_end->x = j+2;
                           match_end->y = i;
                           match_count++;
                           combo++;
                           panel[j+2][i].matched = -1;
                           }
                       }
                   } else j++;
                }
            }

        //Vertical Matches
        for (int j=mvert_left; j<=mvert_right; j++) {
            for (int i=mvert_bottom; i<=mvert_upper; i++) {

                //no voids allowed
                if (panel[j][i+2].colour == 0) { i+=2; continue; }
                if (panel[j][i+1].colour == 0) { i++; continue; }
                if (panel[j][i+0].colour == 0) continue;
                //no falling panels allowed
                if (panel[j][i+1].fall == 1) { i++; continue; }
                if (panel[j][i+2].fall == 1) { i+=2; continue; }
                if (panel[j][i+0].fall == 1) continue;
                //no swapping panels
                if (panel[j][i+2].swap != 0) { i+=2; continue; }
                if (panel[j][i+1].swap != 0) { i++; continue; }
                if (panel[j][i+0].swap != 0) continue;
                //no matched panels
                if (panel[j][i+2].matched > 0) { i+=2; continue; }
                if (panel[j][i+1].matched > 0) { i++; continue; }
                if (panel[j][i+0].matched > 0) continue;
                //no garbage
                    
                if (panel[j][i+1].colour == panel[j][i+2].colour) {
                    if (panel[j][i].colour == panel[j][i+1].colour) {
                        if (panel[j][i+0].matched == 0) {
                            if (match_count != 0) match_end = match_end->right;
                            match_end->x = j;
                            match_end->y = i;
                            match_count++;
                            combo++;
                            panel[j][i+0].matched = -1;
                            }
                        if (panel[j][i+1].matched == 0) {
                            if (match_count != 0) match_end = match_end->right;
                            match_end->x = j;
                            match_end->y = i+1;
                            match_count++;
                            combo++;
                            panel[j][i+1].matched = -1;
                            }
                        if (panel[j][i+2].matched == 0) {
                            if (match_count != 0) match_end = match_end->right;
                            match_end->x = j;
                            match_end->y = i+2;
                            match_count++;
                            combo++;
                            panel[j][i+2].matched = -1;
                            }
                        }
                    } else i++;
                }
            }
        // Giving out timers for the match animation
        if (combo > 0) {
            is_cleared_flag = 1;
            int animate = MATCH_LEAN + BURST_LEAN*combo + 1;
            // Skip animation when there is only 1 match and no falling panels onscreen
            if (fall_count == 0 && match_count == combo) { animate = 1; }
            struct node *temp = match_start;
            int i, j;
            while (temp != match_end->right) {
                i = temp->y;
                j = temp->x;
                if (panel[j][i].matched == -1) panel[j][i].matched = animate;
                temp = temp->right;                
                }
            }       
        mvert_left = 6;
        mvert_right = 0;
        mvert_upper = 0;
        mvert_bottom = 11;
        mhori_left = 5;
        mhori_right = 0;
        mhori_upper = 0;
        mhori_bottom = 11;

        if (is_cleared_flag == 1) {
            for (int i=0; i<=upper_limit; i++) for (int j=0; j<=5; j++) {
                if (panel[j][i].colour != 0 && panel[j][i].matched == 0) {
                    is_cleared_flag = 0;
                    i = 12;
                    j = 6;
                    }
                }
            }
        if (is_cleared_flag == 1) {
            struct node *temp = match_start;
            int i;
            int j;
            int max = 0;
            while (temp != match_end->right) {
                i = temp->y;
                j = temp->x;
                if (max < panel[j][i].matched) max = panel[j][i].matched;
                panel[j][i].matched = 0;
                panel[j][i].colour = 0;
                temp = temp->right;                
                }
            match_end = match_start;
            match_count = 0;
            return;
            }
        }

    // Skip animation when there are matched panels and no falling panels onscreen
    if (match_count > 0 && fall_count == 0 && panel[match_end->x][match_end->y].matched > 1) {
        int min = 99999;
        int i, j;
        struct node *temp = match_start;
        while (temp != match_end->right) {
            i = temp->y;
            j = temp->x;
            if (panel[j][i].matched < min) min = panel[j][i].matched;
            temp = temp->right;                
            }
        min--;
        temp = match_start;
        while (temp != match_end->right) {
            i = temp->y;
            j = temp->x;
            panel[j][i].matched -= min;
            temp = temp->right;                
            }
        }

    // Clearing
    if (match_count > 0) {
        struct node *temp = match_start;
        struct node *prev_temp = match_start;
        int i;
        int j;

        for (int match_found = 0; match_found < match_count;) {
            i = temp->y;
            j = temp->x;
            if (panel[j][i].matched > 1) {
                panel[j][i].matched--;
                match_found++;
                prev_temp = temp;
                temp = temp->right;
                }
            else if (panel[j][i].matched == 1) {
                panel[j][i].matched--;
                panel[j][i].colour = 0;
                match_count--;
                
                if (match_count == 0) {}
                else if (match_found == 0) {
                    if (match_start != match_end) match_start = temp->right;
                    temp->right = match_end->right;
                    match_end->right = temp;
                    temp = match_start;
                    prev_temp = match_start;
                } else if (temp == match_end) match_end = prev_temp;
                else {
                    prev_temp->right = temp->right;
                    temp->right = match_end->right;
                    match_end->right = temp;
                    temp = prev_temp->right;
                    }
                    
                // A match is done clearing and will now set the above panels to hover
                for (int ii=i+1; ii<=11; ii++) {
                    if (panel[j][ii].colour == 0) continue; //But not voids
                    if (panel[j][ii].fall == 1) continue; //Or falling panels
                    if (panel[j][ii].swap != 0) continue; //Or swapping panels
                    if (panel[j][ii].matched > 0) break; //Or matching panels
                    panel[j][ii].fall = 1;
                    panel[j][ii].hover = HOVER_LEAN;
                    if (fall_count != 0) fall_end = fall_end->right;
                    fall_end->x = j;
                    fall_end->y = ii;
                    fall_count++;
                    }
                }

            }
        }

    // Skip animation when there are several hovers and no matching panels onscreen
    if (fall_count > 0 && match_count == 0 && panel[fall_end->x][fall_end->y].hover > 0) {
        int min = 99999;
        int i, j;
        struct node *temp = fall_start;
        while (temp != fall_end->right) {
            i = temp->y;
            j = temp->x;
            if (panel[j][i].hover < min) min = panel[j][i].hover;
            temp = temp->right;                
            }
        temp = fall_start;
        while (temp != fall_end->right) {
            i = temp->y;
            j = temp->x;
            panel[j][i].hover -= min;
            temp = temp->right;                
            }
        }

    return;
}

void endofswap (struct Playfield *P)
{
    for (int x = swap.x; x <= swap.x+1; x++) {
        //If the cursor space is void, the game considers the space above the cursor.
        int y = swap.y;
        if (panel[x][y].colour == 0) y++;
        if (panel[x][y].colour == 0) continue;
        for (; y <= upper_limit; y++) {
            //Landing at the bottom is always supported.
            if (y == 0) {
                check_matches = 1;
                if (x < mvert_left) {
                    mvert_left = x;
                    mhori_left = x-2;
                    if (mhori_left < 0) mhori_left = 0;
                    }
                if (mvert_right < x) {
                    mvert_right = x;
                    mhori_right = x;
                    if (3 < mhori_right) mhori_right = 3;
                    }
                if (mhori_upper < swap.y) {
                    mhori_upper = swap.y;
                    mvert_upper = swap.y;
                    if (upper_limit-2 < mvert_upper) mvert_upper = upper_limit-2;
                    }
                if (swap.y < mhori_bottom) {
                    mhori_bottom = swap.y;
                    mvert_bottom = swap.y-2;
                    if (mvert_bottom < 0) mvert_bottom = 0;
                    }
                break;
                }
            //Hovers are given until a void or matched panel is encountered.
            //Note: garbage will stop it too
            if (panel[x][y].colour == 0 || panel[x][y].matched > 0) break;
            //Voids and falling/hovering panels do not provide support
            if (panel[x][y-1].fall == 1 || panel[x][y-1].colour == 0) {
                panel[x][y].hover = HOVER_LEAN;
                panel[x][y].fall = 1;
                fall_count++;
                if (x < fall_left) fall_left = x;
                if (fall_right < x) fall_right = x;
                }
            //Otherwise it must be supported.
            else {
                check_matches = 1;
                if (x < mvert_left) {
                    mvert_left = x;
                    mhori_left = x-2;
                    if (mhori_left < 0) mhori_left = 0;
                    }
                if (mvert_right < x) {
                    mvert_right = x;
                    mhori_right = x;
                    if (3 < mhori_right) mhori_right = 3;
                    }
                if (mhori_upper < swap.y) {
                    mhori_upper = swap.y;
                    mvert_upper = swap.y;
                    if (upper_limit-2 < mvert_upper) mvert_upper = upper_limit;
                    }
                if (swap.y < mhori_bottom) {
                    mhori_bottom = swap.y;
                    mvert_bottom = swap.y-2;
                    if (mvert_bottom < 0) mvert_bottom = 0;
                    }
                break;
                }
            }
        }
}

void endofswap_nodes (struct Playfield *P)
{
    for (int x = swap.x; x <= swap.x+1; x++) {
        //If the cursor space is void, the game considers the space above the cursor.
        int y = swap.y;
        if (panel[x][y].colour == 0) y++;
        if (panel[x][y].colour == 0) continue;
        for (; y <= upper_limit; y++) {
            //Landing at the bottom is always supported.
            if (y == 0) {
                check_matches = 1;
                if (x < mvert_left) {
                    mvert_left = x;
                    mhori_left = x-2;
                    if (mhori_left < 0) mhori_left = 0;
                    }
                if (mvert_right < x) {
                    mvert_right = x;
                    mhori_right = x;
                    if (3 < mhori_right) mhori_right = 3;
                    }
                if (mhori_upper < swap.y) {
                    mhori_upper = swap.y;
                    mvert_upper = swap.y;
                    if (upper_limit-2 < mvert_upper) mvert_upper = upper_limit-2;
                    }
                if (swap.y < mhori_bottom) {
                    mhori_bottom = swap.y;
                    mvert_bottom = swap.y-2;
                    if (mvert_bottom < 0) mvert_bottom = 0;
                    }
                break;
                }
            //Hovers are given until a void or matched panel is encountered.
            //Note: garbage will stop it too
            if (panel[x][y].colour == 0 || panel[x][y].matched > 0) break;
            //Voids and falling/hovering panels do not provide support
            if (panel[x][y-1].fall == 1 || panel[x][y-1].colour == 0) {
                panel[x][y].hover = HOVER_LEAN;
                panel[x][y].fall = 1;
                if (fall_count != 0) fall_end = fall_end->right;
                fall_end->x = x;
                fall_end->y = y;
                fall_count++;
                if (x < fall_left) fall_left = x;
                if (fall_right < x) fall_right = x;
                }
            //Otherwise it must be supported.
            else {
                check_matches = 1;
                if (x < mvert_left) {
                    mvert_left = x;
                    mhori_left = x-2;
                    if (mhori_left < 0) mhori_left = 0;
                    }
                if (mvert_right < x) {
                    mvert_right = x;
                    mhori_right = x;
                    if (3 < mhori_right) mhori_right = 3;
                    }
                if (mhori_upper < swap.y) {
                    mhori_upper = swap.y;
                    mvert_upper = swap.y;
                    if (upper_limit-2 < mvert_upper) mvert_upper = upper_limit;
                    }
                if (swap.y < mhori_bottom) {
                    mhori_bottom = swap.y;
                    mvert_bottom = swap.y-2;
                    if (mvert_bottom < 0) mvert_bottom = 0;
                    }
                break;
                }
            }
        }
}

void panel_swap (struct Playfield *P, int srcx,int srcy, int dstx, int dsty)
{
    panel[dstx][dsty].colour ^= panel[srcx][srcy].colour;
    panel[srcx][srcy].colour ^= panel[dstx][dsty].colour;
    panel[dstx][dsty].colour ^= panel[srcx][srcy].colour;
    
    panel[dstx][dsty].fall ^= panel[srcx][srcy].fall;
    panel[srcx][srcy].fall ^= panel[dstx][dsty].fall;
    panel[dstx][dsty].fall ^= panel[srcx][srcy].fall;
    
    panel[dstx][dsty].chain ^= panel[srcx][srcy].chain;
    panel[srcx][srcy].chain ^= panel[dstx][dsty].chain;
    panel[dstx][dsty].chain ^= panel[srcx][srcy].chain;
    
    return;
}

int is_active()
{
    for (int i=0; i<=upper_limit; i++) {
        for (int j=0; j<=5; j++) {
            if (panel[j][i].matched != 0) return 1;
            if (panel[j][i].fall != 0) return 1;
            if (panel[j][i].swap != 0) return 1;
        }
    }
    return 0;
}

int is_cleared()
{
    for (int i=0; i<=upper_limit; i++) for (int j=0; j<=5; j++) if (panel[j][i].colour != 0) return 0;
    return 1;
}

void InitPuzzleFrenzyCT(struct Playfield *P) {
  for(int j=P->Height-5; j>0 && j<P->Height; j++)
    RandomizeRow(P, j);

  P->Extra = calloc(1, sizeof(struct CT_state));
}

void FreePuzzleFrenzyCT(struct Playfield *P) {
  if(P->Extra) {
    free(P->Extra);
    P->Extra = NULL;
  }
}

void UpdatePuzzleFrenzyCT(struct Playfield *P) {
}
