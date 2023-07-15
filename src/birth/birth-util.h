#pragma once

#include "system/angband.h"

enum class BirthKind {
    REALM,
    RACE,
    CLASS,
    PERSONALITY,
    AUTO_ROLLER,
};

class PlayerType;
void birth_quit(void);
void show_help(PlayerType *player_ptr, concptr helpfile);
void birth_help_option(PlayerType *player_ptr, char c, BirthKind bk);
