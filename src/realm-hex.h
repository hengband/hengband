/*!
 * @file realm-hex.h
 */

extern bool stop_hex_spell_all(void);
extern bool stop_hex_spell(void);
extern void check_hex(void);
extern bool hex_spell_fully(void);
extern void revenge_spell(void);
extern void revenge_store(HIT_POINT dam);
extern bool teleport_barrier(MONSTER_IDX m_idx);
extern bool magic_barrier(MONSTER_IDX m_idx);
extern bool multiply_barrier(MONSTER_IDX m_idx);
extern concptr do_hex_spell(SPELL_IDX spell, BIT_FLAGS mode);
