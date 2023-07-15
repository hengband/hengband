#include "game-option/input-options.h"

bool rogue_like_commands; /* Rogue-like commands */
bool always_pickup; /* Pick things up by default */
bool carry_query_flag; /* Prompt before picking things up */
bool quick_messages; /* Activate quick messages */
bool auto_more; /* Automatically clear '-more-' prompts */
bool skip_more; /* Automatically skip '-more-' prompts every time */
bool command_menu; /* Enable command selection menu */
bool other_query_flag; /* Prompt for floor item selection */
bool use_old_target; /* Use old target by default */
bool always_repeat; /* Repeat obvious commands */
bool confirm_destroy; /* Prompt for destruction of known worthless items */
bool confirm_wear; /* Confirm to wear/wield known cursed items */
bool confirm_quest; /* Prompt before exiting a quest level */
bool target_pet; /* Allow targeting pets */
bool easy_open; /* Automatically open doors */
bool easy_disarm; /* Automatically disarm traps */
bool easy_floor; /* Display floor stacks in a list */
bool use_command; /* Allow unified use command */
bool over_exert; /* Allow casting spells when short of mana */
bool numpad_as_cursorkey; /* Use numpad keys as cursor key in editor mode */
