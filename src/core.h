#pragma once

extern const concptr copyright[5];

extern concptr ANGBAND_SYS;
extern concptr ANGBAND_KEYBOARD;
extern concptr ANGBAND_GRAF;

extern void play_game(bool new_game);
extern s32b turn_real(s32b hoge);
extern void prevent_turn_overflow(void);
extern void close_game(void);