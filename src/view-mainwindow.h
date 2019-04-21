#pragma once

extern int panel_col_of(int col);
extern void apply_default_feat_lighting(TERM_COLOR f_attr[F_LIT_MAX], SYMBOL_CODE f_char[F_LIT_MAX]);
extern void prt_map(void);
extern void map_info(POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);

/* xtra1.c */
extern void health_track(MONSTER_IDX m_idx);
extern void prt_time(void);
extern concptr map_name(void);
extern void print_monster_list(TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
