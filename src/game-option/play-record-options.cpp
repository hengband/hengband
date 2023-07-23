#include "game-option/play-record-options.h"

bool record_fix_art; /* Record fixed artifacts */
bool record_rand_art; /* Record random artifacts */
bool record_destroy_uniq; /* Record when destroy unique monster */
bool record_fix_quest; /* Record fixed quests */
bool record_rand_quest; /* Record random quests */
bool record_maxdepth; /* Record movements to deepest level */
bool record_stair; /* Record recall and stair movements */
bool record_buy; /* Record purchased items */
bool record_sell; /* Record sold items */
bool record_danger; /* Record hitpoint warning */
bool record_arena; /* Record on_defeat_arena_monster victories */
bool record_ident; /* Record first identified items */
bool record_named_pet; /* Record information about named pets */
char record_o_name[MAX_NLEN];
GAME_TURN record_turn;
