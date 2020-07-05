/*!
 * todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 */

#include "savedata/extra-loader.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "monster-race/monster-race.h"
#include "mutation/mutation.h"
#include "object-enchant/tr-types.h"
#include "player/avatar.h"
#include "player/patron.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "realm/realm-types.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/load-v1-3-0.h"
#include "savedata/load-zangband.h"
#include "savedata/birth-loader.h"
#include "savedata/load-util.h"
#include "savedata/monster-loader.h"
#include "world/world.h"

/*!
 * @brief 腕力などの基本ステータス情報を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void rd_base_status(player_type *creature_ptr)
{
    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_max[i]);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_max_max[i]);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_cur[i]);
}

static void set_spells(player_type *creature_ptr)
{
    for (int i = 0; i < MAX_SPELLS; i++)
        rd_s32b(&creature_ptr->magic_num1[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        rd_byte(&creature_ptr->magic_num2[i]);

    if (h_older_than(1, 3, 0, 1))
        set_spells_old(creature_ptr);
}

static void set_race(player_type *creature_ptr)
{
    byte tmp8u; 
    rd_byte(&tmp8u);
    creature_ptr->start_race = (player_race_type)tmp8u;
    s32b tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->old_race1 = (BIT_FLAGS)tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->old_race2 = (BIT_FLAGS)tmp32s;
    rd_s16b(&creature_ptr->old_realm);
}

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void rd_extra(player_type *creature_ptr)
{
    rd_string(creature_ptr->name, sizeof(creature_ptr->name));
    rd_string(creature_ptr->died_from, sizeof(creature_ptr->died_from));
    if (!h_older_than(1, 7, 0, 1)) {
        char buf[1024];
        rd_string(buf, sizeof buf);
        if (buf[0])
            creature_ptr->last_message = string_make(buf);
    }

    load_quick_start();
    for (int i = 0; i < 4; i++) {
        rd_string(creature_ptr->history[i], sizeof(creature_ptr->history[i]));
    }

    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->prace = (player_race_type)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->pclass = (player_class_type)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->pseikaku = (player_personality_type)tmp8u;

    rd_byte(&creature_ptr->psex);
    rd_byte(&tmp8u);
    creature_ptr->realm1 = (REALM_IDX)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->realm2 = (REALM_IDX)tmp8u;

    rd_byte(&tmp8u);
    if (z_older_than(10, 4, 4))
        set_zangband_realm(creature_ptr);

    rd_byte(&tmp8u);
    creature_ptr->hitdie = tmp8u;
    rd_u16b(&creature_ptr->expfact);

    rd_s16b(&creature_ptr->age);
    rd_s16b(&creature_ptr->ht);
    rd_s16b(&creature_ptr->wt);
    rd_base_status(creature_ptr);
    strip_bytes(24);
    rd_s32b(&creature_ptr->au);

    rd_s32b(&creature_ptr->max_exp);
    if (h_older_than(1, 5, 4, 1))
        creature_ptr->max_max_exp = creature_ptr->max_exp;
    else
        rd_s32b(&creature_ptr->max_max_exp);

    rd_s32b(&creature_ptr->exp);

    if (h_older_than(1, 7, 0, 3)) {
        u16b tmp16u;
        rd_u16b(&tmp16u);
        creature_ptr->exp_frac = (u32b)tmp16u;
    } else {
        rd_u32b(&creature_ptr->exp_frac);
    }

    rd_s16b(&creature_ptr->lev);

    for (int i = 0; i < 64; i++)
        rd_s16b(&creature_ptr->spell_exp[i]);

    if ((creature_ptr->pclass == CLASS_SORCERER) && z_older_than(10, 4, 2))
        for (int i = 0; i < 64; i++)
            creature_ptr->spell_exp[i] = SPELL_EXP_MASTER;

    const int max_weapon_exp_size = z_older_than(10, 3, 6) ? 60 : 64;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < max_weapon_exp_size; j++)
            rd_s16b(&creature_ptr->weapon_exp[i][j]);

    for (int i = 0; i < GINOU_MAX; i++)
        rd_s16b(&creature_ptr->skill_exp[i]);

    if (z_older_than(10, 4, 1))
        set_zangband_skill(creature_ptr);

    if (z_older_than(10, 3, 14))
        set_zangband_spells(creature_ptr);
    else
        set_spells(creature_ptr);

    if (music_singing_any(creature_ptr))
        creature_ptr->action = ACTION_SING;

    if (z_older_than(11, 0, 7))
        set_zangband_race(creature_ptr);
    else
        set_race(creature_ptr);

    if (z_older_than(10, 0, 1)) {
        for (int i = 0; i < MAX_MANE; i++) {
            creature_ptr->mane_spell[i] = -1;
            creature_ptr->mane_dam[i] = 0;
        }
        creature_ptr->mane_num = 0;
    } else if (z_older_than(10, 2, 3)) {
        s16b tmp16s;
        const int OLD_MAX_MANE = 22;
        for (int i = 0; i < OLD_MAX_MANE; i++) {
            rd_s16b(&tmp16s);
            rd_s16b(&tmp16s);
        }

        for (int i = 0; i < MAX_MANE; i++) {
            creature_ptr->mane_spell[i] = -1;
            creature_ptr->mane_dam[i] = 0;
        }

        rd_s16b(&tmp16s);
        creature_ptr->mane_num = 0;
    } else {
        for (int i = 0; i < MAX_MANE; i++) {
            s16b tmp16s;
            rd_s16b(&tmp16s);
            creature_ptr->mane_spell[i] = (SPELL_IDX)tmp16s;
            rd_s16b(&tmp16s);
            creature_ptr->mane_dam[i] = (SPELL_IDX)tmp16s;
        }

        rd_s16b(&creature_ptr->mane_num);
    }

    if (z_older_than(10, 0, 3)) {
        determine_bounty_uniques(creature_ptr);

        for (int i = 0; i < MAX_BOUNTY; i++) {
            /* Is this bounty unique already dead? */
            if (!r_info[current_world_ptr->bounty_r_idx[i]].max_num)
                current_world_ptr->bounty_r_idx[i] += 10000;
        }
    } else {
        for (int i = 0; i < MAX_BOUNTY; i++) {
            rd_s16b(&current_world_ptr->bounty_r_idx[i]);
        }
    }

    if (z_older_than(10, 0, 3)) {
        update_gambling_monsters(creature_ptr);
    } else {
        for (int i = 0; i < 4; i++) {
            rd_s16b(&battle_mon[i]);
            if (z_older_than(10, 3, 4)) {
                s16b tmp16s;
                rd_s16b(&tmp16s);
                mon_odds[i] = tmp16s;
            } else
                rd_u32b(&mon_odds[i]);
        }
    }

    rd_s16b(&creature_ptr->town_num);
    rd_s16b(&creature_ptr->arena_number);
    if (h_older_than(1, 5, 0, 1)) {
        if (creature_ptr->arena_number >= 99)
            creature_ptr->arena_number = ARENA_DEFEATED_OLD_VER;
    }

    s16b tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->current_floor_ptr->inside_arena = (bool)tmp16s;
    rd_s16b(&creature_ptr->current_floor_ptr->inside_quest);
    if (z_older_than(10, 3, 5))
        creature_ptr->phase_out = FALSE;
    else {
        rd_s16b(&tmp16s);
        creature_ptr->phase_out = (bool)tmp16s;
    }

    rd_byte(&creature_ptr->exit_bldg);
    rd_byte(&tmp8u);

    rd_s16b(&tmp16s);
    creature_ptr->oldpx = (POSITION)tmp16s;

    rd_s16b(&tmp16s);
    creature_ptr->oldpy = (POSITION)tmp16s;
    if (z_older_than(10, 3, 13) && !creature_ptr->current_floor_ptr->dun_level && !creature_ptr->current_floor_ptr->inside_arena) {
        creature_ptr->oldpy = 33;
        creature_ptr->oldpx = 131;
    }

    rd_s16b(&tmp16s);
    for (int i = 0; i < tmp16s; i++) {
        s16b tmp16s2;
        rd_s16b(&tmp16s2);
    }

    if (h_older_than(1, 7, 0, 3)) {
        rd_s16b(&tmp16s);
        creature_ptr->mhp = tmp16s;

        rd_s16b(&tmp16s);
        creature_ptr->chp = tmp16s;

        u16b tmp16u;
        rd_u16b(&tmp16u);
        creature_ptr->chp_frac = (u32b)tmp16u;
    } else {
        rd_s32b(&creature_ptr->mhp);
        rd_s32b(&creature_ptr->chp);
        rd_u32b(&creature_ptr->chp_frac);
    }

    if (h_older_than(1, 7, 0, 3)) {
        rd_s16b(&tmp16s);
        creature_ptr->msp = tmp16s;

        rd_s16b(&tmp16s);
        creature_ptr->csp = tmp16s;

        u16b tmp16u;
        rd_u16b(&tmp16u);
        creature_ptr->csp_frac = (u32b)tmp16u;
    } else {
        rd_s32b(&creature_ptr->msp);
        rd_s32b(&creature_ptr->csp);
        rd_u32b(&creature_ptr->csp_frac);
    }

    rd_s16b(&creature_ptr->max_plv);
    if (z_older_than(10, 3, 8)) {
        rd_s16b(&tmp16s);
        max_dlv[DUNGEON_ANGBAND] = tmp16s;
    } else {
        byte max = (byte)current_world_ptr->max_d_idx;

        rd_byte(&max);

        for (int i = 0; i < max; i++) {
            rd_s16b(&tmp16s);
            max_dlv[i] = tmp16s;
            if (max_dlv[i] > d_info[i].maxdepth)
                max_dlv[i] = d_info[i].maxdepth;
        }
    }

    if (creature_ptr->max_plv < creature_ptr->lev)
        creature_ptr->max_plv = creature_ptr->lev;

    strip_bytes(8);
    rd_s16b(&creature_ptr->sc);
    rd_s16b(&creature_ptr->concent);

    strip_bytes(2); /* Old "rest" */
    rd_s16b(&creature_ptr->blind);
    rd_s16b(&creature_ptr->paralyzed);
    rd_s16b(&creature_ptr->confused);
    rd_s16b(&creature_ptr->food);
    strip_bytes(4); /* Old "food_digested" / "protection" */

    rd_s16b(&creature_ptr->energy_need);
    if (z_older_than(11, 0, 13))
        creature_ptr->energy_need = 100 - creature_ptr->energy_need;
    if (h_older_than(2, 1, 2, 0))
        creature_ptr->enchant_energy_need = 0;
    else
        rd_s16b(&creature_ptr->enchant_energy_need);

    rd_s16b(&creature_ptr->fast);
    rd_s16b(&creature_ptr->slow);
    rd_s16b(&creature_ptr->afraid);
    rd_s16b(&creature_ptr->cut);
    rd_s16b(&creature_ptr->stun);
    rd_s16b(&creature_ptr->poisoned);
    rd_s16b(&creature_ptr->image);
    rd_s16b(&creature_ptr->protevil);
    rd_s16b(&creature_ptr->invuln);
    if (z_older_than(10, 0, 0))
        creature_ptr->ult_res = 0;
    else
        rd_s16b(&creature_ptr->ult_res);
    rd_s16b(&creature_ptr->hero);
    rd_s16b(&creature_ptr->shero);
    rd_s16b(&creature_ptr->shield);
    rd_s16b(&creature_ptr->blessed);
    rd_s16b(&creature_ptr->tim_invis);
    rd_s16b(&creature_ptr->word_recall);
    if (z_older_than(10, 3, 8))
        creature_ptr->recall_dungeon = DUNGEON_ANGBAND;
    else {
        rd_s16b(&tmp16s);
        creature_ptr->recall_dungeon = (byte)tmp16s;
    }

    if (h_older_than(1, 5, 0, 0))
        creature_ptr->alter_reality = 0;
    else
        rd_s16b(&creature_ptr->alter_reality);

    rd_s16b(&creature_ptr->see_infra);
    rd_s16b(&creature_ptr->tim_infra);
    rd_s16b(&creature_ptr->oppose_fire);
    rd_s16b(&creature_ptr->oppose_cold);
    rd_s16b(&creature_ptr->oppose_acid);
    rd_s16b(&creature_ptr->oppose_elec);
    rd_s16b(&creature_ptr->oppose_pois);
    if (z_older_than(10, 0, 2))
        creature_ptr->tsuyoshi = 0;
    else
        rd_s16b(&creature_ptr->tsuyoshi);

    /* Old savefiles do not have the following fields... */
    if ((current_world_ptr->z_major == 2) && (current_world_ptr->z_minor == 0) && (current_world_ptr->z_patch == 6)) {
        creature_ptr->tim_esp = 0;
        creature_ptr->wraith_form = 0;
        creature_ptr->resist_magic = 0;
        creature_ptr->tim_regen = 0;
        creature_ptr->tim_pass_wall = 0;
        creature_ptr->tim_stealth = 0;
        creature_ptr->tim_levitation = 0;
        creature_ptr->tim_sh_touki = 0;
        creature_ptr->lightspeed = 0;
        creature_ptr->tsubureru = 0;
        creature_ptr->tim_res_nether = 0;
        creature_ptr->tim_res_time = 0;
        creature_ptr->mimic_form = 0;
        creature_ptr->tim_mimic = 0;
        creature_ptr->tim_sh_fire = 0;
        creature_ptr->tim_reflect = 0;
        creature_ptr->multishadow = 0;
        creature_ptr->dustrobe = 0;
        creature_ptr->chaos_patron = ((creature_ptr->age + creature_ptr->sc) % MAX_PATRON);
        creature_ptr->muta1 = 0;
        creature_ptr->muta2 = 0;
        creature_ptr->muta3 = 0;
        get_virtues(creature_ptr);
    } else {
        rd_s16b(&creature_ptr->tim_esp);
        rd_s16b(&creature_ptr->wraith_form);
        rd_s16b(&creature_ptr->resist_magic);
        rd_s16b(&creature_ptr->tim_regen);
        rd_s16b(&creature_ptr->tim_pass_wall);
        rd_s16b(&creature_ptr->tim_stealth);
        rd_s16b(&creature_ptr->tim_levitation);
        rd_s16b(&creature_ptr->tim_sh_touki);
        rd_s16b(&creature_ptr->lightspeed);
        rd_s16b(&creature_ptr->tsubureru);
        if (z_older_than(10, 4, 7))
            creature_ptr->magicdef = 0;
        else
            rd_s16b(&creature_ptr->magicdef);
        rd_s16b(&creature_ptr->tim_res_nether);
        if (z_older_than(10, 4, 11)) {
            creature_ptr->tim_res_time = 0;
            creature_ptr->mimic_form = 0;
            creature_ptr->tim_mimic = 0;
            creature_ptr->tim_sh_fire = 0;
        } else {
            rd_s16b(&creature_ptr->tim_res_time);
            rd_byte(&tmp8u);
            creature_ptr->mimic_form = (IDX)tmp8u;
            rd_s16b(&creature_ptr->tim_mimic);
            rd_s16b(&creature_ptr->tim_sh_fire);
        }

        if (z_older_than(11, 0, 99)) {
            creature_ptr->tim_sh_holy = 0;
            creature_ptr->tim_eyeeye = 0;
        } else {
            rd_s16b(&creature_ptr->tim_sh_holy);
            rd_s16b(&creature_ptr->tim_eyeeye);
        }

        if (z_older_than(11, 0, 3)) {
            creature_ptr->tim_reflect = 0;
            creature_ptr->multishadow = 0;
            creature_ptr->dustrobe = 0;
        } else {
            rd_s16b(&creature_ptr->tim_reflect);
            rd_s16b(&creature_ptr->multishadow);
            rd_s16b(&creature_ptr->dustrobe);
        }

        rd_s16b(&creature_ptr->chaos_patron);
        rd_u32b(&creature_ptr->muta1);
        rd_u32b(&creature_ptr->muta2);
        rd_u32b(&creature_ptr->muta3);

        for (int i = 0; i < 8; i++)
            rd_s16b(&creature_ptr->virtues[i]);
        for (int i = 0; i < 8; i++)
            rd_s16b(&creature_ptr->vir_types[i]);
    }

    creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    if (z_older_than(10, 0, 9)) {
        rd_byte(&tmp8u);
        if (tmp8u)
            creature_ptr->special_attack = ATTACK_CONFUSE;
        creature_ptr->ele_attack = 0;
    } else {
        rd_s16b(&creature_ptr->ele_attack);
        rd_u32b(&creature_ptr->special_attack);
    }

    if (creature_ptr->special_attack & KAMAE_MASK)
        creature_ptr->action = ACTION_KAMAE;
    else if (creature_ptr->special_attack & KATA_MASK)
        creature_ptr->action = ACTION_KATA;
    if (z_older_than(10, 0, 12)) {
        creature_ptr->ele_immune = 0;
        creature_ptr->special_defense = 0;
    } else {
        rd_s16b(&creature_ptr->ele_immune);
        rd_u32b(&creature_ptr->special_defense);
    }

    rd_byte(&creature_ptr->knowledge);
    rd_byte(&tmp8u);
    creature_ptr->autopick_autoregister = tmp8u ? TRUE : FALSE;

    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
    creature_ptr->action = (ACTION_IDX)tmp8u;
    if (!z_older_than(10, 4, 3)) {
        rd_byte(&tmp8u);
        if (tmp8u)
            creature_ptr->action = ACTION_LEARN;
    }

    rd_byte((byte *)&preserve_mode);
    rd_byte((byte *)&creature_ptr->wait_report_score);

    for (int i = 0; i < 48; i++)
        rd_byte(&tmp8u);

    strip_bytes(12);
    rd_u32b(&current_world_ptr->seed_flavor);
    rd_u32b(&current_world_ptr->seed_town);

    rd_u16b(&creature_ptr->panic_save);
    rd_u16b(&current_world_ptr->total_winner);
    rd_u16b(&current_world_ptr->noscore);

    rd_byte(&tmp8u);
    creature_ptr->is_dead = tmp8u;

    rd_byte(&creature_ptr->feeling);

    switch (creature_ptr->start_race) {
    case RACE_VAMPIRE:
    case RACE_SKELETON:
    case RACE_ZOMBIE:
    case RACE_SPECTRE:
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
        break;
    default:
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
        break;
    }

    current_world_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    rd_s32b(&creature_ptr->current_floor_ptr->generated_turn);
    if (h_older_than(1, 7, 0, 4)) {
        creature_ptr->feeling_turn = creature_ptr->current_floor_ptr->generated_turn;
    } else {
        rd_s32b(&creature_ptr->feeling_turn);
    }

    rd_s32b(&current_world_ptr->game_turn);
    if (z_older_than(10, 3, 12)) {
        current_world_ptr->dungeon_turn = current_world_ptr->game_turn;
    } else
        rd_s32b(&current_world_ptr->dungeon_turn);

    if (z_older_than(11, 0, 13)) {
        creature_ptr->current_floor_ptr->generated_turn /= 2;
        creature_ptr->feeling_turn /= 2;
        current_world_ptr->game_turn /= 2;
        current_world_ptr->dungeon_turn /= 2;
    }

    if (z_older_than(10, 3, 13)) {
        current_world_ptr->arena_start_turn = current_world_ptr->game_turn;
    } else
        rd_s32b(&current_world_ptr->arena_start_turn);

    if (z_older_than(10, 0, 3)) {
        determine_daily_bounty(creature_ptr, TRUE);
    } else {
        rd_s16b(&today_mon);
        rd_s16b(&creature_ptr->today_mon);
    }

    if (z_older_than(10, 0, 7)) {
        creature_ptr->riding = 0;
    } else {
        rd_s16b(&creature_ptr->riding);
    }

    if (h_older_than(1, 5, 0, 0)) {
        creature_ptr->floor_id = 0;
    } else {
        rd_s16b(&creature_ptr->floor_id);
    }

    if (h_older_than(1, 5, 0, 2)) {
        /* Nothing to do */
    } else {
        rd_s16b(&tmp16s);
        for (int i = 0; i < tmp16s; i++) {
            monster_type dummy_mon;
            rd_monster(creature_ptr, &dummy_mon);
        }
    }

    if (z_older_than(10, 1, 2)) {
        current_world_ptr->play_time = 0;
    } else {
        rd_u32b(&current_world_ptr->play_time);
    }

    if (z_older_than(10, 3, 9)) {
        creature_ptr->visit = 1L;
    } else if (z_older_than(10, 3, 10)) {
        s32b tmp32s;
        rd_s32b(&tmp32s);
        creature_ptr->visit = 1L;
    } else {
        s32b tmp32s;
        rd_s32b(&tmp32s);
        creature_ptr->visit = (BIT_FLAGS)tmp32s;
    }

    if (!z_older_than(11, 0, 5)) {
        rd_u32b(&creature_ptr->count);
    }
}
