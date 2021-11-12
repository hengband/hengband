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

class PlayerType;
void birth_quit(void);
void show_help(PlayerType* player_ptr, concptr helpfile);
void birth_help_option(PlayerType *player_ptr, char c, birth_kind bk);
