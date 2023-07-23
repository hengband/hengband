#pragma once

#include "system/angband.h"

extern bool rogue_like_commands; /* Rogue-like commands */
extern bool always_pickup; /* Pick things up by default */
extern bool carry_query_flag; /* Prompt before picking things up */
extern bool quick_messages; /* Activate quick messages */
extern bool auto_more; /* Automatically clear '-more-' prompts */
extern bool skip_more; /* Automatically skip '-more-' prompts every time */
extern bool command_menu; /* Enable command selection menu */
extern bool other_query_flag; /* Prompt for floor item selection */
extern bool use_old_target; /* Use old target by default */
extern bool always_repeat; /* Repeat obvious commands */
extern bool confirm_destroy; /* Prompt for destruction of known worthless items */
extern bool confirm_wear; /* Confirm to wear/wield known cursed items */
extern bool confirm_quest; /* Prompt before exiting a quest level */
extern bool target_pet; /* Allow targetting pets */
extern bool easy_open; /* Automatically open doors */
extern bool easy_disarm; /* Automatically disarm traps */
extern bool easy_floor; /* Display floor stacks in a list */
extern bool use_command; /* Allow unified use command */
extern bool over_exert; /* Allow casting spells when short of mana */
extern bool numpad_as_cursorkey; /* Use numpad keys as cursor key in editor mode */
