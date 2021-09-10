#pragma once

#include "system/angband.h"

/* 職業ごとの選択可能な魔法領域 / Possible realms that can be chosen. */
enum choosable_realm {
	CH_NONE = 0x00,
	CH_LIFE = 0x01,
	CH_SORCERY = 0x02,
	CH_NATURE = 0x04,
	CH_CHAOS = 0x08,
	CH_DEATH = 0x10,
	CH_TRUMP = 0x20,
	CH_ARCANE = 0x40,
	CH_ENCHANT = 0x80,
	CH_DAEMON = 0x100,
	CH_CRUSADE = 0x200,
	
	CH_MUSIC = 0x8000,
    CH_HISSATSU = 0x10000,
    CH_HEX = 0x20000,
};

extern const uint32_t realm_choices1[];
extern const uint32_t realm_choices2[];

struct player_type;
int16_t get_realm1_book(player_type *player_ptr);
int16_t get_realm2_book(player_type *player_ptr);
