#pragma once

enum blue_magic_type : int {
	MONSPELL_TYPE_BOLT = 1,
    MONSPELL_TYPE_BALL = 2,
    MONSPELL_TYPE_BREATH = 3,
    MONSPELL_TYPE_SUMMON = 4,
    MONSPELL_TYPE_OTHER = 5,
};

struct player_type;
bool do_cmd_cast_learned(player_type *player_ptr);
