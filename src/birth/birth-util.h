﻿#pragma once

#include "system/angband.h"

enum birth_kind
{
	BK_REALM,
	BK_RACE,
	BK_CLASS,
	BK_PERSONALITY,
	BK_AUTO_ROLLER,
};

void birth_quit(void);
void show_help(player_type* creature_ptr, concptr helpfile);
void birth_help_option(player_type *creature_ptr, char c, birth_kind bk);
