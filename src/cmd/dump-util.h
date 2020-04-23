#pragma once

#define FILE_NAME_SIZE 1024

// Clipboard variables for copy&paste in visual mode
extern TERM_COLOR attr_idx;
extern SYMBOL_CODE char_idx;

extern TERM_COLOR attr_idx_feat[F_LIT_MAX];
extern SYMBOL_CODE char_idx_feat[F_LIT_MAX];

bool visual_mode_command(char ch, bool *visual_list_ptr, int height, int width,
	TERM_COLOR *attr_top_ptr, byte *char_left_ptr,
	TERM_COLOR *cur_attr_ptr, SYMBOL_CODE *cur_char_ptr, bool *need_redraw);
bool open_temporary_file(FILE **fff, char *file_name);
