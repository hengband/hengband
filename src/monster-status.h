
extern HIT_POINT mon_damage_mod(monster_type *m_ptr, HIT_POINT dam, bool is_psy_spear);
extern bool mon_take_hit(MONSTER_IDX m_idx, HIT_POINT dam, bool *fear, concptr note);
extern int get_mproc_idx(MONSTER_IDX m_idx, int mproc_type);

