#pragma once

extern void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p);
extern int panel_col_of(int col);
extern void apply_default_feat_lighting(TERM_COLOR f_attr[F_LIT_MAX], SYMBOL_CODE f_char[F_LIT_MAX]);
extern void prt_map(void);
extern void map_info(POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);

extern void health_track(MONSTER_IDX m_idx);
extern void prt_time(void);
extern concptr map_name(void);
extern void print_monster_list(TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
extern void move_cursor_relative(int row, int col);
extern void prt_path(POSITION y, POSITION x);
extern void monster_race_track(MONRACE_IDX r_idx);
extern void object_kind_track(KIND_OBJECT_IDX k_idx);
extern void resize_map(void);
extern void redraw_window(void);
extern bool change_panel(POSITION dy, POSITION dx);

extern void window_stuff(void);
extern void redraw_stuff(void);

