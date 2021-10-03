#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "object-enchant/tr-flags.h"

typedef struct monap_type monap_type;
struct monster_type;
struct player_attack_type;
struct player_type;
MULTIPLY mult_hissatsu(player_type *player_ptr, MULTIPLY mult, const TrFlags &flgs, monster_type *m_ptr, combat_options mode);
void concentration(player_type *player_ptr);
bool choose_samurai_stance(player_type* player_ptr);
int calc_attack_quality(player_type *player_ptr, player_attack_type *pa_ptr);
void mineuchi(player_type *player_ptr, player_attack_type *pa_ptr);
void musou_counterattack(player_type *player_ptr, monap_type *monap_ptr);
