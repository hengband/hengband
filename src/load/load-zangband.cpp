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
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "player/patron.h"
#include "player/player-personality.h"
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

void set_zangband_realm(PlayerType *player_ptr)
{
    if (player_ptr->realm1 == 9)
        player_ptr->realm1 = REALM_MUSIC;

    if (player_ptr->realm2 == 9)
        player_ptr->realm2 = REALM_MUSIC;

    if (player_ptr->realm1 == 10)
        player_ptr->realm1 = REALM_HISSATSU;

    if (player_ptr->realm2 == 10)
        player_ptr->realm2 = REALM_HISSATSU;
}

void set_zangband_skill(PlayerType *player_ptr)
{
    if (!PlayerClass(player_ptr).equals(PlayerClassType::BEASTMASTER))
        player_ptr->skill_exp[PlayerSkillKindType::RIDING] /= 2;

    player_ptr->skill_exp[PlayerSkillKindType::RIDING] = std::min(player_ptr->skill_exp[PlayerSkillKindType::RIDING], s_info[enum2i(player_ptr->pclass)].s_max[PlayerSkillKindType::RIDING]);
}

void set_zangband_race(PlayerType *player_ptr)
{
    player_ptr->start_race = player_ptr->prace;
    player_ptr->old_race1 = 0L;
    player_ptr->old_race2 = 0L;
    player_ptr->old_realm = 0;
}

void set_zangband_bounty_uniques(PlayerType *player_ptr)
{
    determine_bounty_uniques(player_ptr);
    for (int i = 0; i < MAX_BOUNTY; i++) {
        /* Is this bounty unique already dead? */
        if (!r_info[w_ptr->bounty_r_idx[i]].max_num)
            w_ptr->bounty_r_idx[i] += 10000;
    }
}

void set_zangband_mimic(PlayerType *player_ptr)
{
    player_ptr->tim_res_time = 0;
    player_ptr->mimic_form = 0;
    player_ptr->tim_mimic = 0;
    player_ptr->tim_sh_fire = 0;
}

void set_zangband_holy_aura(PlayerType *player_ptr)
{
    player_ptr->tim_sh_holy = 0;
    player_ptr->tim_eyeeye = 0;
}

void set_zangband_reflection(PlayerType *player_ptr)
{
    player_ptr->tim_reflect = 0;
    player_ptr->multishadow = 0;
    player_ptr->dustrobe = 0;
}

void rd_zangband_dungeon()
{
    max_dlv[DUNGEON_ANGBAND] = rd_s16b();
}

void set_zangband_game_turns(PlayerType *player_ptr)
{
    player_ptr->current_floor_ptr->generated_turn /= 2;
    player_ptr->feeling_turn /= 2;
    w_ptr->game_turn /= 2;
    w_ptr->dungeon_turn /= 2;
}

void set_zangband_gambling_monsters(int i)
{
    mon_odds[i] = rd_s16b();
}

void set_zangband_special_attack(PlayerType *player_ptr)
{
    if (rd_byte() != 0)
        player_ptr->special_attack = ATTACK_CONFUSE;

    player_ptr->ele_attack = 0;
}

void set_zangband_special_defense(PlayerType *player_ptr)
{
    player_ptr->ele_immune = 0;
    player_ptr->special_defense = 0;
}

void set_zangband_action(PlayerType *player_ptr)
{
    if (rd_byte() != 0)
        player_ptr->action = ACTION_LEARN;
}

void set_zangband_visited_towns(PlayerType *player_ptr)
{
    strip_bytes(4);
    player_ptr->visit = 1L;
}

void set_zangband_quest(PlayerType *player_ptr, quest_type *const q_ptr, int loading_quest_index, const QuestId old_inside_quest)
{
    if (q_ptr->flags & QUEST_FLAG_PRESET) {
        q_ptr->dungeon = 0;
        return;
    }

    init_flags = INIT_ASSIGN;
    player_ptr->current_floor_ptr->quest_number = i2enum<QuestId>(loading_quest_index);
    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    player_ptr->current_floor_ptr->quest_number = old_inside_quest;
}

void set_zangband_class(PlayerType *player_ptr)
{
    PlayerClass pc(player_ptr);
    if (h_older_than(0, 2, 2) && pc.equals(PlayerClassType::BEASTMASTER) && !player_ptr->is_dead) {
        player_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(player_ptr, SPOP_NONE);
    }

    if (h_older_than(0, 3, 2) && pc.equals(PlayerClassType::ARCHER) && !player_ptr->is_dead) {
        player_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(player_ptr, SPOP_NONE);
    }

    if (h_older_than(0, 2, 6) && pc.equals(PlayerClassType::SORCERER) && !player_ptr->is_dead) {
        player_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(player_ptr, SPOP_NONE);
    }

    if (h_older_than(0, 4, 7) && pc.equals(PlayerClassType::BLUE_MAGE) && !player_ptr->is_dead) {
        player_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(player_ptr, SPOP_NONE);
    }
}

void set_zangband_learnt_spells(PlayerType *player_ptr)
{
    player_ptr->learned_spells = 0;
    for (int i = 0; i < 64; i++)
        if ((i < 32) ? (player_ptr->spell_learned1 & (1UL << i)) : (player_ptr->spell_learned2 & (1UL << (i - 32))))
            player_ptr->learned_spells++;
}

void set_zangband_pet(PlayerType *player_ptr)
{
    player_ptr->pet_extra_flags = 0;
    if (rd_byte() != 0)
        player_ptr->pet_extra_flags |= PF_OPEN_DOORS;

    if (rd_byte() != 0)
        player_ptr->pet_extra_flags |= PF_PICKUP_ITEMS;

    if (h_older_than(0, 0, 4))
        player_ptr->pet_extra_flags |= PF_TELEPORT;
    else {
        if (rd_byte() != 0)
            player_ptr->pet_extra_flags |= PF_TELEPORT;
    }

    if (h_older_than(0, 0, 7))
        player_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
    else {
        if (rd_byte() != 0)
            player_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
    }

    if (h_older_than(0, 0, 8))
        player_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
    else {
        if (rd_byte() != 0)
            player_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
    }

    if (h_older_than(0, 0, 8))
        return;

    if (rd_byte() != 0)
        player_ptr->pet_extra_flags |= PF_BALL_SPELL;
}
