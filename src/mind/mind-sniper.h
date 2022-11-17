#pragma once

#include "system/angband.h"

class MonsterEntity;
class PlayerType;
void reset_concentration(PlayerType *player_ptr, bool msg);
void display_snipe_list(PlayerType *player_ptr);
MULTIPLY calc_snipe_damage_with_slay(PlayerType *player_ptr, MULTIPLY mult, MonsterEntity *m_ptr, SPELL_IDX snipe_type);
void do_cmd_snipe(PlayerType *player_ptr);
void do_cmd_snipe_browse(PlayerType *player_ptr);
int boost_concentration_damage(PlayerType *player_ptr, int tdam);
