#pragma once

#include "system/monster-type-definition.h"

/* Imitator */
typedef struct monster_power {
    PLAYER_LEVEL level;
    MANA_POINT smana;
    PERCENTAGE fail;
    int manedam;
    int manefail;
    int use_stat;
    concptr name;
} monster_power;

/* Spell Type flag */
#define MONSTER_TO_PLAYER 0x01
#define MONSTER_TO_MONSTER 0x02

/* monster spell number */
#define RF4_SPELL_START 32 * 3
#define RF5_SPELL_START 32 * 4
#define RF6_SPELL_START 32 * 5

#define MAX_MONSPELLS 96

extern const monster_power monster_powers[MAX_MONSPELLS];
extern const concptr monster_powers_short[MAX_MONSPELLS];

/* mspells1.c */
extern bool clean_shot(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
extern bool summon_possible(player_type *target_ptr, POSITION y1, POSITION x1);
extern bool raise_possible(player_type *target_ptr, monster_type *m_ptr);
extern bool dispel_check(player_type *creature_ptr, MONSTER_IDX m_idx);
extern bool spell_is_inate(SPELL_IDX spell);
extern bool make_attack_spell(MONSTER_IDX m_idx, player_type *target_ptr);
extern void beam(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type);
extern void bolt(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type);
extern void breath(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, EFFECT_ID typ, int dam_hp, POSITION rad, bool breath, int monspell, int target_type);

/* mspells2.c */
extern void get_project_point(player_type *target_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg);
extern bool monst_spell_monst(player_type *target_ptr, MONSTER_IDX m_idx);

/* mspells3.c */
extern bool do_cmd_cast_learned(player_type *caster_ptr);
extern void learn_spell(player_type *learner_ptr, int monspell);
extern void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, BIT_FLAGS mode);
