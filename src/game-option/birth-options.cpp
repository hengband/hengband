#include "game-option/birth-options.h"

bool smart_learn; /* Monsters learn from their mistakes (*) */
bool smart_cheat; /* Monsters exploit players weaknesses (*) */
bool vanilla_town; /* Use 'vanilla' town without quests and wilderness */
bool lite_town; /* Use 'lite' town without a wilderness */
bool ironman_shops; /* Stores are permanently closed (*) */
bool ironman_small_levels; /* Always create unusually small dungeon levels (*) */
bool ironman_downward; /* Disable recall and use of up stairs (*) */
bool ironman_empty_levels; /* Always create empty 'on_defeat_arena_monster' levels (*) */
bool ironman_rooms; /* Always generate very unusual rooms (*) */
bool ironman_nightmare; /* Nightmare mode(it isn't even remotely fair!)(*) */
bool left_hander; /* Left-Hander */
bool preserve_mode; /* Preserve artifacts (*) */
bool autoroller; /* Allow use of autoroller for stats (*) */
bool autochara; /* Autoroll for weight, height and social status */
bool powerup_home; /* Increase capacity of your home (*) */
bool keep_savefile; //!< 同一のセーブファイルでゲームを開始する / Start game with same savefile thet is loaded
