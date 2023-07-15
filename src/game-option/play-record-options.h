#pragma once

#include "system/angband.h"

extern bool record_fix_art; /* Record fixed artifacts */
extern bool record_rand_art; /* Record random artifacts */
extern bool record_destroy_uniq; /* Record when destroy unique monster */
extern bool record_fix_quest; /* Record fixed quests */
extern bool record_rand_quest; /* Record random quests */
extern bool record_maxdepth; /* Record movements to deepest level */
extern bool record_stair; /* Record recall and stair movements */
extern bool record_buy; /* Record purchased items */
extern bool record_sell; /* Record sold items */
extern bool record_danger; /* Record hitpoint warning */
extern bool record_arena; /* Record on_defeat_arena_monster victories */
extern bool record_ident; /* Record first identified items */
extern bool record_named_pet; /* Record information about named pets */

extern char record_o_name[MAX_NLEN];
extern GAME_TURN record_turn;
