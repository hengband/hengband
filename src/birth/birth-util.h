#pragma once

#include <string_view>

enum class BirthKind {
    REALM,
    RACE,
    CLASS,
    PERSONALITY,
    AUTO_ROLLER,
};

class PlayerType;
void birth_quit();
void show_help(PlayerType *player_ptr, std::string_view helpfile);
void birth_help_option(PlayerType *player_ptr, char c, BirthKind bk);
