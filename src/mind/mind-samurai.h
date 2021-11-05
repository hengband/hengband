﻿#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "object-enchant/tr-flags.h"

typedef struct monap_type monap_type;
struct monster_type;
struct player_attack_type;
class PlayerType;
MULTIPLY mult_hissatsu(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr, combat_options mode);
void concentration(PlayerType *player_ptr);
bool choose_samurai_stance(PlayerType* player_ptr);
int calc_attack_quality(PlayerType *player_ptr, player_attack_type *pa_ptr);
void mineuchi(PlayerType *player_ptr, player_attack_type *pa_ptr);
void musou_counterattack(PlayerType *player_ptr, monap_type *monap_ptr);
