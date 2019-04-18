#pragma once

extern HIT_POINT mon_damage_mod(monster_type *m_ptr, HIT_POINT dam, bool is_psy_spear);
extern bool mon_take_hit(MONSTER_IDX m_idx, HIT_POINT dam, bool *fear, concptr note);
extern int get_mproc_idx(MONSTER_IDX m_idx, int mproc_type);
extern bool monster_is_valid(monster_type *m_ptr);

extern bool set_monster_csleep(MONSTER_IDX m_idx, int v);
extern bool set_monster_fast(MONSTER_IDX m_idx, int v);
extern bool set_monster_slow(MONSTER_IDX m_idx, int v);
extern bool set_monster_stunned(MONSTER_IDX m_idx, int v);
extern bool set_monster_confused(MONSTER_IDX m_idx, int v);
extern bool set_monster_monfear(MONSTER_IDX m_idx, int v);
extern bool set_monster_invulner(MONSTER_IDX m_idx, int v, bool energy_need);
