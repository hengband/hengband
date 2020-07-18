/*!
 * @file save.c
 * @brief セーブファイル書き込み処理 / Purpose: interact with savefiles
 * @date 2014/07/12
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "save/save.h"
#include "birth/quick-start.h"
#include "cmd-building/cmd-building.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-events.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/report.h"
#include "io/uid-checker.h"
#include "load/floor-loader.h"
#include "load/load.h"
#include "load/savedata-flag-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-compaction.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object/object-kind.h"
#include "save/info-writer.h"
#include "save/item-writer.h"
#include "save/monster-writer.h"
#include "save/save-util.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "util/angband-files.h"
#include "util/quarks.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief その他の情報を書き込む / Write some "extra" info
 * @return なし
 */
static void wr_extra(player_type *creature_ptr)
{
    wr_string(creature_ptr->name);
    wr_string(creature_ptr->died_from);
    wr_string(creature_ptr->last_message ? creature_ptr->last_message : "");

    save_quick_start();

    for (int i = 0; i < 4; i++) {
        wr_string(creature_ptr->history[i]);
    }

    wr_byte((byte)creature_ptr->prace);
    wr_byte((byte)creature_ptr->pclass);
    wr_byte((byte)creature_ptr->pseikaku);
    wr_byte((byte)creature_ptr->psex);
    wr_byte((byte)creature_ptr->realm1);
    wr_byte((byte)creature_ptr->realm2);
    wr_byte(0);

    wr_byte((byte)creature_ptr->hitdie);
    wr_u16b(creature_ptr->expfact);

    wr_s16b(creature_ptr->age);
    wr_s16b(creature_ptr->ht);
    wr_s16b(creature_ptr->wt);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(creature_ptr->stat_max[i]);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(creature_ptr->stat_max_max[i]);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(creature_ptr->stat_cur[i]);

    for (int i = 0; i < 12; ++i)
        wr_s16b(0);

    wr_u32b(creature_ptr->au);
    wr_u32b(creature_ptr->max_exp);
    wr_u32b(creature_ptr->max_max_exp);
    wr_u32b(creature_ptr->exp);
    wr_u32b(creature_ptr->exp_frac);
    wr_s16b(creature_ptr->lev);

    for (int i = 0; i < 64; i++)
        wr_s16b(creature_ptr->spell_exp[i]);

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 64; j++)
            wr_s16b(creature_ptr->weapon_exp[i][j]);

    for (int i = 0; i < GINOU_MAX; i++)
        wr_s16b(creature_ptr->skill_exp[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        wr_s32b(creature_ptr->magic_num1[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        wr_byte(creature_ptr->magic_num2[i]);

    wr_byte((byte)creature_ptr->start_race);
    wr_s32b(creature_ptr->old_race1);
    wr_s32b(creature_ptr->old_race2);
    wr_s16b(creature_ptr->old_realm);
    for (int i = 0; i < MAX_MANE; i++) {
        wr_s16b((s16b)creature_ptr->mane_spell[i]);
        wr_s16b((s16b)creature_ptr->mane_dam[i]);
    }

    wr_s16b(creature_ptr->mane_num);
    for (int i = 0; i < MAX_BOUNTY; i++)
        wr_s16b(current_world_ptr->bounty_r_idx[i]);

    for (int i = 0; i < 4; i++) {
        wr_s16b(battle_mon[i]);
        wr_u32b(mon_odds[i]);
    }

    wr_s16b(creature_ptr->town_num);

    wr_s16b(creature_ptr->arena_number);
    wr_s16b(creature_ptr->current_floor_ptr->inside_arena);
    wr_s16b(creature_ptr->current_floor_ptr->inside_quest);
    wr_s16b(creature_ptr->phase_out);
    wr_byte(creature_ptr->exit_bldg);
    wr_byte(0); /* Unused */

    wr_s16b((s16b)creature_ptr->oldpx);
    wr_s16b((s16b)creature_ptr->oldpy);

    wr_s16b(0);
    wr_s32b(creature_ptr->mhp);
    wr_s32b(creature_ptr->chp);
    wr_u32b(creature_ptr->chp_frac);
    wr_s32b(creature_ptr->msp);
    wr_s32b(creature_ptr->csp);
    wr_u32b(creature_ptr->csp_frac);
    wr_s16b(creature_ptr->max_plv);

    byte tmp8u = (byte)current_world_ptr->max_d_idx;
    wr_byte(tmp8u);
    for (int i = 0; i < tmp8u; i++)
        wr_s16b((s16b)max_dlv[i]);

    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(creature_ptr->sc);
    wr_s16b(creature_ptr->concent);

    wr_s16b(0); /* old "rest" */
    wr_s16b(creature_ptr->blind);
    wr_s16b(creature_ptr->paralyzed);
    wr_s16b(creature_ptr->confused);
    wr_s16b(creature_ptr->food);
    wr_s16b(0); /* old "food_digested" */
    wr_s16b(0); /* old "protection" */
    wr_s16b(creature_ptr->energy_need);
    wr_s16b(creature_ptr->enchant_energy_need);
    wr_s16b(creature_ptr->fast);
    wr_s16b(creature_ptr->slow);
    wr_s16b(creature_ptr->afraid);
    wr_s16b(creature_ptr->cut);
    wr_s16b(creature_ptr->stun);
    wr_s16b(creature_ptr->poisoned);
    wr_s16b(creature_ptr->image);
    wr_s16b(creature_ptr->protevil);
    wr_s16b(creature_ptr->invuln);
    wr_s16b(creature_ptr->ult_res);
    wr_s16b(creature_ptr->hero);
    wr_s16b(creature_ptr->shero);
    wr_s16b(creature_ptr->shield);
    wr_s16b(creature_ptr->blessed);
    wr_s16b(creature_ptr->tim_invis);
    wr_s16b(creature_ptr->word_recall);
    wr_s16b(creature_ptr->recall_dungeon);
    wr_s16b(creature_ptr->alter_reality);
    wr_s16b(creature_ptr->see_infra);
    wr_s16b(creature_ptr->tim_infra);
    wr_s16b(creature_ptr->oppose_fire);
    wr_s16b(creature_ptr->oppose_cold);
    wr_s16b(creature_ptr->oppose_acid);
    wr_s16b(creature_ptr->oppose_elec);
    wr_s16b(creature_ptr->oppose_pois);
    wr_s16b(creature_ptr->tsuyoshi);
    wr_s16b(creature_ptr->tim_esp);
    wr_s16b(creature_ptr->wraith_form);
    wr_s16b(creature_ptr->resist_magic);
    wr_s16b(creature_ptr->tim_regen);
    wr_s16b(creature_ptr->tim_pass_wall);
    wr_s16b(creature_ptr->tim_stealth);
    wr_s16b(creature_ptr->tim_levitation);
    wr_s16b(creature_ptr->tim_sh_touki);
    wr_s16b(creature_ptr->lightspeed);
    wr_s16b(creature_ptr->tsubureru);
    wr_s16b(creature_ptr->magicdef);
    wr_s16b(creature_ptr->tim_res_nether);
    wr_s16b(creature_ptr->tim_res_time);
    wr_byte((byte)creature_ptr->mimic_form);
    wr_s16b(creature_ptr->tim_mimic);
    wr_s16b(creature_ptr->tim_sh_fire);
    wr_s16b(creature_ptr->tim_sh_holy);
    wr_s16b(creature_ptr->tim_eyeeye);

    wr_s16b(creature_ptr->tim_reflect);
    wr_s16b(creature_ptr->multishadow);
    wr_s16b(creature_ptr->dustrobe);

    wr_s16b(creature_ptr->chaos_patron);
    wr_u32b(creature_ptr->muta1);
    wr_u32b(creature_ptr->muta2);
    wr_u32b(creature_ptr->muta3);

    for (int i = 0; i < 8; i++)
        wr_s16b(creature_ptr->virtues[i]);

    for (int i = 0; i < 8; i++)
        wr_s16b(creature_ptr->vir_types[i]);

    wr_s16b(creature_ptr->ele_attack);
    wr_u32b(creature_ptr->special_attack);
    wr_s16b(creature_ptr->ele_immune);
    wr_u32b(creature_ptr->special_defense);
    wr_byte(creature_ptr->knowledge);
    wr_byte(creature_ptr->autopick_autoregister);
    wr_byte(0);
    wr_byte((byte)creature_ptr->action);
    wr_byte(0);
    wr_byte(preserve_mode);
    wr_byte(creature_ptr->wait_report_score);

    for (int i = 0; i < 12; i++)
        wr_u32b(0L);

    /* Ignore some flags */
    wr_u32b(0L);
    wr_u32b(0L);
    wr_u32b(0L);

    wr_u32b(current_world_ptr->seed_flavor);
    wr_u32b(current_world_ptr->seed_town);
    wr_u16b(creature_ptr->panic_save);
    wr_u16b(current_world_ptr->total_winner);
    wr_u16b(current_world_ptr->noscore);
    wr_byte(creature_ptr->is_dead);
    wr_byte(creature_ptr->feeling);
    wr_s32b(creature_ptr->current_floor_ptr->generated_turn);
    wr_s32b(creature_ptr->feeling_turn);
    wr_s32b(current_world_ptr->game_turn);
    wr_s32b(current_world_ptr->dungeon_turn);
    wr_s32b(current_world_ptr->arena_start_turn);
    wr_s16b(today_mon);
    wr_s16b(creature_ptr->today_mon);
    wr_s16b(creature_ptr->riding);
    wr_s16b(creature_ptr->floor_id);

    /* Save temporary preserved pets (obsolated) */
    wr_s16b(0);
    wr_u32b(current_world_ptr->play_time);
    wr_s32b(creature_ptr->visit);
    wr_u32b(creature_ptr->count);
}

/*!
 * @brief 保存フロアの書き込み / Actually write a saved floor data using effectively compressed format.
 * @param sf_ptr 保存したいフロアの参照ポインタ
 * @return なし
 */
static void wr_saved_floor(player_type *player_ptr, saved_floor_type *sf_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!sf_ptr) {
        wr_s16b((s16b)floor_ptr->dun_level);
    } else {
        wr_s16b(sf_ptr->floor_id);
        wr_byte((byte)sf_ptr->savefile_id);
        wr_s16b((s16b)sf_ptr->dun_level);
        wr_s32b(sf_ptr->last_visit);
        wr_u32b(sf_ptr->visit_mark);
        wr_s16b(sf_ptr->upper_floor_id);
        wr_s16b(sf_ptr->lower_floor_id);
    }

    wr_u16b((u16b)floor_ptr->base_level);
    wr_u16b((s16b)player_ptr->current_floor_ptr->num_repro);
    wr_u16b((u16b)player_ptr->y);
    wr_u16b((u16b)player_ptr->x);
    wr_u16b((u16b)floor_ptr->height);
    wr_u16b((u16b)floor_ptr->width);
    wr_byte(player_ptr->feeling);

    /*
     * Usually number of templates are fewer than 255.  Even if
     * more than 254 are exist, the occurrence of each template
     * with larger ID is very small when we sort templates by
     * occurrence.  So we will use two (or more) bytes for
     * templete ID larger than 254.
     *
     * Ex: 256 will be "0xff" "0x01".
     *     515 will be "0xff" "0xff" "0x03"
     */

    /* Fake max number */
    u16b max_num_temp = 255;

    grid_template_type *templates;
    C_MAKE(templates, max_num_temp, grid_template_type);
    u16b num_temp = 0;
    for (int y = 0; y < floor_ptr->height; y++) {
        for (int x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr = &floor_ptr->grid_array[y][x];
            int i;
            for (i = 0; i < num_temp; i++) {
                if (templates[i].info == g_ptr->info && templates[i].feat == g_ptr->feat && templates[i].mimic == g_ptr->mimic
                    && templates[i].special == g_ptr->special) {
                    templates[i].occurrence++;
                    break;
                }
            }

            if (i < num_temp)
                continue;

            if (num_temp >= max_num_temp) {
                grid_template_type *old_template = templates;
                C_MAKE(templates, max_num_temp + 255, grid_template_type);
                (void)C_COPY(templates, old_template, max_num_temp, grid_template_type);
                C_KILL(old_template, max_num_temp, grid_template_type);
                max_num_temp += 255;
            }

            templates[num_temp].info = g_ptr->info;
            templates[num_temp].feat = g_ptr->feat;
            templates[num_temp].mimic = g_ptr->mimic;
            templates[num_temp].special = g_ptr->special;
            templates[num_temp].occurrence = 1;
            num_temp++;
        }
    }

    int dummy_why;
    ang_sort(player_ptr, templates, &dummy_why, num_temp, ang_sort_comp_cave_temp, ang_sort_swap_cave_temp);

    /*** Dump templates ***/
    wr_u16b(num_temp);
    for (int i = 0; i < num_temp; i++) {
        grid_template_type *ct_ptr = &templates[i];
        wr_u16b((u16b)ct_ptr->info);
        wr_s16b(ct_ptr->feat);
        wr_s16b(ct_ptr->mimic);
        wr_s16b(ct_ptr->special);
    }

    byte count = 0;
    u16b prev_u16b = 0;
    for (int y = 0; y < floor_ptr->height; y++) {
        for (int x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr = &floor_ptr->grid_array[y][x];
            int i;
            for (i = 0; i < num_temp; i++) {
                if (templates[i].info == g_ptr->info && templates[i].feat == g_ptr->feat && templates[i].mimic == g_ptr->mimic
                    && templates[i].special == g_ptr->special)
                    break;
            }

            u16b tmp16u = (u16b)i;
            if ((tmp16u == prev_u16b) && (count != MAX_UCHAR)) {
                count++;
                continue;
            }

            wr_byte((byte)count);
            while (prev_u16b >= MAX_UCHAR) {
                wr_byte(MAX_UCHAR);
                prev_u16b -= MAX_UCHAR;
            }

            wr_byte((byte)prev_u16b);
            prev_u16b = tmp16u;
            count = 1;
        }
    }

    if (count > 0) {
        wr_byte((byte)count);
        while (prev_u16b >= MAX_UCHAR) {
            wr_byte(MAX_UCHAR);
            prev_u16b -= MAX_UCHAR;
        }

        wr_byte((byte)prev_u16b);
    }

    C_KILL(templates, max_num_temp, grid_template_type);

    /*** Dump objects ***/
    wr_u16b(floor_ptr->o_max);
    for (int i = 1; i < floor_ptr->o_max; i++) {
        object_type *o_ptr = &floor_ptr->o_list[i];
        wr_item(o_ptr);
    }

    /*** Dump the monsters ***/
    wr_u16b(floor_ptr->m_max);
    for (int i = 1; i < floor_ptr->m_max; i++) {
        monster_type *m_ptr = &floor_ptr->m_list[i];
        wr_monster(m_ptr);
    }
}

/*!
 * @brief 現在フロアの書き込み /
 * Write the current dungeon (new method)
 * @player_ptr プレーヤーへの参照ポインタ
 * @return 保存に成功したらTRUE
 */
static bool wr_dungeon(player_type *player_ptr)
{
    forget_lite(player_ptr->current_floor_ptr);
    forget_view(player_ptr->current_floor_ptr);
    clear_mon_lite(player_ptr->current_floor_ptr);
    player_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
    player_ptr->update |= (PU_MONSTERS | PU_DISTANCE | PU_FLOW);
    wr_s16b(max_floor_id);
    wr_byte((byte)player_ptr->dungeon_idx);
    if (!player_ptr->floor_id) {
        /* No array elements */
        wr_byte(0);
        wr_saved_floor(player_ptr, NULL);
        return TRUE;
    }

    /*** In the dungeon ***/
    wr_byte(MAX_SAVED_FLOORS);
    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        wr_s16b(sf_ptr->floor_id);
        wr_byte((byte)sf_ptr->savefile_id);
        wr_s16b((s16b)sf_ptr->dun_level);
        wr_s32b(sf_ptr->last_visit);
        wr_u32b(sf_ptr->visit_mark);
        wr_s16b(sf_ptr->upper_floor_id);
        wr_s16b(sf_ptr->lower_floor_id);
    }

    saved_floor_type *cur_sf_ptr;
    cur_sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (!save_floor(player_ptr, cur_sf_ptr, (SLF_SECOND)))
        return FALSE;

    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        if (!sf_ptr->floor_id)
            continue;
        if (!load_floor(player_ptr, sf_ptr, (SLF_SECOND | SLF_NO_KILL))) {
            wr_byte(1);
            continue;
        }

        wr_byte(0);
        wr_saved_floor(player_ptr, sf_ptr);
    }

    return load_floor(player_ptr, cur_sf_ptr, (SLF_SECOND));
}

/*!
 * @brief セーブデータの書き込み /
 * Actually write a save-file
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 */
static bool wr_savefile_new(player_type *player_ptr)
{
    compact_objects(player_ptr, 0);
    compact_monsters(player_ptr, 0);

    u32b now = (u32b)time((time_t *)0);
    current_world_ptr->sf_system = 0L;
    current_world_ptr->sf_when = now;
    current_world_ptr->sf_saves++;
    save_xor_byte = 0;
    wr_byte(FAKE_VER_MAJOR);
    save_xor_byte = 0;
    wr_byte(FAKE_VER_MINOR);
    save_xor_byte = 0;
    wr_byte(FAKE_VER_PATCH);
    save_xor_byte = 0;

    byte tmp8u = (byte)Rand_external(256);
    wr_byte(tmp8u);
    v_stamp = 0L;
    x_stamp = 0L;

    wr_byte(H_VER_EXTRA);
    wr_byte(H_VER_PATCH);
    wr_byte(H_VER_MINOR);
    wr_byte(H_VER_MAJOR);
    wr_u32b(current_world_ptr->sf_system);
    wr_u32b(current_world_ptr->sf_when);
    wr_u16b(current_world_ptr->sf_lives);
    wr_u16b(current_world_ptr->sf_saves);

    wr_u32b(0L);
    wr_u16b(0);
    wr_byte(0);

#ifdef JP
#ifdef EUC
    wr_byte(2);
#endif
#ifdef SJIS
    wr_byte(3);
#endif
#else
    wr_byte(1);
#endif

    wr_randomizer();
    wr_options();
    u32b tmp32u = message_num();
    if (compress_savefile && (tmp32u > 40))
        tmp32u = 40;

    wr_u32b(tmp32u);
    for (int i = tmp32u - 1; i >= 0; i--)
        wr_string(message_str((s16b)i));

    u16b tmp16u = max_r_idx;
    wr_u16b(tmp16u);
    for (MONRACE_IDX r_idx = 0; r_idx < tmp16u; r_idx++)
        wr_lore(r_idx);

    tmp16u = max_k_idx;
    wr_u16b(tmp16u);
    for (KIND_OBJECT_IDX k_idx = 0; k_idx < tmp16u; k_idx++)
        wr_perception(k_idx);

    tmp16u = max_towns;
    wr_u16b(tmp16u);

    tmp16u = max_q_idx;
    wr_u16b(tmp16u);

    tmp8u = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST;
    wr_byte(tmp8u);

    for (int i = 0; i < max_q_idx; i++) {
        quest_type *const q_ptr = &quest[i];
        wr_s16b(q_ptr->status);
        wr_s16b((s16b)q_ptr->level);
        wr_byte((byte)q_ptr->complev);
        wr_u32b(q_ptr->comptime);

        bool is_quest_running = q_ptr->status == QUEST_STATUS_TAKEN;
        is_quest_running |= q_ptr->status == QUEST_STATUS_COMPLETED;
        is_quest_running |= !is_fixed_quest_idx(i);
        if (!is_quest_running)
            continue;

        wr_s16b((s16b)q_ptr->cur_num);
        wr_s16b((s16b)q_ptr->max_num);
        wr_s16b(q_ptr->type);
        wr_s16b(q_ptr->r_idx);
        wr_s16b(q_ptr->k_idx);
        wr_byte((byte)q_ptr->flags);
        wr_byte((byte)q_ptr->dungeon);
    }

    wr_s32b(player_ptr->wilderness_x);
    wr_s32b(player_ptr->wilderness_y);
    wr_byte(player_ptr->wild_mode);
    wr_byte(player_ptr->ambush_flag);
    wr_s32b(current_world_ptr->max_wild_x);
    wr_s32b(current_world_ptr->max_wild_y);
    for (int i = 0; i < current_world_ptr->max_wild_x; i++)
        for (int j = 0; j < current_world_ptr->max_wild_y; j++)
            wr_u32b(wilderness[j][i].seed);

    tmp16u = max_a_idx;
    wr_u16b(tmp16u);
    for (int i = 0; i < tmp16u; i++) {
        artifact_type *a_ptr = &a_info[i];
        wr_byte(a_ptr->cur_num);
        wr_s16b(a_ptr->floor_id);
    }

    wr_extra(player_ptr);
    tmp16u = PY_MAX_LEVEL;
    wr_u16b(tmp16u);
    for (int i = 0; i < tmp16u; i++)
        wr_s16b((s16b)player_ptr->player_hp[i]);

    wr_u32b(player_ptr->spell_learned1);
    wr_u32b(player_ptr->spell_learned2);
    wr_u32b(player_ptr->spell_worked1);
    wr_u32b(player_ptr->spell_worked2);
    wr_u32b(player_ptr->spell_forgotten1);
    wr_u32b(player_ptr->spell_forgotten2);
    wr_s16b(player_ptr->learned_spells);
    wr_s16b(player_ptr->add_spells);
    for (int i = 0; i < 64; i++)
        wr_byte((byte)player_ptr->spell_order[i]);

    for (int i = 0; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        wr_u16b((u16b)i);
        wr_item(o_ptr);
    }

    wr_u16b(0xFFFF);
    tmp16u = max_towns;
    wr_u16b(tmp16u);

    tmp16u = MAX_STORES;
    wr_u16b(tmp16u);
    for (int i = 1; i < max_towns; i++)
        for (int j = 0; j < MAX_STORES; j++)
            wr_store(&town_info[i].store[j]);

    wr_s16b(player_ptr->pet_follow_distance);
    wr_s16b(player_ptr->pet_extra_flags);
    if (screen_dump && (player_ptr->wait_report_score || !player_ptr->is_dead))
        wr_string(screen_dump);
    else
        wr_string("");

    if (!player_ptr->is_dead) {
        if (!wr_dungeon(player_ptr))
            return FALSE;

        wr_ghost();
        wr_s32b(0);
    }

    wr_u32b(v_stamp);
    wr_u32b(x_stamp);
    return !ferror(saving_savefile) && (fflush(saving_savefile) != EOF);
}

/*!
 * @brief セーブデータ書き込みのサブルーチン /
 * Medium level player saver
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 * @details
 * Angband 2.8.0 will use "fd" instead of "fff" if possible
 */
static bool save_player_aux(player_type *player_ptr, char *name)
{
    safe_setuid_grab(player_ptr);
    int file_permission = 0644;
    int fd = fd_make(name, file_permission);
    safe_setuid_drop();

    bool is_save_successful = FALSE;
    saving_savefile = NULL;
    if (fd >= 0) {
        (void)fd_close(fd);
        safe_setuid_grab(player_ptr);
        saving_savefile = angband_fopen(name, "wb");
        safe_setuid_drop();
        if (saving_savefile) {
            if (wr_savefile_new(player_ptr))
                is_save_successful = TRUE;

            if (angband_fclose(saving_savefile))
                is_save_successful = FALSE;
        }

        safe_setuid_grab(player_ptr);
        if (!is_save_successful)
            (void)fd_kill(name);

        safe_setuid_drop();
    }

    if (!is_save_successful)
        return FALSE;

    counts_write(player_ptr, 0, current_world_ptr->play_time);
    current_world_ptr->character_saved = TRUE;
    return TRUE;
}

/*!
 * @brief セーブデータ書き込みのメインルーチン /
 * Attempt to save the player in a savefile
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 */
bool save_player(player_type *player_ptr)
{
    char safe[1024];
    strcpy(safe, savefile);
    strcat(safe, ".new");
    safe_setuid_grab(player_ptr);
    fd_kill(safe);
    safe_setuid_drop();
    update_playtime();
    bool result = FALSE;
    if (save_player_aux(player_ptr, safe)) {
        char temp[1024];
        strcpy(temp, savefile);
        strcat(temp, ".old");
        safe_setuid_grab(player_ptr);
        fd_kill(temp);
        fd_move(savefile, temp);
        fd_move(safe, savefile);
        fd_kill(temp);
        safe_setuid_drop();
        current_world_ptr->character_loaded = TRUE;
        result = TRUE;
    }

    return result;
}

/*!
 * @brief セーブデータ読み込みのメインルーチン /
 * Attempt to Load a "savefile"
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 */
bool load_player(player_type *player_ptr)
{
    concptr what = "generic";
    current_world_ptr->game_turn = 0;
    player_ptr->is_dead = FALSE;
    if (!savefile[0])
        return TRUE;

#ifndef WINDOWS
    if (access(savefile, 0) < 0) {
        msg_print(_("セーブファイルがありません。", "Savefile does not exist."));
        msg_print(NULL);
        return TRUE;
    }

#endif

    errr err = 0;
    int fd = -1;
    byte vvv[4];
    if (!err) {
        fd = fd_open(savefile, O_RDONLY);
        if (fd < 0)
            err = -1;

        if (err)
            what = _("セーブファイルを開けません。", "Cannot open savefile");
    }

    if (!err) {
        if (fd_read(fd, (char *)(vvv), 4))
            err = -1;

        if (err)
            what = _("セーブファイルを読めません。", "Cannot read savefile");

        (void)fd_close(fd);
    }

    if (!err) {
        current_world_ptr->z_major = vvv[0];
        current_world_ptr->z_minor = vvv[1];
        current_world_ptr->z_patch = vvv[2];
        current_world_ptr->sf_extra = vvv[3];
        term_clear();
        err = rd_savefile(player_ptr);
        if (err)
            what = _("セーブファイルを解析出来ません。", "Cannot parse savefile");
    }

    if (!err) {
        if (!current_world_ptr->game_turn)
            err = -1;

        if (err)
            what = _("セーブファイルが壊れています", "Broken savefile");
    }

    if (!err) {
        if ((FAKE_VER_MAJOR != current_world_ptr->z_major) || (FAKE_VER_MINOR != current_world_ptr->z_minor)
            || (FAKE_VER_PATCH != current_world_ptr->z_patch)) {
            if (current_world_ptr->z_major == 2 && current_world_ptr->z_minor == 0 && current_world_ptr->z_patch == 6) {
                msg_print(_("バージョン 2.0.* 用のセーブファイルを変換しました。", "Converted a 2.0.* savefile."));
            } else {
                msg_format(_("バージョン %d.%d.%d 用のセーブ・ファイルを変換しました。", "Converted a %d.%d.%d savefile."),
                    (current_world_ptr->z_major > 9) ? current_world_ptr->z_major - 10 : current_world_ptr->z_major, current_world_ptr->z_minor,
                    current_world_ptr->z_patch);
            }

            msg_print(NULL);
        }

        if (player_ptr->is_dead) {
            if (arg_wizard) {
                current_world_ptr->character_loaded = TRUE;
                return TRUE;
            }

            player_ptr->is_dead = FALSE;
            current_world_ptr->sf_lives++;
            return TRUE;
        }

        current_world_ptr->character_loaded = TRUE;
        u32b tmp = counts_read(player_ptr, 2);
        if (tmp > player_ptr->count)
            player_ptr->count = tmp;

        if (counts_read(player_ptr, 0) > current_world_ptr->play_time || counts_read(player_ptr, 1) == current_world_ptr->play_time)
            counts_write(player_ptr, 2, ++player_ptr->count);

        counts_write(player_ptr, 1, current_world_ptr->play_time);
        return TRUE;
    }

    msg_format(_("エラー(%s)がバージョン%d.%d.%d 用セーブファイル読み込み中に発生。", "Error (%s) reading %d.%d.%d savefile."), what,
        (current_world_ptr->z_major > 9) ? current_world_ptr->z_major - 10 : current_world_ptr->z_major, current_world_ptr->z_minor,
        current_world_ptr->z_patch);

    msg_print(NULL);
    return FALSE;
}

/*!
 * @brief ゲームプレイ中のフロア一時保存出力処理サブルーチン / Actually write a temporary saved floor file
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 保存フロア参照ポインタ
 * @return なし
 */
static bool save_floor_aux(player_type *player_ptr, saved_floor_type *sf_ptr)
{
    compact_objects(player_ptr, 0);
    compact_monsters(player_ptr, 0);

    byte tmp8u = (byte)randint0(256);
    save_xor_byte = 0;
    wr_byte(tmp8u);

    /* Reset the checksum */
    v_stamp = 0L;
    x_stamp = 0L;
    wr_u32b(saved_floor_file_sign);
    wr_saved_floor(player_ptr, sf_ptr);
    wr_u32b(v_stamp);
    wr_u32b(x_stamp);

    return !ferror(saving_savefile) && (fflush(saving_savefile) != EOF);
}

/*!
 * @brief ゲームプレイ中のフロア一時保存出力処理メインルーチン / Attempt to save the temporarily saved-floor data
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 保存フロア参照ポインタ
 * @param mode 保存オプション
 * @return なし
 */
bool save_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode)
{
    FILE *old_fff = NULL;
    byte old_xor_byte = 0;
    u32b old_v_stamp = 0;
    u32b old_x_stamp = 0;

    char floor_savefile[1024];
    if ((mode & SLF_SECOND) != 0) {
        old_fff = saving_savefile;
        old_xor_byte = save_xor_byte;
        old_v_stamp = v_stamp;
        old_x_stamp = x_stamp;
    }

    sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);
    safe_setuid_grab(player_ptr);
    fd_kill(floor_savefile);
    safe_setuid_drop();
    saving_savefile = NULL;
    safe_setuid_grab(player_ptr);

    int fd = fd_make(floor_savefile, 0644);
    safe_setuid_drop();
    bool is_save_successful = FALSE;
    if (fd >= 0) {
        (void)fd_close(fd);
        safe_setuid_grab(player_ptr);
        saving_savefile = angband_fopen(floor_savefile, "wb");
        safe_setuid_drop();
        if (saving_savefile) {
            if (save_floor_aux(player_ptr, sf_ptr))
                is_save_successful = TRUE;

            if (angband_fclose(saving_savefile))
                is_save_successful = FALSE;
        }

        if (!is_save_successful) {
            safe_setuid_grab(player_ptr);
            (void)fd_kill(floor_savefile);
            safe_setuid_drop();
        }
    }

    if ((mode & SLF_SECOND) != 0) {
        saving_savefile = old_fff;
        save_xor_byte = old_xor_byte;
        v_stamp = old_v_stamp;
        x_stamp = old_x_stamp;
    }

    return is_save_successful;
}
