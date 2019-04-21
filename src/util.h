#pragma once

/* util.c */
extern errr path_parse(char *buf, int max, concptr file);
extern errr path_build(char *buf, int max, concptr path, concptr file);
extern FILE *my_fopen(concptr file, concptr mode);
extern FILE *my_fopen_temp(char *buf, int max);
extern errr my_fgets(FILE *fff, char *buf, huge n);
extern errr my_fputs(FILE *fff, concptr buf, huge n);
extern errr my_fclose(FILE *fff);
extern errr fd_kill(concptr file);
extern errr fd_move(concptr file, concptr what);
extern errr fd_copy(concptr file, concptr what);
extern int fd_make(concptr file, BIT_FLAGS mode);
extern int fd_open(concptr file, int flags);
extern errr fd_lock(int fd, int what);
extern errr fd_seek(int fd, huge n);
extern errr fd_chop(int fd, huge n);
extern errr fd_read(int fd, char *buf, huge n);
extern errr fd_write(int fd, concptr buf, huge n);
extern errr fd_close(int fd);
extern void flush(void);
extern void bell(void);
extern errr play_music(int type, int num);
extern void select_floor_music(void);
extern void sound(int num);
extern void move_cursor(int row, int col);
extern void text_to_ascii(char *buf, concptr str);
extern void ascii_to_text(char *buf, concptr str);
extern errr macro_add(concptr pat, concptr act);
extern sint macro_find_exact(concptr pat);
extern char inkey(void);
extern concptr quark_str(STR_OFFSET num);
extern void quark_init(void);
extern u16b quark_add(concptr str);
extern s32b message_num(void);
extern concptr message_str(int age);
extern void message_add(concptr msg);
extern void msg_erase(void);
extern void msg_print(concptr msg);
extern void msg_print_wizard(int cheat_type, concptr msg);
#ifndef SWIG
extern void msg_format(concptr fmt, ...);
extern void msg_format_wizard(int cheat_type, concptr fmt, ...);
#endif /* SWIG */
extern void screen_save(void);
extern void screen_load(void);
extern void c_put_str(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col);
extern void put_str(concptr str, TERM_LEN row, TERM_LEN col);
extern void c_prt(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col);
extern void prt(concptr str, TERM_LEN row, TERM_LEN col);
extern void c_roff(TERM_COLOR attr, concptr str);
extern void roff(concptr str);
extern void clear_from(int row);
extern bool askfor_aux(char *buf, int len, bool numpad_cursor);
extern bool askfor(char *buf, int len);
extern bool get_string(concptr prompt, char *buf, int len);
extern bool get_check(concptr prompt);
extern bool get_check_strict(concptr prompt, BIT_FLAGS mode);
extern bool get_com(concptr prompt, char *command, bool z_escape);
extern QUANTITY get_quantity(concptr prompt, QUANTITY max);
extern void pause_line(int row);
extern void request_command(int shopping);
extern bool is_a_vowel(int ch);
extern int get_keymap_dir(char ch);
extern errr type_string(concptr str, uint len);
extern void roff_to_buf(concptr str, int wlen, char *tbuf, size_t bufsize);

#ifdef SORT_R_INFO
extern void tag_sort(tag_type elements[], int number);
#endif /* SORT_R_INFO */

#ifdef SUPPORT_GAMMA
extern byte gamma_table[256];
extern void build_gamma_table(int gamma);
#endif /* SUPPORT_GAMMA */

extern size_t my_strcpy(char *buf, concptr src, size_t bufsize);
extern size_t my_strcat(char *buf, concptr src, size_t bufsize);
extern char *my_strstr(concptr haystack, concptr needle);
extern char *my_strchr(concptr ptr, char ch);
extern void str_tolower(char *str);
extern int inkey_special(bool numpad_cursor);
