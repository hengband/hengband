#pragma once

#include "system/angband.h"

#include "combat/combat-options-type.h"
#include "object-enchant/tr-flags.h"

class MonsterAttackPlayer;
class MonsterEntity;
struct player_attack_type;
class PlayerType;
MULTIPLY mult_hissatsu(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flags, MonsterEntity *m_ptr, combat_options mode);
void concentration(PlayerType *player_ptr);
bool choose_samurai_stance(PlayerType *player_ptr);
int calc_attack_quality(PlayerType *player_ptr, player_attack_type *pa_ptr);
void mineuchi(PlayerType *player_ptr, player_attack_type *pa_ptr);
void musou_counterattack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
