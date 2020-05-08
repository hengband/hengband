#pragma once
#include "realm/realm.h"

/* spells1.c */
extern bool in_disintegration_range(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern void breath_shape(player_type *caster_ptr, u16b *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ);
extern POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern bool binding_field(player_type *caster_ptr, HIT_POINT dam);
extern void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam);
extern concptr spell_category_name(OBJECT_TYPE_VALUE tval);

/* spells3.c */
extern bool teleport_away(player_type *caster_ptr, MONSTER_IDX m_idx, POSITION dis, teleport_flags mode);
extern void teleport_monster_to(player_type *caster_ptr, MONSTER_IDX m_idx, POSITION ty, POSITION tx, int power, teleport_flags mode);
extern bool teleport_player_aux(player_type *creature_ptr, POSITION dis, bool is_quantum_effect, teleport_flags mode);
extern void teleport_player(player_type *creature_ptr, POSITION dis, BIT_FLAGS mode);
extern void teleport_player_away(MONSTER_IDX m_idx, player_type *target_ptr, POSITION dis, bool is_quantum_effect);
extern void teleport_player_to(player_type *creature_ptr, POSITION ny, POSITION nx, teleport_flags mode);
extern void teleport_away_followable(player_type *creature_ptr, MONSTER_IDX m_idx);
extern bool teleport_level_other(player_type *caster_ptr);
extern void teleport_level(player_type *creature_ptr, MONSTER_IDX m_idx);
extern bool recall_player(player_type *creature_ptr, TIME_EFFECT turns);
extern bool free_level_recall(player_type *creature_ptr);
extern bool reset_recall(player_type *caster_ptr);
extern bool apply_disenchant(player_type *target_ptr, BIT_FLAGS mode);
extern void call_the_void(player_type *caster_ptr);
extern void fetch(player_type *caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los);
extern void reserve_alter_reality(player_type *caster_ptr);
extern void identify_pack(player_type *target_ptr);
extern int remove_curse(player_type *caster_ptr);
extern int remove_all_curse(player_type *caster_ptr);
extern bool alchemy(player_type *caster_ptr);

extern bool artifact_scroll(player_type *caster_ptr);
extern bool ident_spell(player_type *caster_ptr, bool only_equip, OBJECT_TYPE_VALUE item_tester_tval);
extern bool mundane_spell(player_type *ownner_ptr, bool only_equip);
extern bool identify_item(player_type *owner_ptr, object_type *o_ptr);
extern bool identify_fully(player_type *caster_ptr, bool only_equip, OBJECT_TYPE_VALUE item_tester_tval);
extern bool recharge(player_type *caster_ptr, int power);
extern void display_spell_list(player_type *caster_ptr);
extern EXP experience_of_spell(player_type *caster_ptr, SPELL_IDX spell, REALM_IDX use_realm);
extern MANA_POINT mod_need_mana(player_type *caster_ptr, MANA_POINT need_mana, SPELL_IDX spell, REALM_IDX realm);
extern PERCENTAGE mod_spell_chance_1(player_type *caster_ptr, PERCENTAGE chance);
extern PERCENTAGE mod_spell_chance_2(player_type *caster_ptr, PERCENTAGE chance);
extern PERCENTAGE spell_chance(player_type *caster_ptr, SPELL_IDX spell, REALM_IDX realm);
extern void print_spells(player_type* caster_ptr, SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, REALM_IDX realm);
extern bool polymorph_monster(player_type *caster_ptr, POSITION y, POSITION x);
extern bool dimension_door(player_type *caster_ptr);
extern bool mirror_tunnel(player_type *caster_ptr);
extern void massacre(player_type *caster_ptr);
extern bool eat_rock(player_type *caster_ptr);
extern bool shock_power(player_type *caster_ptr);
extern bool fetch_monster(player_type *caster_ptr);
extern bool booze(player_type *creature_ptr);
extern bool detonation(player_type *creature_ptr);
extern void blood_curse_to_enemy(player_type *caster_ptr, MONSTER_IDX m_idx);
extern bool fire_crimson(player_type *shooter_ptr);
extern bool tele_town(player_type *caster_ptr);
extern bool is_teleport_level_ineffective(player_type *caster_ptr, MONSTER_IDX idx);
