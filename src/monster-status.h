#pragma once

extern bool monster_is_powerful(floor_type *floor_ptr, MONSTER_IDX m_idx);
extern DEPTH monster_level_idx(floor_type *floor_ptr, MONSTER_IDX m_idx);

extern HIT_POINT mon_damage_mod(player_type *target_ptr, monster_type *m_ptr, HIT_POINT dam, bool is_psy_spear);
extern bool mon_take_hit(player_type *target_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *fear, concptr note);
extern int get_mproc_idx(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type);
extern bool monster_is_valid(monster_type *m_ptr);

extern bool set_monster_csleep(player_type *target_ptr, MONSTER_IDX m_idx, int v);
extern bool set_monster_fast(player_type *target_ptr, MONSTER_IDX m_idx, int v);
extern bool set_monster_slow(player_type *target_ptr, MONSTER_IDX m_idx, int v);
extern bool set_monster_stunned(player_type *target_ptr, MONSTER_IDX m_idx, int v);
extern bool set_monster_confused(player_type *target_ptr, MONSTER_IDX m_idx, int v);
extern bool set_monster_monfear(player_type *target_ptr, MONSTER_IDX m_idx, int v);
extern bool set_monster_invulner(player_type *target_ptr, MONSTER_IDX m_idx, int v, bool energy_need);
extern bool set_monster_timewalk(player_type *target_ptr, int num, MONSTER_IDX who, bool vs_player);

extern void dispel_monster_status(player_type *target_ptr, MONSTER_IDX m_idx);
extern void monster_gain_exp(MONSTER_IDX m_idx, MONRACE_IDX s_idx);

extern int get_mproc_idx(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type);
extern void mproc_init(floor_type *floor_ptr);
extern void process_monsters_mtimed(player_type *target_ptr, int mtimed_idx);
