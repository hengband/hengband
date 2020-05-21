#include "combat/combat-options-type.h"

extern HIT_POINT tot_dam_aux(player_type *attacker_ptr, object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, combat_options mode, bool thrown);
extern HIT_POINT critical_norm(player_type *attacker_ptr, WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, combat_options mode);
extern bool do_cmd_attack(player_type *attacker_ptr, POSITION y, POSITION x, combat_options mode);
extern bool make_attack_normal(player_type *targer_ptr, MONSTER_IDX m_idx);
extern void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who);
extern bool monst_attack_monst(player_type *subject_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
