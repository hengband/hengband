#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

typedef struct effect_player_type
{
	DEPTH rlev; // モンスターのレベル (但し0のモンスターは1になる).
	monster_type *m_ptr;
	char killer[80];
	GAME_TEXT m_name[MAX_NLEN];
	int get_damage;

	MONSTER_IDX who;
	HIT_POINT dam;
	spell_type effect_type;
	BIT_FLAGS flag;
	int monspell;
} effect_player_type;
