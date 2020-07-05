/*!
 * @brief セーブファイル読み込み処理 / Purpose: support for loading savefiles -BEN-
 * @date 2014/07/07
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "savedata/load.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/runtime-arguments.h"
#include "info-reader/fixed-map-parser.h"
#include "io/report.h"
#include "io/uid-checker.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "object-enchant/artifact.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind.h"
#include "pet/pet-util.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/dummy-loader.h"
#include "savedata/dungeon-loader.h"
#include "savedata/extra-loader.h"
#include "savedata/inventory-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-5-0.h"
#include "savedata/lore-loader.h"
#include "savedata/option-loader.h"
#include "savedata/store-loader.h"
#include "savedata/player-info-loader.h"
#include "savedata/world-loader.h"
#include "spell/spells-status.h"
#include "system/system-variables.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"

static void rd_version_info(void)
{
    strip_bytes(4);
    load_xor_byte = current_world_ptr->sf_extra;
    v_check = 0L;
    x_check = 0L;

    /* Old savefile will be version 0.0.0.3 */
    rd_byte(&current_world_ptr->h_ver_extra);
    rd_byte(&current_world_ptr->h_ver_patch);
    rd_byte(&current_world_ptr->h_ver_minor);
    rd_byte(&current_world_ptr->h_ver_major);

    load_note(format(_("バージョン %d.%d.%d.%d のセーブ・ファイルをロード中...", "Loading a %d.%d.%d.%d savefile..."),
        (current_world_ptr->h_ver_major > 9) ? current_world_ptr->h_ver_major - 10 : current_world_ptr->h_ver_major, current_world_ptr->h_ver_minor,
        current_world_ptr->h_ver_patch, current_world_ptr->h_ver_extra));

    rd_u32b(&current_world_ptr->sf_system);
    rd_u32b(&current_world_ptr->sf_when);
    rd_u16b(&current_world_ptr->sf_lives);
    rd_u16b(&current_world_ptr->sf_saves);
}

/*!
 * @brief メッセージログを読み込む / Read the saved messages
 * @return なし
 */
static void rd_messages(void)
{
    if (h_older_than(2, 2, 0, 75)) {
        u16b num;
        rd_u16b(&num);
        int message_max;
        message_max = (int)num;

        for (int i = 0; i < message_max; i++) {
            char buf[128];
            rd_string(buf, sizeof(buf));
            message_add(buf);
        }
    }

    u32b num;
    rd_u32b(&num);
    int message_max;
    message_max = (int)num;

    for (int i = 0; i < message_max; i++) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        message_add(buf);
    }
}

/*!
 * @brief 乱数状態を読み込む / Read RNG state (added in 2.8.0)
 * @return なし
 */
static void rd_randomizer(void)
{
    u16b tmp16u;
    rd_u16b(&tmp16u);
    rd_u16b(&Rand_place);
    for (int i = 0; i < RAND_DEG; i++)
        rd_u32b(&Rand_state[i]);
}

/*!
 * @brief セーブファイル読み込み処理の実体 / Actually read the savefile
 * @return エラーコード
 */
static errr exe_reading_savefile(player_type *creature_ptr)
{
    rd_version_info();

    u32b tmp32u;
    rd_u32b(&tmp32u);

    u16b tmp16u;
    rd_u16b(&tmp16u);

    byte tmp8u;
    rd_byte(&tmp8u);
    rd_byte(&kanji_code);

    rd_randomizer();
    if (arg_fiddle)
        load_note(_("乱数情報をロードしました", "Loaded Randomizer Info"));

    rd_options();
    if (arg_fiddle)
        load_note(_("オプションをロードしました", "Loaded Option Flags"));

    rd_messages();
    if (arg_fiddle)
        load_note(_("メッセージをロードしました", "Loaded Messages"));

    /* ランダムクエストのモンスターを確定するために試行する回数 / Maximum number of tries for selection of a proper quest monster */
    const int MAX_TRIES = 100;
    for (int i = 0; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        r_ptr->max_num = MAX_TRIES;

        if (r_ptr->flags1 & RF1_UNIQUE)
            r_ptr->max_num = 1;
        else if (r_ptr->flags7 & RF7_NAZGUL)
            r_ptr->max_num = MAX_NAZGUL_NUM;
    }

    rd_u16b(&tmp16u);
    if (tmp16u > max_r_idx) {
        load_note(format(_("モンスターの種族が多すぎる(%u)！", "Too many (%u) monster races!"), tmp16u));
        return (21);
    }

    for (int i = 0; i < tmp16u; i++) {
        rd_lore((MONRACE_IDX)i);
    }

    if (arg_fiddle)
        load_note(_("モンスターの思い出をロードしました", "Loaded Monster Memory"));

    rd_u16b(&tmp16u);
    if (tmp16u > max_k_idx) {
        load_note(format(_("アイテムの種類が多すぎる(%u)！", "Too many (%u) object kinds!"), tmp16u));
        return (22);
    }

    for (int i = 0; i < tmp16u; i++) {
        object_kind *k_ptr = &k_info[i];
        rd_byte(&tmp8u);
        k_ptr->aware = (tmp8u & 0x01) ? TRUE : FALSE;
        k_ptr->tried = (tmp8u & 0x02) ? TRUE : FALSE;
    }
    if (arg_fiddle)
        load_note(_("アイテムの記録をロードしました", "Loaded Object Memory"));

    /* 2.1.3 or newer version */
    {
        u16b max_towns_load;
        u16b max_quests_load;
        byte max_rquests_load;
        s16b old_inside_quest = creature_ptr->current_floor_ptr->inside_quest;

        rd_u16b(&max_towns_load);
        if (max_towns_load > max_towns) {
            load_note(format(_("町が多すぎる(%u)！", "Too many (%u) towns!"), max_towns_load));
            return (23);
        }

        rd_u16b(&max_quests_load);
        if (z_older_than(11, 0, 7)) {
            max_rquests_load = 10;
        } else {
            rd_byte(&max_rquests_load);
        }

        if (max_quests_load > max_q_idx) {
            load_note(format(_("クエストが多すぎる(%u)！", "Too many (%u) quests!"), max_quests_load));
            return (23);
        }

        for (int i = 0; i < max_quests_load; i++) {
            if (i >= max_q_idx) {
                strip_bytes(2);
                strip_bytes(2);
                continue;
            }

            quest_type *const q_ptr = &quest[i];

            rd_s16b(&q_ptr->status);
            s16b tmp16s;
            rd_s16b(&tmp16s);
            q_ptr->level = tmp16s;

            if (z_older_than(11, 0, 6)) {
                q_ptr->complev = 0;
            } else {
                rd_byte(&tmp8u);
                q_ptr->complev = tmp8u;
            }
            if (h_older_than(2, 1, 2, 2)) {
                q_ptr->comptime = 0;
            } else {
                rd_u32b(&q_ptr->comptime);
            }

            bool is_quest_running = (q_ptr->status == QUEST_STATUS_TAKEN);
            is_quest_running |= (!z_older_than(10, 3, 14) && (q_ptr->status == QUEST_STATUS_COMPLETED));
            is_quest_running |= (!z_older_than(11, 0, 7) && (i >= MIN_RANDOM_QUEST) && (i <= (MIN_RANDOM_QUEST + max_rquests_load)));
            if (!is_quest_running)
                continue;

            rd_s16b(&tmp16s);
            q_ptr->cur_num = (MONSTER_NUMBER)tmp16s;
            rd_s16b(&tmp16s);
            q_ptr->max_num = (MONSTER_NUMBER)tmp16s;
            rd_s16b(&q_ptr->type);

            rd_s16b(&q_ptr->r_idx);
            if ((q_ptr->type == QUEST_TYPE_RANDOM) && (!q_ptr->r_idx)) {
                determine_random_questor(creature_ptr, &quest[i]);
            }

            rd_s16b(&q_ptr->k_idx);
            if (q_ptr->k_idx)
                a_info[q_ptr->k_idx].gen_flags |= TRG_QUESTITEM;

            rd_byte(&tmp8u);
            q_ptr->flags = tmp8u;

            if (z_older_than(10, 3, 11)) {
                if (q_ptr->flags & QUEST_FLAG_PRESET) {
                    q_ptr->dungeon = 0;
                } else {
                    init_flags = INIT_ASSIGN;
                    creature_ptr->current_floor_ptr->inside_quest = (QUEST_IDX)i;

                    parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
                    creature_ptr->current_floor_ptr->inside_quest = old_inside_quest;
                }
            } else {
                rd_byte(&tmp8u);
                q_ptr->dungeon = tmp8u;
            }

            if (q_ptr->status == QUEST_STATUS_TAKEN || q_ptr->status == QUEST_STATUS_UNTAKEN)
                if (r_info[q_ptr->r_idx].flags1 & RF1_UNIQUE)
                    r_info[q_ptr->r_idx].flags1 |= RF1_QUESTOR;
        }

        /* Quest 18 was removed */
        if (h_older_than(1, 7, 0, 6)) {
            (void)WIPE(&quest[OLD_QUEST_WATER_CAVE], quest_type);
            quest[OLD_QUEST_WATER_CAVE].status = QUEST_STATUS_UNTAKEN;
        }

        rd_s32b(&creature_ptr->wilderness_x);
        rd_s32b(&creature_ptr->wilderness_y);
        if (z_older_than(10, 3, 13)) {
            creature_ptr->wilderness_x = 5;
            creature_ptr->wilderness_y = 48;
        }

        if (z_older_than(10, 3, 7))
            creature_ptr->wild_mode = FALSE;
        else
            rd_byte((byte *)&creature_ptr->wild_mode);
        if (z_older_than(10, 3, 7))
            creature_ptr->ambush_flag = FALSE;
        else
            rd_byte((byte *)&creature_ptr->ambush_flag);

        s32b wild_x_size;
        s32b wild_y_size;
        rd_s32b(&wild_x_size);
        rd_s32b(&wild_y_size);

        if ((wild_x_size > current_world_ptr->max_wild_x) || (wild_y_size > current_world_ptr->max_wild_y)) {
            load_note(format(_("荒野が大きすぎる(%u/%u)！", "Wilderness is too big (%u/%u)!"), wild_x_size, wild_y_size));
            return (23);
        }

        for (int i = 0; i < wild_x_size; i++) {
            for (int j = 0; j < wild_y_size; j++) {
                rd_u32b(&wilderness[j][i].seed);
            }
        }
    }

    if (arg_fiddle)
        load_note(_("クエスト情報をロードしました", "Loaded Quests"));

    rd_u16b(&tmp16u);
    if (tmp16u > max_a_idx) {
        load_note(format(_("伝説のアイテムが多すぎる(%u)！", "Too many (%u) artifacts!"), tmp16u));
        return (24);
    }

    for (int i = 0; i < tmp16u; i++) {
        artifact_type *a_ptr = &a_info[i];

        rd_byte(&tmp8u);
        a_ptr->cur_num = tmp8u;

        if (h_older_than(1, 5, 0, 0)) {
            a_ptr->floor_id = 0;

            rd_byte(&tmp8u);
            rd_byte(&tmp8u);
            rd_byte(&tmp8u);
        } else {
            rd_s16b(&a_ptr->floor_id);
        }
    }

    if (arg_fiddle)
        load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));

    rd_base_info(creature_ptr);
    rd_player_info(creature_ptr);
    rd_byte((byte *)&preserve_mode);
    rd_byte((byte *)&creature_ptr->wait_report_score);
    rd_dummy2();
    rd_global_configurations(creature_ptr);
    rd_extra(creature_ptr);

    if (creature_ptr->energy_need < -999)
        creature_ptr->timewalk = TRUE;

    if (arg_fiddle)
        load_note(_("特別情報をロードしました", "Loaded extra information"));

    rd_u16b(&tmp16u);
    if (tmp16u > PY_MAX_LEVEL) {
        load_note(format(_("ヒットポイント配列が大きすぎる(%u)！", "Too many (%u) hitpoint entries!"), tmp16u));
        return (25);
    }

    for (int i = 0; i < tmp16u; i++) {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        creature_ptr->player_hp[i] = (HIT_POINT)tmp16s;
    }

    sp_ptr = &sex_info[creature_ptr->psex];
    rp_ptr = &race_info[creature_ptr->prace];
    cp_ptr = &class_info[creature_ptr->pclass];
    ap_ptr = &personality_info[creature_ptr->pseikaku];

    if (z_older_than(10, 2, 2) && (creature_ptr->pclass == CLASS_BEASTMASTER) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, 0L);
    }

    if (z_older_than(10, 3, 2) && (creature_ptr->pclass == CLASS_ARCHER) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, 0L);
    }

    if (z_older_than(10, 2, 6) && (creature_ptr->pclass == CLASS_SORCERER) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, 0L);
    }

    if (z_older_than(10, 4, 7) && (creature_ptr->pclass == CLASS_BLUE_MAGE) && !creature_ptr->is_dead) {
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        roll_hitdice(creature_ptr, 0L);
    }

    mp_ptr = &m_info[creature_ptr->pclass];

    rd_u32b(&creature_ptr->spell_learned1);
    rd_u32b(&creature_ptr->spell_learned2);
    rd_u32b(&creature_ptr->spell_worked1);
    rd_u32b(&creature_ptr->spell_worked2);
    rd_u32b(&creature_ptr->spell_forgotten1);
    rd_u32b(&creature_ptr->spell_forgotten2);

    if (z_older_than(10, 0, 5)) {
        creature_ptr->learned_spells = 0;
        for (int i = 0; i < 64; i++) {
            if ((i < 32) ? (creature_ptr->spell_learned1 & (1L << i)) : (creature_ptr->spell_learned2 & (1L << (i - 32)))) {
                creature_ptr->learned_spells++;
            }
        }
    } else
        rd_s16b(&creature_ptr->learned_spells);

    if (z_older_than(10, 0, 6)) {
        creature_ptr->add_spells = 0;
    } else
        rd_s16b(&creature_ptr->add_spells);

    if (creature_ptr->pclass == CLASS_MINDCRAFTER)
        creature_ptr->add_spells = 0;

    for (int i = 0; i < 64; i++) {
        rd_byte(&tmp8u);
        creature_ptr->spell_order[i] = (SPELL_IDX)tmp8u;
    }

    if (rd_inventory(creature_ptr)) {
        load_note(_("持ち物情報を読み込むことができません", "Unable to read inventory"));
        return (21);
    }

    rd_u16b(&tmp16u);
    int town_count = tmp16u;

    rd_u16b(&tmp16u);
    for (int i = 1; i < town_count; i++) {
        for (int j = 0; j < tmp16u; j++) {
            if (rd_store(creature_ptr, i, j))
                return (22);
        }
    }

    rd_s16b(&creature_ptr->pet_follow_distance);
    if (z_older_than(10, 4, 10)) {
        creature_ptr->pet_extra_flags = 0;
        rd_byte(&tmp8u);
        if (tmp8u)
            creature_ptr->pet_extra_flags |= PF_OPEN_DOORS;
        rd_byte(&tmp8u);
        if (tmp8u)
            creature_ptr->pet_extra_flags |= PF_PICKUP_ITEMS;

        if (z_older_than(10, 0, 4))
            creature_ptr->pet_extra_flags |= PF_TELEPORT;
        else {
            rd_byte(&tmp8u);
            if (tmp8u)
                creature_ptr->pet_extra_flags |= PF_TELEPORT;
        }

        if (z_older_than(10, 0, 7))
            creature_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
        else {
            rd_byte(&tmp8u);
            if (tmp8u)
                creature_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
        }

        if (z_older_than(10, 0, 8))
            creature_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
        else {
            rd_byte(&tmp8u);
            if (tmp8u)
                creature_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
        }

        if (!z_older_than(10, 0, 8)) {
            rd_byte(&tmp8u);
            if (tmp8u)
                creature_ptr->pet_extra_flags |= PF_BALL_SPELL;
        }
    } else {
        rd_s16b(&creature_ptr->pet_extra_flags);
    }

    if (!z_older_than(11, 0, 9)) {
        char buf[SCREEN_BUF_MAX_SIZE];
        rd_string(buf, sizeof(buf));
        if (buf[0])
            screen_dump = string_make(buf);
    }

    if (creature_ptr->is_dead) {
        for (int i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++) {
            r_info[quest[i].r_idx].flags1 &= ~(RF1_QUESTOR);
        }
    }

    if (!creature_ptr->is_dead) {
        load_note(_("ダンジョン復元中...", "Restoring Dungeon..."));
        if (rd_dungeon(creature_ptr)) {
            load_note(_("ダンジョンデータ読み込み失敗", "Error reading dungeon data"));
            return (34);
        }

        rd_ghost();
        {
            s32b tmp32s;

            rd_s32b(&tmp32s);
            strip_bytes(tmp32s);
        }
    }

    /* Quest 18 was removed */
    if (h_older_than(1, 7, 0, 6)) {
        if (creature_ptr->current_floor_ptr->inside_quest == OLD_QUEST_WATER_CAVE) {
            creature_ptr->dungeon_idx = lite_town ? DUNGEON_ANGBAND : DUNGEON_GALGALS;
            creature_ptr->current_floor_ptr->dun_level = 1;
            creature_ptr->current_floor_ptr->inside_quest = 0;
        }
    }

    u32b n_v_check = v_check;
    u32b o_v_check;
    rd_u32b(&o_v_check);
    if (o_v_check != n_v_check) {
        load_note(_("チェックサムがおかしい", "Invalid checksum"));
        return 11;
    }

    u32b n_x_check = x_check;
    u32b o_x_check;
    rd_u32b(&o_x_check);
    if (o_x_check != n_x_check) {
        load_note(_("エンコードされたチェックサムがおかしい", "Invalid encoded checksum"));
        return 11;
    }

    return 0;
}

/*!
 * @brief セーブファイル読み込み処理 (UIDチェック等含む) / Reading the savefile (including UID check)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 */
errr rd_savefile(player_type *player_ptr)
{
    safe_setuid_grab(player_ptr);
    loading_savefile = angband_fopen(savefile, "rb");
    safe_setuid_drop();
    if (!loading_savefile)
        return -1;

    errr err = exe_reading_savefile(player_ptr);
    if (ferror(loading_savefile))
        err = -1;

    angband_fclose(loading_savefile);
    return err;
}
