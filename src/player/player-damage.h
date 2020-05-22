#pragma once

/* Multishadow effects is determined by turn */
#define CHECK_MULTISHADOW(CRE_PTR) ((CRE_PTR)->multishadow && (current_world_ptr->game_turn & 1))

#define DAMAGE_FORCE    1
#define DAMAGE_GENO     2
#define DAMAGE_LOSELIFE 3
#define DAMAGE_ATTACK   4
#define DAMAGE_NOESCAPE 5
#define DAMAGE_USELIFE  6
extern int take_hit(player_type *creature_ptr, int damage_type, HIT_POINT damage, concptr kb_str, int monspell);

/*
 * This seems like a pretty standard "typedef"
 */
typedef int(*inven_func)(object_type *);

extern void inventory_damage(player_type *creature_ptr, inven_func typ, int perc);
extern HIT_POINT acid_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura);
extern HIT_POINT elec_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura);
extern HIT_POINT fire_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura);
extern HIT_POINT cold_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura);
void touch_zap_player(monster_type *m_ptr, player_type *touched_ptr);
