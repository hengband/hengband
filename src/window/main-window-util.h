#pragma once

#include "system/angband.h"

#define ROW_MAP 0
#define COL_MAP 12

class ObjectType;
extern ObjectType *autopick_obj;
extern POSITION panel_row_min;
extern POSITION panel_row_max;
extern POSITION panel_col_min;
extern POSITION panel_col_max;
extern POSITION panel_col_prt;
extern POSITION panel_row_prt;
extern int match_autopick;
extern int feat_priority;

class PlayerType;
void print_field(concptr info, TERM_LEN row, TERM_LEN col);
void print_map(PlayerType *player_ptr);
void display_map(PlayerType *player_ptr, int *cy, int *cx);
void set_term_color(PlayerType *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, char *cp);
int panel_col_of(int col);
