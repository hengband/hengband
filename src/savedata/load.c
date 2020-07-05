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
#include "savedata/item-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-5-0.h"
#include "savedata/load-v1-7-0.h"
#include "savedata/lore-loader.h"
#include "savedata/load-zangband.h"
#include "savedata/option-loader.h"
#include "savedata/quest-loader.h"
#include "savedata/player-info-loader.h"
#include "savedata/store-loader.h"
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

static void rd_system_info(void)
{
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
}

/*!
 * @brief 変愚蛮怒 v2.1.3で追加された街とクエストについて読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 */
static errr load_town_quest(player_type *creature_ptr)
{
    if (z_older_than(12, 1, 3))
        return 0;

    errr load_town_result = load_town();
    if (load_town_result != 0)
        return load_town_result;

    u16b max_quests_load;
    byte max_rquests_load;
    errr load_quest_result = load_quest_info(&max_quests_load, &max_rquests_load);
    if (load_quest_result != 0)
        return load_quest_result;

    analyze_quests(creature_ptr, max_quests_load, max_rquests_load);

    /* Quest 18 was removed */
    if (h_older_than(1, 7, 0, 6)) {
        (void)WIPE(&quest[OLD_QUEST_WATER_CAVE], quest_type);
        quest[OLD_QUEST_WATER_CAVE].status = QUEST_STATUS_UNTAKEN;
    }

    load_wilderness_info(creature_ptr);
    return analyze_wilderness();
}

static void load_player_world(player_type *creature_ptr)
{
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
}

static errr load_hp(player_type *creature_ptr)
{
    u16b tmp16u;
    rd_u16b(&tmp16u);
    if (tmp16u > PY_MAX_LEVEL) {
        load_note(format(_("ヒットポイント配列が大きすぎる(%u)！", "Too many (%u) hitpoint entries!"), tmp16u));
        return 25;
    }

    for (int i = 0; i < tmp16u; i++) {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        creature_ptr->player_hp[i] = (HIT_POINT)tmp16s;
    }

    return 0;
}

static void load_spells(player_type *creature_ptr)
{
    rd_u32b(&creature_ptr->spell_learned1);
    rd_u32b(&creature_ptr->spell_learned2);
    rd_u32b(&creature_ptr->spell_worked1);
    rd_u32b(&creature_ptr->spell_worked2);
    rd_u32b(&creature_ptr->spell_forgotten1);
    rd_u32b(&creature_ptr->spell_forgotten2);

    if (z_older_than(10, 0, 5))
        set_zangband_learnt_spells(creature_ptr);
    else
        rd_s16b(&creature_ptr->learned_spells);

    if (z_older_than(10, 0, 6))
        creature_ptr->add_spells = 0;
    else
        rd_s16b(&creature_ptr->add_spells);
}

static errr verify_checksum()
{
    u32b n_v_check = v_check;
    u32b o_v_check;
    rd_u32b(&o_v_check);
    if (o_v_check == n_v_check)
        return 0;

    load_note(_("チェックサムがおかしい", "Invalid checksum"));
    return 11;
}

static errr verify_encoded_checksum()
{
    u32b n_x_check = x_check;
    u32b o_x_check;
    rd_u32b(&o_x_check);
    if (o_x_check == n_x_check)
        return 0;

    load_note(_("エンコードされたチェックサムがおかしい", "Invalid encoded checksum"));
    return 11;
}

/*!
 * @brief セーブファイル読み込み処理の実体 / Actually read the savefile
 * @return エラーコード
 */
static errr exe_reading_savefile(player_type *creature_ptr)
{
    rd_version_info();
    rd_dummy3();
    rd_system_info();
    rd_unique_info();
    errr load_lore_result = load_lore();
    if (load_lore_result != 0)
        return load_lore_result;

    errr load_item_result = load_item();
    if (load_item_result != 0)
        return load_item_result;

    errr load_town_quest_result = load_town_quest(creature_ptr);
    if (load_town_quest_result != 0)
        return load_town_quest_result;

    if (arg_fiddle)
        load_note(_("クエスト情報をロードしました", "Loaded Quests"));

    errr load_artifact_result = load_artifact();
    if (load_artifact_result != 0)
        return load_artifact_result;

    load_player_world(creature_ptr);
    errr load_hp_result = load_hp(creature_ptr);
    if (load_hp_result != 0)
        return load_hp_result;

    sp_ptr = &sex_info[creature_ptr->psex];
    rp_ptr = &race_info[creature_ptr->prace];
    cp_ptr = &class_info[creature_ptr->pclass];
    ap_ptr = &personality_info[creature_ptr->pseikaku];

    set_zangband_class(creature_ptr);
    mp_ptr = &m_info[creature_ptr->pclass];

    load_spells(creature_ptr);
    if (creature_ptr->pclass == CLASS_MINDCRAFTER)
        creature_ptr->add_spells = 0;

    errr load_inventory_result = load_inventory(creature_ptr);
    if (load_inventory_result != 0)
        return load_inventory_result;

    errr load_store_result = load_store(creature_ptr);
    if (load_store_result != 0)
        return load_store_result;

    rd_s16b(&creature_ptr->pet_follow_distance);
    if (z_older_than(10, 4, 10))
        set_zangband_pet(creature_ptr);
    else
        rd_s16b(&creature_ptr->pet_extra_flags);

    if (!z_older_than(11, 0, 9)) {
        char buf[SCREEN_BUF_MAX_SIZE];
        rd_string(buf, sizeof(buf));
        if (buf[0])
            screen_dump = string_make(buf);
    }

    errr restore_dungeon_result = restore_dungeon(creature_ptr);
    if (restore_dungeon_result != 0)
        return restore_dungeon_result;

    if (h_older_than(1, 7, 0, 6))
        remove_water_cave(creature_ptr);

    errr checksum_result = verify_checksum();
    if (checksum_result != 0)
        return checksum_result;

    return verify_encoded_checksum();
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
