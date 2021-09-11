#pragma once

#include "system/angband.h"

enum birth_kind
{
	BK_REALM,
	BK_RACE,
	BK_CLASS,
	BK_PERSONALITY,
	BK_AUTO_ROLLER,
};

struct player_type;
void birth_quit(void);
void show_help(player_type* player_ptr, concptr helpfile);
void birth_help_option(player_type *player_ptr, char c, birth_kind bk);
