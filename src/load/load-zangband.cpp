#include "load/load-zangband.h"
#include "avatar/avatar.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "game-option/option-flags.h"
#include "info-reader/fixed-map-parser.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "market/bounty.h"
#include "monster-race/monster-race.h"
#include "pet/pet-util.h"
#include "player/attack-defense-types.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race.h"
#include "player/player-skill.h"
#include "realm/realm-types.h"
#include "spell/spells-status.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "world/world.h"

void load_zangband_options(void)
{
    if (option_flag[5] & (0x00000001U << 4))
        option_flag[5] &= ~(0x00000001U << 4);
    else
        option_flag[5] |= (0x00000001U << 4);

    if (option_flag[2] & (0x00000001U << 5))
        option_flag[2] &= ~(0x00000001U << 5);
    else
        option_flag[2] |= (0x00000001U << 5);

    if (option_flag[4] & (0x00000001U << 5))
        option_flag[4] &= ~(0x00000001U << 5);
    else
        option_flag[4] |= (0x00000001U << 5);

    if (option_flag[5] & (0x00000001U << 0))
        option_flag[5] &= ~(0x00000001U << 0);
    else
        option_flag[5] |= (0x00000001U << 0);

    if (option_flag[5] & (0x00000001U << 12))
        option_flag[5] &= ~(0x00000001U << 12);
    else
        option_flag[5] |= (0x00000001U << 12);

    if (option_flag[1] & (0x00000001U << 0))
        option_flag[1] &= ~(0x00000001U << 0);
    else
        option_flag[1] |= (0x00000001U << 0);

    if (option_flag[1] & (0x00000001U << 18))
        option_flag[1] &= ~(0x00000001U << 18);
    else
        option_flag[1] |= (0x00000001U << 18);

    if (option_flag[1] & (0x00000001U << 19))
        option_flag[1] &= ~(0x00000001U << 19);
    else
        option_flag[1] |= (0x00000001U << 19);

    if (option_flag[5] & (0x00000001U << 3))
        option_flag[1] &= ~(0x00000001U << 3);
    else
        option_flag[5] |= (0x00000001U << 3);
}

void set_zangband_realm(player_type *creature_ptr)
{
    if (creature_ptr->realm1 == 9)
        creature_ptr->realm1 = REALM_MUSIC;

    if (creature_ptr->realm2 == 9)
        creature_ptr->realm2 = REALM_MUSIC;

    if (creature_ptr->realm1 == 10)
        creature_ptr->realm1 = REALM_HISSATSU;

    if (creature_ptr->realm2 == 10)
        creature_ptr->realm2 = REALM_HISSATSU;
}

void set_zangband_skill(player_type *creature_ptr)
{
    if (creature_ptr->pclass != CLASS_BEASTMASTER)
        creature_ptr->skill_exp[SKILL_RIDING] /= 2;

    creature_ptr->skill_exp[SKILL_RIDING] = MIN(creature_ptr->skill_exp[SKILL_RIDING], s_info[creature_ptr->pclass].s_max[SKILL_RIDING]);
}

void set_zangband_spells(player_type *creature_ptr)
{
    for (int i = 0; i < MAX_SPELLS; i++)
        creature_ptr->magic_num1[i] = 0;

    for (int i = 0; i < MAX_SPELLS; i++)
        creature_ptr->magic_num2[i] = 0;
}

void set_zangband_race(player_type *creature_ptr)
{
    creature_ptr->start_race = creature_ptr->prace;
    creature_ptr->old_race1 = 0L;
    creature_ptr->old_race2 = 0L;
    creature_ptr->old_realm = 0;
}

void set_zangband_bounty_uniques(player_type *creature_ptr)
{
    determine_bounty_uniques(creature_ptr);
    for (int i = 0; i < MAX_BOUNTY; i++) {
        /* Is this bounty unique already dead? */
        if (!r_info[current_world_ptr->bounty_r_idx[i]].max_num)
            current_world_ptr->bounty_r_idx[i] += 10000;
    }
}

void set_zangband_mimic(player_type *creature_ptr)
{
    creature_ptr->tim_res_time = 0;
    creature_ptr->mimic_form = 0;
    creature_ptr->tim_mimic = 0;
    creature_ptr->tim_sh_fire = 0;
}

void set_zangband_holy_aura(player_type *creature_ptr)
{
    creature_ptr->tim_sh_holy = 0;
    creature_ptr->tim_eyeeye = 0;
}

void set_zangband_reflection(player_type *creature_ptr)
{
    creature_ptr->tim_reflect = 0;
    creature_ptr->multishadow = 0;
    creature_ptr->dustrobe = 0;
}

void rd_zangband_dungeon()
{
    int16_t tmp16s;
    rd_s16b(&tmp16s);
    max_dlv[DUNGEON_ANGBAND] = tmp16s;
}

void set_zangband_game_turns(player_type *creature_ptr)
{
    creature_ptr->current_floor_ptr->generated_turn /= 2;
    creature_ptr->feeling_turn /= 2;
    current_world_ptr->game_turn /= 2;
    current_world_ptr->dungeon_turn /= 2;
}

void set_zangband_gambling_monsters(int i)
{
    int16_t tmp16s;
    rd_s16b(&tmp16s);
    mon_odds[i] = tmp16s;
}

void set_zangband_special_attack(player_type *creature_ptr)
{
    byte tmp8u;
    rd_byte(&tmp8u);
    if (tmp8u)
        creature_ptr->special_attack = ATTACK_CONFUSE;

    creature_ptr->ele_attack = 0;
}

void set_zangband_special_defense(player_type *creature_ptr)
{
    creature_ptr->ele_immune = 0;
    creature_ptr->special_defense = 0;
}

void set_zangband_action(player_type *creature_ptr)
{
    byte tmp8u;
    rd_byte(&tmp8u);
    if (tmp8u)
        creature_ptr->action = ACTION_LEARN;
}

void set_zangband_visited_towns(player_type *creature_ptr)
{
    int32_t tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->visit = 1L;
}

void set_zangband_quest(player_type *creature_ptr, quest_type *const q_ptr, int loading_quest_index, const QUEST_IDX old_inside_quest)
{
    if (q_ptr->flags & QUEST_FLAG_PRESET) {
        q_ptr->dungeon = 0;
        return;
    }

    init_flags = INIT_ASSIGN;
    creature_ptr->current_floor_ptr->inside_quest = (QUEST_IDX)loading_quest_index;
    parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    creature_ptr->current_floor_ptr->inside_quest = old_inside_quest;
}

void set_zangband_class(player_type *creature_ptr)
{
    if (h_older_than(0, 2, 2) && (creature_ptr->pclass == CLASS_BEASTMASTER) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, SPOP_NONE);
    }

    if (h_older_than(0, 3, 2) && (creature_ptr->pclass == CLASS_ARCHER) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, SPOP_NONE);
    }

    if (h_older_than(0, 2, 6) && (creature_ptr->pclass == CLASS_SORCERER) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, SPOP_NONE);
    }

    if (h_older_than(0, 4, 7) && (creature_ptr->pclass == CLASS_BLUE_MAGE) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, SPOP_NONE);
    }
}

void set_zangband_learnt_spells(player_type *creature_ptr)
{
    creature_ptr->learned_spells = 0;
    for (int i = 0; i < 64; i++)
        if ((i < 32) ? (creature_ptr->spell_learned1 & (1UL << i)) : (creature_ptr->spell_learned2 & (1UL << (i - 32))))
            creature_ptr->learned_spells++;
}

void set_zangband_pet(player_type *creature_ptr)
{
    creature_ptr->pet_extra_flags = 0;
    byte tmp8u;
    rd_byte(&tmp8u);
    if (tmp8u)
        creature_ptr->pet_extra_flags |= PF_OPEN_DOORS;

    rd_byte(&tmp8u);
    if (tmp8u)
        creature_ptr->pet_extra_flags |= PF_PICKUP_ITEMS;

    if (h_older_than(0, 0, 4))
        creature_ptr->pet_extra_flags |= PF_TELEPORT;
    else {
        rd_byte(&tmp8u);
        if (tmp8u)
            creature_ptr->pet_extra_flags |= PF_TELEPORT;
    }

    if (h_older_than(0, 0, 7))
        creature_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
    else {
        rd_byte(&tmp8u);
        if (tmp8u)
            creature_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
    }

    if (h_older_than(0, 0, 8))
        creature_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
    else {
        rd_byte(&tmp8u);
        if (tmp8u)
            creature_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
    }

    if (h_older_than(0, 0, 8))
        return;

    rd_byte(&tmp8u);
    if (tmp8u)
        creature_ptr->pet_extra_flags |= PF_BALL_SPELL;
}
