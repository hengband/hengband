#pragma once

/*
 * todo 双方向の依存性を招いている原因の一端なので、いずれ抹殺する
 * 但しget_aim_dir() に入れ込む必要がありとてつもない分量の変更が入る
 * 後ほど実施する
 */
extern int init_flags;

/*
 * todo ここにいるべきではない。files.c 辺りか？
 */
extern concptr ANGBAND_SYS;
extern concptr ANGBAND_KEYBOARD;
extern concptr ANGBAND_GRAF;

extern void play_game(player_type *player_ptr, bool new_game);
