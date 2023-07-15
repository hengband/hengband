#pragma once

#include "system/angband.h"

extern bool smart_learn; /* Monsters learn from their mistakes (*) */
extern bool smart_cheat; /* Monsters exploit players weaknesses (*) */
extern bool vanilla_town; /* Use 'vanilla' town without quests and wilderness */
extern bool lite_town; /* Use 'lite' town without a wilderness */
extern bool ironman_shops; /* Stores are permanently closed (*) */
extern bool ironman_small_levels; /* Always create unusually small dungeon levels (*) */
extern bool ironman_downward; /* Disable recall and use of up stairs (*) */
extern bool ironman_empty_levels; /* Always create empty 'on_defeat_arena_monster' levels (*) */
extern bool ironman_rooms; /* Always generate very unusual rooms (*) */
extern bool ironman_nightmare; /* Nightmare mode(it isn't even remotely fair!)(*) */
extern bool left_hander; /* Left-Hander */
extern bool preserve_mode; /* Preserve artifacts (*) */
extern bool autoroller; /* Allow use of autoroller for stats (*) */
extern bool autochara; /* Autoroll for weight, height and social status */
extern bool powerup_home; /* Increase capacity of your home (*) */
extern bool keep_savefile; //!< 同一のセーブファイルでゲームを開始する / Start game with same savefile thet is loaded
