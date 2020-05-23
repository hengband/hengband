#include "combat/combat-options-type.h"

bool make_attack_normal(player_type *targer_ptr, MONSTER_IDX m_idx);
void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who);
bool monst_attack_monst(player_type *subject_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
