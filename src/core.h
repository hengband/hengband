#pragma once

extern const concptr copyright[5];


/*
 * Initialization flags
 */
#define INIT_NAME_ONLY          0x01
#define INIT_SHOW_TEXT          0x02
#define INIT_ASSIGN             0x04
#define INIT_CREATE_DUNGEON     0x08
#define INIT_ONLY_FEATURES      0x10
#define INIT_ONLY_BUILDINGS     0x20
extern int init_flags;

extern concptr ANGBAND_SYS;
extern concptr ANGBAND_KEYBOARD;
extern concptr ANGBAND_GRAF;

extern bool can_save;
extern COMMAND_CODE now_message;

extern void play_game(bool new_game);
extern void update_playtime(void);
extern s32b turn_real(s32b hoge);
extern void prevent_turn_overflow(void);
extern void close_game(void);
