/*!
 * @file realm/realm-hex.h
 */

#define hex_spelling_any(CREATURE_PTR) \
	(((CREATURE_PTR)->realm1 == REALM_HEX) && ((CREATURE_PTR)->magic_num1[0]))
#define CASTING_HEX_FLAGS(P_PTR) ((P_PTR)->magic_num1[0])
#define CASTING_HEX_NUM(P_PTR) ((P_PTR)->magic_num2[0])
#define HEX_REVENGE_POWER(P_PTR) ((P_PTR)->magic_num1[2])
#define HEX_REVENGE_TURN(P_PTR) ((P_PTR)->magic_num2[2])
#define HEX_REVENGE_TYPE(P_PTR) ((P_PTR)->magic_num2[1])

/* 1st book */
#define HEX_BLESS             0
#define HEX_CURE_LIGHT        1
#define HEX_DEMON_AURA        2
#define HEX_STINKING_MIST     3
#define HEX_XTRA_MIGHT        4
#define HEX_CURSE_WEAPON      5
#define HEX_DETECT_EVIL       6
#define HEX_PATIENCE          7
/* 2nd book */
#define HEX_ICE_ARMOR         8
#define HEX_CURE_SERIOUS      9
#define HEX_INHAIL           10
#define HEX_VAMP_MIST        11
#define HEX_RUNESWORD        12
#define HEX_CONFUSION        13
#define HEX_BUILDING         14
#define HEX_ANTI_TELE        15
/* 3rd book */
#define HEX_SHOCK_CLOAK      16
#define HEX_CURE_CRITICAL    17
#define HEX_RECHARGE         18
#define HEX_RAISE_DEAD       19
#define HEX_CURSE_ARMOUR     20
#define HEX_SHADOW_CLOAK     21
#define HEX_PAIN_TO_MANA     22
#define HEX_EYE_FOR_EYE      23
/* 4th book */
#define HEX_ANTI_MULTI       24
#define HEX_RESTORE          25
#define HEX_DRAIN_CURSE      26
#define HEX_VAMP_BLADE       27
#define HEX_STUN_MONSTERS    28
#define HEX_SHADOW_MOVE      29
#define HEX_ANTI_MAGIC       30
#define HEX_REVENGE          31

extern bool stop_hex_spell_all(player_type *caster_ptr);
extern bool stop_hex_spell(player_type *caster_ptr);
extern void check_hex(player_type *caster_ptr);
extern bool hex_spell_fully(player_type *caster_ptr);
extern void revenge_spell(player_type *caster_ptr);
extern void revenge_store(player_type *caster_ptr, HIT_POINT dam);
extern bool teleport_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
extern bool magic_barrier(player_type *target_ptr, MONSTER_IDX m_idx);
extern bool multiply_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
extern concptr do_hex_spell(player_type *caster_ptr, SPELL_IDX spell, BIT_FLAGS mode);
extern bool hex_spelling(player_type *caster_type, int hex);
