#pragma once

#include "system/angband.h"

#define ROW_MAP 0
#define COL_MAP 12

struct object_type;
extern object_type *autopick_obj;
extern POSITION panel_row_min;
extern POSITION panel_row_max;
extern POSITION panel_col_min;
extern POSITION panel_col_max;
extern POSITION panel_col_prt;
extern POSITION panel_row_prt;
extern int match_autopick;
extern int feat_priority;

struct player_type;
void print_field(concptr info, TERM_LEN row, TERM_LEN col);
void print_map(player_type *player_ptr);
void display_map(player_type *player_ptr, int *cy, int *cx);
void set_term_color(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp);
int panel_col_of(int col);
