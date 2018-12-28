extern bool test_hit_norm(HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible);
extern PERCENTAGE hit_chance(HIT_RELIABILITY chance, ARMOUR_CLASS ac);
extern HIT_POINT critical_norm(WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, BIT_FLAGS mode);
extern bool py_attack(POSITION y, POSITION x, BIT_FLAGS mode);
extern bool make_attack_normal(MONSTER_IDX m_idx);

