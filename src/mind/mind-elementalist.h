#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

enum class ElementRealmType {
    FIRE = 1,
    ICE = 2,
    SKY = 3,
    SEA = 4,
    DARKNESS = 5,
    CHAOS = 6,
    EARTH = 7,
    DEATH = 8,
    MAX
};

class PlayerType;
class EffectMonster;
struct rc_type;

concptr get_element_title(int realm_idx);
AttributeType get_element_type(int realm_idx, int n);
concptr get_element_name(int realm_idx, int n);
void do_cmd_element(PlayerType *player_ptr);
void do_cmd_element_browse(PlayerType *player_ptr);
ProcessResult effect_monster_elemental_genocide(PlayerType *player_ptr, EffectMonster *em_ptr);
bool has_element_resist(PlayerType *player_ptr, ElementRealmType realm, PLAYER_LEVEL lev);
byte select_element_realm(PlayerType *player_ptr);
void switch_element_racial(PlayerType *player_ptr, rc_type *rc_ptr);
bool switch_element_execution(PlayerType *player_ptr);
