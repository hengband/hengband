#pragma once

#include "system/angband.h"
#include <map>
#include <string>

enum class Virtue : short {
    NONE = 0,
    COMPASSION = 1,
    HONOUR = 2,
    JUSTICE = 3,
    SACRIFICE = 4,
    KNOWLEDGE = 5,
    FAITH = 6,
    ENLIGHTEN = 7,
    ENCHANT = 8,
    CHANCE = 9,
    NATURE = 10,
    HARMONY = 11,
    VITALITY = 12,
    UNLIFE = 13,
    PATIENCE = 14,
    TEMPERANCE = 15,
    DILIGENCE = 16,
    VALOUR = 17,
    INDIVIDUALISM = 18,
    MAX,
};

class PlayerType;
extern const std::map<Virtue, std::string> virtue_names;
bool compare_virtue(PlayerType *player_ptr, Virtue virtue, int threshold);
int virtue_number(PlayerType *player_ptr, Virtue virtue);
void initialize_virtues(PlayerType *player_ptr);
void chg_virtue(PlayerType *player_ptr, Virtue virtue, int amount);
void set_virtue(PlayerType *player_ptr, Virtue virtue, int amount);
void dump_virtues(PlayerType *player_ptr, FILE *out_file);
