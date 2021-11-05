﻿#pragma once

#include "system/angband.h"

class PlayerType;
typedef struct quest_type quest_type;
void load_zangband_options(void);
void set_zangband_realm(PlayerType *player_ptr);
void set_zangband_skill(PlayerType *player_ptr);
void set_zangband_race(PlayerType *player_ptr);
void set_zangband_bounty_uniques(PlayerType *player_ptr);
void set_zangband_mimic(PlayerType *player_ptr);
void set_zangband_holy_aura(PlayerType *player_ptr);
void set_zangband_reflection(PlayerType *player_ptr);
void rd_zangband_dungeon(void);
void set_zangband_game_turns(PlayerType *player_ptr);
void set_zangband_gambling_monsters(int i);
void set_zangband_special_attack(PlayerType *player_ptr);
void set_zangband_special_defense(PlayerType *player_ptr);
void set_zangband_action(PlayerType *player_ptr);
void set_zangband_visited_towns(PlayerType *player_ptr);
void set_zangband_quest(PlayerType *player_ptr, quest_type *const q_ptr, int loading_quest_index, const QUEST_IDX old_inside_quest);
void set_zangband_class(PlayerType *player_ptr);
void set_zangband_learnt_spells(PlayerType *player_ptr);
void set_zangband_pet(PlayerType *player_ptr);
