#pragma once

#include "system/angband.h"
#include "system/terrain-type-definition.h"

#define FILE_NAME_SIZE 1024

// Clipboard variables for copy&paste in visual mode
extern TERM_COLOR attr_idx;
extern char char_idx;

extern TERM_COLOR attr_idx_feat[F_LIT_MAX];
extern char char_idx_feat[F_LIT_MAX];

bool visual_mode_command(char ch, bool *visual_list_ptr, int height, int width,
    TERM_COLOR *attr_top_ptr, byte *char_left_ptr,
    TERM_COLOR *cur_attr_ptr, char *cur_char_ptr, bool *need_redraw);

bool open_temporary_file(FILE **fff, char *file_name);
void browser_cursor(char ch, int *column, IDX *grp_cur, int grp_cnt, IDX *list_cur, int list_cnt);
void display_group_list(int col, int row, int wid, int per_page, IDX grp_idx[], concptr group_text[], int grp_cur, int grp_top);
void display_visual_list(int col, int row, int height, int width, TERM_COLOR attr_top, byte char_left);
void place_visual_list_cursor(TERM_LEN col, TERM_LEN row, TERM_COLOR a, byte c, TERM_COLOR attr_top, byte char_left);
