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
#include "art-definition/art-accessory-types.h"
#include "birth/quick-start.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-item/cmd-smith.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-generate.h"
#include "floor/floor-save.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/option-flags.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-slot-types.h"
#include "io/report.h"
#include "io/uid-checker.h"
#include "locale/japanese.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "mind/mind-weaponsmith.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "mutation/mutation.h"
#include "object-enchant/artifact.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "pet/pet-util.h"
#include "player/avatar.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-sex.h"
#include "player/player-skill.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/item-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-5-0.h"
#include "savedata/load-zangband.h"
#include "savedata/monster-loader.h"
#include "savedata/old-feature-types.h"
#include "savedata/save.h"
#include "savedata/savedata-flag-types.h"
#include "spell/spells-status.h"
#include "store/store-util.h"
#include "store/store.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/angband-version.h"
#include "system/system-variables.h" // 暫定、init_flags の扱いを決めた上で消す.
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/object-sort.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "world/world-object.h"
#include "world/world.h"

/* Old hidden trap flag */
static const BIT_FLAGS CAVE_TRAP = 0x8000;

static const int OLD_QUEST_WATER_CAVE = 18; // 湖の洞窟.
static const int QUEST_OLD_CASTLE = 27; // 古い城.
static const int QUEST_ROYAL_CRYPT = 28; // 王家の墓.

/*!
 * @brief モンスターの思い出を読み込む / Read the monster lore
 * @param r_idx 読み込み先モンスターID
 * @return なし
 */
static void rd_lore(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];

    s16b tmp16s;
    rd_s16b(&tmp16s);
    r_ptr->r_sights = (MONSTER_NUMBER)tmp16s;

    rd_s16b(&tmp16s);
    r_ptr->r_deaths = (MONSTER_NUMBER)tmp16s;

    rd_s16b(&tmp16s);
    r_ptr->r_pkills = (MONSTER_NUMBER)tmp16s;

    if (h_older_than(1, 7, 0, 5)) {
        r_ptr->r_akills = r_ptr->r_pkills;
    } else {
        rd_s16b(&tmp16s);
        r_ptr->r_akills = (MONSTER_NUMBER)tmp16s;
    }

    rd_s16b(&tmp16s);
    r_ptr->r_tkills = (MONSTER_NUMBER)tmp16s;

    rd_byte(&r_ptr->r_wake);
    rd_byte(&r_ptr->r_ignore);
    rd_byte(&r_ptr->r_xtra1);
    rd_byte(&r_ptr->r_xtra2);

    byte tmp8u;
    rd_byte(&tmp8u);
    r_ptr->r_drop_gold = (ITEM_NUMBER)tmp8u;
    rd_byte(&tmp8u);
    r_ptr->r_drop_item = (ITEM_NUMBER)tmp8u;

    rd_byte(&tmp8u);
    rd_byte(&r_ptr->r_cast_spell);

    rd_byte(&r_ptr->r_blows[0]);
    rd_byte(&r_ptr->r_blows[1]);
    rd_byte(&r_ptr->r_blows[2]);
    rd_byte(&r_ptr->r_blows[3]);

    rd_u32b(&r_ptr->r_flags1);
    rd_u32b(&r_ptr->r_flags2);
    rd_u32b(&r_ptr->r_flags3);
    rd_u32b(&r_ptr->r_flags4);
    rd_u32b(&r_ptr->r_flags5);
    rd_u32b(&r_ptr->r_flags6);
    if (h_older_than(1, 5, 0, 3))
        set_old_lore(r_ptr, r_idx);
    else
        rd_u32b(&r_ptr->r_flagsr);

    rd_byte(&tmp8u);
    r_ptr->max_num = (MONSTER_NUMBER)tmp8u;

    rd_s16b(&r_ptr->floor_id);
    rd_byte(&tmp8u);

    r_ptr->r_flags1 &= r_ptr->flags1;
    r_ptr->r_flags2 &= r_ptr->flags2;
    r_ptr->r_flags3 &= r_ptr->flags3;
    r_ptr->r_flags4 &= r_ptr->flags4;
    r_ptr->r_flags5 &= r_ptr->a_ability_flags1;
    r_ptr->r_flags6 &= r_ptr->a_ability_flags2;
    r_ptr->r_flagsr &= r_ptr->flagsr;
}

/*!
 * @brief 店置きのアイテムオブジェクトを読み込む / Add the item "o_ptr" to the inventory of the "Home"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param store_ptr 店舗の参照ポインタ
 * @param o_ptr アイテムオブジェクト参照ポインタ
 * @return なし
 * @details
 * In all cases, return the slot (or -1) where the object was placed
 *
 * Note that this is a hacked up version of "store_item_to_inventory()".
 *
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 */
static void home_carry(player_type *player_ptr, store_type *store_ptr, object_type *o_ptr)
{
    for (int i = 0; i < store_ptr->stock_num; i++) {
        object_type *j_ptr;
        j_ptr = &store_ptr->stock[i];
        if (!object_similar(j_ptr, o_ptr))
            continue;

        object_absorb(j_ptr, o_ptr);
        return;
    }

    if (store_ptr->stock_num >= STORE_INVEN_MAX * 10)
        return;

    s32b value = object_value(player_ptr, o_ptr);
    int slot;
    for (slot = 0; slot < store_ptr->stock_num; slot++) {
        if (object_sort_comp(player_ptr, o_ptr, value, &store_ptr->stock[slot]))
            break;
    }

    for (int i = store_ptr->stock_num; i > slot; i--) {
        store_ptr->stock[i] = store_ptr->stock[i - 1];
    }

    store_ptr->stock_num++;
    store_ptr->stock[slot] = *o_ptr;
    chg_virtue(player_ptr, V_SACRIFICE, -1);
}

/*!
 * @brief 店舗情報を読み込む / Read a store
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param town_number 街ID
 * @param store_number 店舗ID
 * @return エラーID
 */
static errr rd_store(player_type *player_ptr, int town_number, int store_number)
{
    store_type *store_ptr;
    bool sort = FALSE;
    if (z_older_than(10, 3, 3) && (store_number == STORE_HOME)) {
        store_ptr = &town_info[1].store[store_number];
        if (store_ptr->stock_num)
            sort = TRUE;
    } else {
        store_ptr = &town_info[town_number].store[store_number];
    }

    byte own;
    byte tmp8u;
    s16b num;
    rd_s32b(&store_ptr->store_open);
    rd_s16b(&store_ptr->insult_cur);
    rd_byte(&own);
    if (z_older_than(11, 0, 4)) {
        rd_byte(&tmp8u);
        num = tmp8u;
    } else {
        rd_s16b(&num);
    }

    rd_s16b(&store_ptr->good_buy);
    rd_s16b(&store_ptr->bad_buy);

    rd_s32b(&store_ptr->last_visit);
    store_ptr->owner = own;

    for (int j = 0; j < num; j++) {
        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        object_wipe(q_ptr);

        rd_item(player_ptr, q_ptr);

        bool is_valid_item = store_ptr->stock_num
            < (store_number == STORE_HOME ? STORE_INVEN_MAX * 10 : store_number == STORE_MUSEUM ? STORE_INVEN_MAX * 50 : STORE_INVEN_MAX);
        if (!is_valid_item)
            continue;

        if (sort) {
            home_carry(player_ptr, store_ptr, q_ptr);
        } else {
            int k = store_ptr->stock_num++;
            object_copy(&store_ptr->stock[k], q_ptr);
        }
    }

    return 0;
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
    for (int i = 0; i < RAND_DEG; i++) {
        rd_u32b(&Rand_state[i]);
    }
}

/*!
 * @brief ゲームオプションを読み込む / Read options (ignore most pre-2.8.0 options)
 * @return なし
 * @details
 * Note that the normal options are now stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
static void rd_options(void)
{
    strip_bytes(16);

    byte b;
    rd_byte(&b);
    delay_factor = b;

    rd_byte(&b);
    hitpoint_warn = b;

    if (h_older_than(1, 7, 0, 0)) {
        mana_warn = 2;
    } else {
        rd_byte(&b);
        mana_warn = b;
    }

    u16b c;
    rd_u16b(&c);

    if (c & 0x0002)
        current_world_ptr->wizard = TRUE;

    cheat_peek = (c & 0x0100) ? TRUE : FALSE;
    cheat_hear = (c & 0x0200) ? TRUE : FALSE;
    cheat_room = (c & 0x0400) ? TRUE : FALSE;
    cheat_xtra = (c & 0x0800) ? TRUE : FALSE;
    cheat_know = (c & 0x1000) ? TRUE : FALSE;
    cheat_live = (c & 0x2000) ? TRUE : FALSE;
    cheat_save = (c & 0x4000) ? TRUE : FALSE;
    cheat_diary_output = (c & 0x8000) ? TRUE : FALSE;
    cheat_turn = (c & 0x0080) ? TRUE : FALSE;
    cheat_sight = (c & 0x0040) ? TRUE : FALSE;

    rd_byte((byte *)&autosave_l);
    rd_byte((byte *)&autosave_t);
    rd_s16b(&autosave_freq);

    BIT_FLAGS flag[8];
    for (int n = 0; n < 8; n++)
        rd_u32b(&flag[n]);

    BIT_FLAGS mask[8];
    for (int n = 0; n < 8; n++)
        rd_u32b(&mask[n]);

    for (int n = 0; n < 8; n++) {
        for (int i = 0; i < 32; i++) {
            if (!(mask[n] & (1L << i)))
                continue;
            if (!(option_mask[n] & (1L << i)))
                continue;

            if (flag[n] & (1L << i)) {
                option_flag[n] |= (1L << i);
            } else {
                option_flag[n] &= ~(1L << i);
            }
        }
    }

    if (z_older_than(10, 4, 5))
        load_zangband_options();

    extract_option_vars();
    for (int n = 0; n < 8; n++)
        rd_u32b(&flag[n]);

    for (int n = 0; n < 8; n++)
        rd_u32b(&mask[n]);

    for (int n = 0; n < 8; n++) {
        for (int i = 0; i < 32; i++) {
            if (!(mask[n] & (1L << i)))
                continue;
            if (!(window_mask[n] & (1L << i)))
                continue;

            if (flag[n] & (1L << i)) {
                window_flag[n] |= (1L << i);
            } else {
                window_flag[n] &= ~(1L << i);
            }
        }
    }
}

/*!
 * @brief ダミー情報スキップ / Hack -- strip the "ghost" info
 * @return なし
 * @details
 * This is such a nasty hack it hurts.
 */
static void rd_ghost(void)
{
    char buf[64];
    rd_string(buf, sizeof(buf));
    strip_bytes(60);
}

/*!
 * @brief クイックスタート情報を読み込む / Load quick start data
 * @return なし
 */
static void load_quick_start(void)
{
    if (z_older_than(11, 0, 13)) {
        previous_char.quick_ok = FALSE;
        return;
    }

    rd_byte(&previous_char.psex);
    byte tmp8u;
    rd_byte(&tmp8u);
    previous_char.prace = (player_race_type)tmp8u;
    rd_byte(&tmp8u);
    previous_char.pclass = (player_class_type)tmp8u;
    rd_byte(&tmp8u);
    previous_char.pseikaku = (player_personality_type)tmp8u;
    rd_byte(&tmp8u);
    previous_char.realm1 = (REALM_IDX)tmp8u;
    rd_byte(&tmp8u);
    previous_char.realm2 = (REALM_IDX)tmp8u;

    rd_s16b(&previous_char.age);
    rd_s16b(&previous_char.ht);
    rd_s16b(&previous_char.wt);
    rd_s16b(&previous_char.sc);
    rd_s32b(&previous_char.au);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&previous_char.stat_max[i]);
    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&previous_char.stat_max_max[i]);

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        previous_char.player_hp[i] = (HIT_POINT)tmp16s;
    }

    rd_s16b(&previous_char.chaos_patron);

    for (int i = 0; i < 8; i++)
        rd_s16b(&previous_char.vir_types[i]);

    for (int i = 0; i < 4; i++)
        rd_string(previous_char.history[i], sizeof(previous_char.history[i]));

    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
    previous_char.quick_ok = (bool)tmp8u;
}

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @return なし
 */
static void rd_extra(player_type *creature_ptr)
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
    if (z_older_than(10, 4, 4)) {
        if (creature_ptr->realm1 == 9)
            creature_ptr->realm1 = REALM_MUSIC;
        if (creature_ptr->realm2 == 9)
            creature_ptr->realm2 = REALM_MUSIC;
        if (creature_ptr->realm1 == 10)
            creature_ptr->realm1 = REALM_HISSATSU;
        if (creature_ptr->realm2 == 10)
            creature_ptr->realm2 = REALM_HISSATSU;
    }

    rd_byte(&tmp8u);
    creature_ptr->hitdie = tmp8u;
    rd_u16b(&creature_ptr->expfact);

    rd_s16b(&creature_ptr->age);
    rd_s16b(&creature_ptr->ht);
    rd_s16b(&creature_ptr->wt);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_max[i]);
    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_max_max[i]);
    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_cur[i]);

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
    if ((creature_ptr->pclass == CLASS_SORCERER) && z_older_than(10, 4, 2)) {
        for (int i = 0; i < 64; i++)
            creature_ptr->spell_exp[i] = SPELL_EXP_MASTER;
    }

    if (z_older_than(10, 3, 6))
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 60; j++)
                rd_s16b(&creature_ptr->weapon_exp[i][j]);
    else
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 64; j++)
                rd_s16b(&creature_ptr->weapon_exp[i][j]);
    for (int i = 0; i < GINOU_MAX; i++)
        rd_s16b(&creature_ptr->skill_exp[i]);
    if (z_older_than(10, 4, 1)) {
        if (creature_ptr->pclass != CLASS_BEASTMASTER)
            creature_ptr->skill_exp[GINOU_RIDING] /= 2;
        creature_ptr->skill_exp[GINOU_RIDING] = MIN(creature_ptr->skill_exp[GINOU_RIDING], s_info[creature_ptr->pclass].s_max[GINOU_RIDING]);
    }

    if (z_older_than(10, 3, 14)) {
        for (int i = 0; i < 108; i++)
            creature_ptr->magic_num1[i] = 0;
        for (int i = 0; i < 108; i++)
            creature_ptr->magic_num2[i] = 0;
    } else {
        for (int i = 0; i < 108; i++)
            rd_s32b(&creature_ptr->magic_num1[i]);
        for (int i = 0; i < 108; i++)
            rd_byte(&creature_ptr->magic_num2[i]);
        if (h_older_than(1, 3, 0, 1)) {
            if (creature_ptr->pclass == CLASS_SMITH) {
                creature_ptr->magic_num1[TR_ES_ATTACK] = creature_ptr->magic_num1[96];
                creature_ptr->magic_num1[96] = 0;
                creature_ptr->magic_num1[TR_ES_AC] = creature_ptr->magic_num1[97];
                creature_ptr->magic_num1[97] = 0;
            }
        }
    }

    if (music_singing_any(creature_ptr))
        creature_ptr->action = ACTION_SING;

    if (z_older_than(11, 0, 7)) {
        creature_ptr->start_race = creature_ptr->prace;
        creature_ptr->old_race1 = 0L;
        creature_ptr->old_race2 = 0L;
        creature_ptr->old_realm = 0;
    } else {
        rd_byte(&tmp8u);
        creature_ptr->start_race = (player_race_type)tmp8u;
        s32b tmp32s;
        rd_s32b(&tmp32s);
        creature_ptr->old_race1 = (BIT_FLAGS)tmp32s;
        rd_s32b(&tmp32s);
        creature_ptr->old_race2 = (BIT_FLAGS)tmp32s;
        rd_s16b(&creature_ptr->old_realm);
    }

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

/*!
 * @brief プレイヤーの所持品情報を読み込む / Read the player inventory
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Note that the inventory changed in Angband 2.7.4.  Two extra
 * pack slots were added and the equipment was rearranged.  Note
 * that these two features combine when parsing old save-files, in
 * which items from the old "aux" slot are "carried", perhaps into
 * one of the two new "inventory" slots.
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(player_type *player_ptr)
{
    player_ptr->total_weight = 0;
    player_ptr->inven_cnt = 0;
    player_ptr->equip_cnt = 0;

    if (player_ptr->inventory_list != NULL)
        C_WIPE(player_ptr->inventory_list, INVEN_TOTAL, object_type);
    C_MAKE(player_ptr->inventory_list, INVEN_TOTAL, object_type);

    int slot = 0;
    while (TRUE) {
        u16b n;
        rd_u16b(&n);

        if (n == 0xFFFF)
            break;
        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        object_wipe(q_ptr);

        rd_item(player_ptr, q_ptr);
        if (!q_ptr->k_idx)
            return (53);

        if (n >= INVEN_RARM) {
            q_ptr->marked |= OM_TOUCHED;
            object_copy(&player_ptr->inventory_list[n], q_ptr);
            player_ptr->total_weight += (q_ptr->number * q_ptr->weight);
            player_ptr->equip_cnt++;
            continue;
        }

        if (player_ptr->inven_cnt == INVEN_PACK) {
            load_note(_("持ち物の中のアイテムが多すぎる！", "Too many items in the inventory"));
            return (54);
        }

        n = slot++;
        q_ptr->marked |= OM_TOUCHED;
        object_copy(&player_ptr->inventory_list[n], q_ptr);
        player_ptr->total_weight += (q_ptr->number * q_ptr->weight);
        player_ptr->inven_cnt++;
    }

    return 0;
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
 * @brief メッセージログを読み込む / Read the dungeon (old method)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
static errr rd_dungeon_old(player_type *player_ptr)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->dun_level = (DEPTH)tmp16s;
    if (z_older_than(10, 3, 8))
        player_ptr->dungeon_idx = DUNGEON_ANGBAND;
    else {
        byte tmp8u;
        rd_byte(&tmp8u);
        player_ptr->dungeon_idx = (IDX)tmp8u;
    }

    floor_ptr->base_level = floor_ptr->dun_level;
    rd_s16b(&tmp16s);
    floor_ptr->base_level = (DEPTH)tmp16s;

    rd_s16b(&tmp16s);
    floor_ptr->num_repro = (MONSTER_NUMBER)tmp16s;
    rd_s16b(&tmp16s);
    player_ptr->y = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    player_ptr->x = (POSITION)tmp16s;
    if (z_older_than(10, 3, 13) && !floor_ptr->dun_level && !floor_ptr->inside_arena) {
        player_ptr->y = 33;
        player_ptr->x = 131;
    }
    rd_s16b(&tmp16s);
    floor_ptr->height = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->width = (POSITION)tmp16s;
    rd_s16b(&tmp16s); /* max_panel_rows */
    rd_s16b(&tmp16s); /* max_panel_cols */

    int ymax = floor_ptr->height;
    int xmax = floor_ptr->width;

    for (int x = 0, y = 0; y < ymax;) {
        u16b info;
        byte count;
        rd_byte(&count);
        if (z_older_than(10, 3, 6)) {
            byte tmp8u;
            rd_byte(&tmp8u);
            info = (u16b)tmp8u;
        } else {
            rd_u16b(&info);
            info &= ~(CAVE_LITE | CAVE_VIEW | CAVE_MNLT | CAVE_MNDK);
        }

        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info = info;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    for (int x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);
        byte tmp8u;
        rd_byte(&tmp8u);
        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->feat = (s16b)tmp8u;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    for (int x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);
        byte tmp8u;
        rd_byte(&tmp8u);
        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->mimic = (s16b)tmp8u;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    for (int x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);
        rd_s16b(&tmp16s);
        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->special = tmp16s;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    if (z_older_than(11, 0, 99)) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            }
        }
    }

    if (h_older_than(1, 1, 1, 0)) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];

                /* Very old */
                if (g_ptr->feat == OLD_FEAT_INVIS) {
                    g_ptr->feat = feat_floor;
                    g_ptr->info |= CAVE_TRAP;
                }

                /* Older than 1.1.1 */
                if (g_ptr->feat == OLD_FEAT_MIRROR) {
                    g_ptr->feat = feat_floor;
                    g_ptr->info |= CAVE_OBJECT;
                }
            }
        }
    }

    if (h_older_than(1, 3, 1, 0)) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];

                /* Old CAVE_IN_MIRROR flag */
                if (g_ptr->info & CAVE_OBJECT) {
                    g_ptr->mimic = feat_mirror;
                } else if ((g_ptr->feat == OLD_FEAT_MINOR_GLYPH) || (g_ptr->feat == OLD_FEAT_GLYPH)) {
                    g_ptr->info |= CAVE_OBJECT;
                    g_ptr->mimic = g_ptr->feat;
                    g_ptr->feat = feat_floor;
                } else if (g_ptr->info & CAVE_TRAP) {
                    g_ptr->info &= ~CAVE_TRAP;
                    g_ptr->mimic = g_ptr->feat;
                    g_ptr->feat = choose_random_trap(player_ptr);
                } else if (g_ptr->feat == OLD_FEAT_INVIS) {
                    g_ptr->mimic = feat_floor;
                    g_ptr->feat = feat_trap_open;
                }
            }
        }
    }

    /* Quest 18 was removed */
    if (h_older_than(1, 7, 0, 6) && !vanilla_town) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];

                if ((g_ptr->special == OLD_QUEST_WATER_CAVE) && !floor_ptr->dun_level) {
                    if (g_ptr->feat == OLD_FEAT_QUEST_ENTER) {
                        g_ptr->feat = feat_tree;
                        g_ptr->special = 0;
                    } else if (g_ptr->feat == OLD_FEAT_BLDG_1) {
                        g_ptr->special = lite_town ? QUEST_OLD_CASTLE : QUEST_ROYAL_CRYPT;
                    }
                } else if ((g_ptr->feat == OLD_FEAT_QUEST_EXIT) && (floor_ptr->inside_quest == OLD_QUEST_WATER_CAVE)) {
                    g_ptr->feat = feat_up_stair;
                    g_ptr->special = 0;
                }
            }
        }
    }

    u16b limit;
    rd_u16b(&limit);
    if (limit > current_world_ptr->max_o_idx) {
        load_note(format(_("アイテムの配列が大きすぎる(%d)！", "Too many (%d) object entries!"), limit));
        return (151);
    }

    for (int i = 1; i < limit; i++) {
        OBJECT_IDX o_idx = o_pop(floor_ptr);
        if (i != o_idx) {
            load_note(format(_("アイテム配置エラー (%d <> %d)", "Object allocation error (%d <> %d)"), i, o_idx));
            return (152);
        }

        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[o_idx];
        rd_item(player_ptr, o_ptr);
        if (object_is_held_monster(o_ptr)) {
            monster_type *m_ptr;
            m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
            o_ptr->next_o_idx = m_ptr->hold_o_idx;
            m_ptr->hold_o_idx = o_idx;
            continue;
        }

        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[o_ptr->iy][o_ptr->ix];
        o_ptr->next_o_idx = g_ptr->o_idx;
        g_ptr->o_idx = o_idx;
    }

    rd_u16b(&limit);
    if (limit > current_world_ptr->max_m_idx) {
        load_note(format(_("モンスターの配列が大きすぎる(%d)！", "Too many (%d) monster entries!"), limit));
        return (161);
    }

    for (int i = 1; i < limit; i++) {
        MONSTER_IDX m_idx;
        monster_type *m_ptr;
        m_idx = m_pop(floor_ptr);
        if (i != m_idx) {
            load_note(format(_("モンスター配置エラー (%d <> %d)", "Monster allocation error (%d <> %d)"), i, m_idx));
            return (162);
        }

        m_ptr = &floor_ptr->m_list[m_idx];
        rd_monster(player_ptr, m_ptr);
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[m_ptr->fy][m_ptr->fx];
        g_ptr->m_idx = m_idx;
        real_r_ptr(m_ptr)->cur_num++;
    }

    if (z_older_than(10, 3, 13) && !floor_ptr->dun_level && !floor_ptr->inside_arena)
        current_world_ptr->character_dungeon = FALSE;
    else
        current_world_ptr->character_dungeon = TRUE;

    return 0;
}

/*!
 * @brief 保存されたフロアを読み込む / Read the saved floor
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 最後に保存されたフロアへの参照ポインタ
 * @return info読み込みエラーコード
 * @details
 * この関数は、セーブデータの互換性を保つために多くのデータ改変処理を備えている。
 * 現在確認している処理は以下の通り、
 * <ul>
 * <li>1.7.0.2で8bitだったgrid_typeのfeat,mimicのID値を16bitに拡張する処理。</li>
 * <li>1.7.0.8までに廃止、IDなどを差し替えたクエスト番号を置換する処理。</li>
 * </ul>
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
static errr rd_saved_floor(player_type *player_ptr, saved_floor_type *sf_ptr)
{
    grid_template_type *templates;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    clear_cave(player_ptr);
    player_ptr->x = player_ptr->y = 0;

    if (!sf_ptr) {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        floor_ptr->dun_level = (DEPTH)tmp16s;
        floor_ptr->base_level = floor_ptr->dun_level;
    } else {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->floor_id)
            return 171;

        byte tmp8u;
        rd_byte(&tmp8u);
        if (tmp8u != sf_ptr->savefile_id)
            return 171;

        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->dun_level)
            return 171;
        floor_ptr->dun_level = sf_ptr->dun_level;

        s32b tmp32s;
        rd_s32b(&tmp32s);
        if (tmp32s != sf_ptr->last_visit)
            return 171;

        u32b tmp32u;
        rd_u32b(&tmp32u);
        if (tmp32u != sf_ptr->visit_mark)
            return 171;

        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->upper_floor_id)
            return 171;

        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->lower_floor_id)
            return 171;
    }

    s16b tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->base_level = (DEPTH)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->num_repro = (MONSTER_NUMBER)tmp16s;

    u16b tmp16u;
    rd_u16b(&tmp16u);
    player_ptr->y = (POSITION)tmp16u;

    rd_u16b(&tmp16u);
    player_ptr->x = (POSITION)tmp16u;

    rd_s16b(&tmp16s);
    floor_ptr->height = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->width = (POSITION)tmp16s;

    rd_byte(&player_ptr->feeling);

    u16b limit;
    rd_u16b(&limit);
    C_MAKE(templates, limit, grid_template_type);

    for (int i = 0; i < limit; i++) {
        grid_template_type *ct_ptr = &templates[i];
        rd_u16b(&tmp16u);
        ct_ptr->info = (BIT_FLAGS)tmp16u;
        if (h_older_than(1, 7, 0, 2)) {
            byte tmp8u;
            rd_byte(&tmp8u);
            ct_ptr->feat = (s16b)tmp8u;
            rd_byte(&tmp8u);
            ct_ptr->mimic = (s16b)tmp8u;
        } else {
            rd_s16b(&ct_ptr->feat);
            rd_s16b(&ct_ptr->mimic);
        }

        rd_s16b(&ct_ptr->special);
    }

    POSITION ymax = floor_ptr->height;
    POSITION xmax = floor_ptr->width;
    for (POSITION x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);

        u16b id = 0;
        byte tmp8u;
        do {
            rd_byte(&tmp8u);
            id += tmp8u;
        } while (tmp8u == MAX_UCHAR);

        for (int i = count; i > 0; i--) {
            grid_type *g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info = templates[id].info;
            g_ptr->feat = templates[id].feat;
            g_ptr->mimic = templates[id].mimic;
            g_ptr->special = templates[id].special;

            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    /* Quest 18 was removed */
    if (h_older_than(1, 7, 0, 6) && !vanilla_town) {
        for (POSITION y = 0; y < ymax; y++) {
            for (POSITION x = 0; x < xmax; x++) {
                grid_type *g_ptr = &floor_ptr->grid_array[y][x];

                if ((g_ptr->special == OLD_QUEST_WATER_CAVE) && !floor_ptr->dun_level) {
                    if (g_ptr->feat == OLD_FEAT_QUEST_ENTER) {
                        g_ptr->feat = feat_tree;
                        g_ptr->special = 0;
                    } else if (g_ptr->feat == OLD_FEAT_BLDG_1) {
                        g_ptr->special = lite_town ? QUEST_OLD_CASTLE : QUEST_ROYAL_CRYPT;
                    }
                } else if ((g_ptr->feat == OLD_FEAT_QUEST_EXIT) && (floor_ptr->inside_quest == OLD_QUEST_WATER_CAVE)) {
                    g_ptr->feat = feat_up_stair;
                    g_ptr->special = 0;
                }
            }
        }
    }

    C_KILL(templates, limit, grid_template_type);
    rd_u16b(&limit);
    if (limit > current_world_ptr->max_o_idx)
        return 151;
    for (int i = 1; i < limit; i++) {
        OBJECT_IDX o_idx;
        object_type *o_ptr;
        o_idx = o_pop(floor_ptr);
        if (i != o_idx)
            return 152;

        o_ptr = &floor_ptr->o_list[o_idx];
        rd_item(player_ptr, o_ptr);

        if (object_is_held_monster(o_ptr)) {
            monster_type *m_ptr;
            m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
            o_ptr->next_o_idx = m_ptr->hold_o_idx;
            m_ptr->hold_o_idx = o_idx;
        } else {
            grid_type *g_ptr = &floor_ptr->grid_array[o_ptr->iy][o_ptr->ix];
            o_ptr->next_o_idx = g_ptr->o_idx;
            g_ptr->o_idx = o_idx;
        }
    }

    rd_u16b(&limit);
    if (limit > current_world_ptr->max_m_idx)
        return 161;

    for (int i = 1; i < limit; i++) {
        grid_type *g_ptr;
        MONSTER_IDX m_idx;
        monster_type *m_ptr;
        m_idx = m_pop(floor_ptr);
        if (i != m_idx)
            return 162;

        m_ptr = &floor_ptr->m_list[m_idx];
        rd_monster(player_ptr, m_ptr);
        g_ptr = &floor_ptr->grid_array[m_ptr->fy][m_ptr->fx];
        g_ptr->m_idx = m_idx;
        real_r_ptr(m_ptr)->cur_num++;
    }

    return 0;
}

/*!
 * @brief 保存されたフロアを読み込む(現版) / Read the dungeon (new method)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 * @details
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
static errr rd_dungeon(player_type *player_ptr)
{
    init_saved_floors(player_ptr, FALSE);
    errr err = 0;
    if (h_older_than(1, 5, 0, 0)) {
        err = rd_dungeon_old(player_ptr);
        if (player_ptr->dungeon_idx) {
            player_ptr->floor_id = get_new_floor_id(player_ptr);
            get_sf_ptr(player_ptr->floor_id)->dun_level = player_ptr->current_floor_ptr->dun_level;
        }

        return err;
    }

    rd_s16b(&max_floor_id);
    byte tmp8u;
    rd_byte(&tmp8u);
    player_ptr->dungeon_idx = (DUNGEON_IDX)tmp8u;
    byte num;
    rd_byte(&num);
    if (num == 0) {
        err = rd_saved_floor(player_ptr, NULL);
    } else {
        for (int i = 0; i < num; i++) {
            saved_floor_type *sf_ptr = &saved_floors[i];

            rd_s16b(&sf_ptr->floor_id);
            rd_byte(&tmp8u);
            sf_ptr->savefile_id = (s16b)tmp8u;

            s16b tmp16s;
            rd_s16b(&tmp16s);
            sf_ptr->dun_level = (DEPTH)tmp16s;

            rd_s32b(&sf_ptr->last_visit);
            rd_u32b(&sf_ptr->visit_mark);
            rd_s16b(&sf_ptr->upper_floor_id);
            rd_s16b(&sf_ptr->lower_floor_id);
        }

        for (int i = 0; i < num; i++) {
            saved_floor_type *sf_ptr = &saved_floors[i];
            if (!sf_ptr->floor_id)
                continue;
            rd_byte(&tmp8u);
            if (tmp8u)
                continue;

            err = rd_saved_floor(player_ptr, sf_ptr);
            if (err)
                break;

            if (!save_floor(player_ptr, sf_ptr, SLF_SECOND))
                err = 182;

            if (err)
                break;
        }

        if (err == 0) {
            if (!load_floor(player_ptr, get_sf_ptr(player_ptr->floor_id), SLF_SECOND))
                err = 183;
        }
    }

    switch (err) {
    case 151:
        load_note(_("アイテムの配列が大きすぎる！", "Too many object entries!"));
        break;

    case 152:
        load_note(_("アイテム配置エラー", "Object allocation error"));
        break;

    case 161:
        load_note(_("モンスターの配列が大きすぎる！", "Too many monster entries!"));
        break;

    case 162:
        load_note(_("モンスター配置エラー", "Monster allocation error"));
        break;

    case 171:
        load_note(_("保存されたフロアのダンジョンデータが壊れています！", "Dungeon data of saved floors are broken!"));
        break;

    case 182:
        load_note(_("テンポラリ・ファイルを作成できません！", "Failed to make temporary files!"));
        break;

    case 183:
        load_note(_("Error 183", "Error 183"));
        break;
    }

    current_world_ptr->character_dungeon = TRUE;
    return err;
}

/*!
 * @brief ロード処理全体のサブ関数 / Actually read the savefile
 * @return エラーコード
 */
static errr rd_savefile_new_aux(player_type *creature_ptr)
{
    u32b n_x_check, n_v_check;
    u32b o_x_check, o_v_check;

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

    n_v_check = v_check;
    rd_u32b(&o_v_check);
    if (o_v_check != n_v_check) {
        load_note(_("チェックサムがおかしい", "Invalid checksum"));
        return 11;
    }

    n_x_check = x_check;
    rd_u32b(&o_x_check);
    if (o_x_check != n_x_check) {
        load_note(_("エンコードされたチェックサムがおかしい", "Invalid encoded checksum"));
        return 11;
    }

    return 0;
}

/*!
 * @brief ロード処理全体のメイン関数 / Actually read the savefile
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return エラーコード
 */
errr rd_savefile_new(player_type *player_ptr)
{
    safe_setuid_grab(player_ptr);
    loading_savefile = angband_fopen(savefile, "rb");
    safe_setuid_drop();
    if (!loading_savefile)
        return -1;
    errr err = rd_savefile_new_aux(player_ptr);

    if (ferror(loading_savefile))
        err = -1;
    angband_fclose(loading_savefile);
    return err;
}

/*!
 * @brief 保存フロア読み込みのサブ関数 / Actually load and verify a floor save data
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 保存フロア読み込み先
 * @return 成功したらtrue
 */
static bool load_floor_aux(player_type *player_ptr, saved_floor_type *sf_ptr)
{
    u32b n_x_check, n_v_check;
    u32b o_x_check, o_v_check;

    load_xor_byte = 0;
    byte tmp8u;
    rd_byte(&tmp8u);

    v_check = 0L;
    x_check = 0L;

    current_world_ptr->h_ver_extra = H_VER_EXTRA;
    current_world_ptr->h_ver_patch = H_VER_PATCH;
    current_world_ptr->h_ver_minor = H_VER_MINOR;
    current_world_ptr->h_ver_major = H_VER_MAJOR;

    u32b tmp32u;
    rd_u32b(&tmp32u);
    if (saved_floor_file_sign != tmp32u)
        return FALSE;

    if (rd_saved_floor(player_ptr, sf_ptr))
        return FALSE;

    n_v_check = v_check;
    rd_u32b(&o_v_check);

    if (o_v_check != n_v_check)
        return FALSE;

    n_x_check = x_check;
    rd_u32b(&o_x_check);

    if (o_x_check != n_x_check)
        return FALSE;
    return TRUE;
}

/*!
 * @brief 一時保存フロア情報を読み込む / Attempt to load the temporarily saved-floor data
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 保存フロア読み込み先
 * @param mode オプション
 * @return 成功したらtrue
 */
bool load_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode)
{
    /*
     * Temporary files are always written in system depended kanji
     * code.
     */
#ifdef JP
#ifdef EUC
    kanji_code = 2;
#endif
#ifdef SJIS
    kanji_code = 3;
#endif
#else
    kanji_code = 1;
#endif

    FILE *old_fff = NULL;
    byte old_xor_byte = 0;
    u32b old_v_check = 0;
    u32b old_x_check = 0;
    byte old_h_ver_major = 0;
    byte old_h_ver_minor = 0;
    byte old_h_ver_patch = 0;
    byte old_h_ver_extra = 0;
    if (mode & SLF_SECOND) {
        old_fff = loading_savefile;
        old_xor_byte = load_xor_byte;
        old_v_check = v_check;
        old_x_check = x_check;
        old_h_ver_major = current_world_ptr->h_ver_major;
        old_h_ver_minor = current_world_ptr->h_ver_minor;
        old_h_ver_patch = current_world_ptr->h_ver_patch;
        old_h_ver_extra = current_world_ptr->h_ver_extra;
    }

    char floor_savefile[1024];
    sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);

    safe_setuid_grab(player_ptr);
    loading_savefile = angband_fopen(floor_savefile, "rb");
    safe_setuid_drop();

    bool is_save_successful = TRUE;
    if (!loading_savefile)
        is_save_successful = FALSE;

    if (is_save_successful) {
        is_save_successful = load_floor_aux(player_ptr, sf_ptr);
        if (ferror(loading_savefile))
            is_save_successful = FALSE;
        angband_fclose(loading_savefile);

        safe_setuid_grab(player_ptr);
        if (!(mode & SLF_NO_KILL))
            (void)fd_kill(floor_savefile);

        safe_setuid_drop();
    }

    if (mode & SLF_SECOND) {
        loading_savefile = old_fff;
        load_xor_byte = old_xor_byte;
        v_check = old_v_check;
        x_check = old_x_check;
        current_world_ptr->h_ver_major = old_h_ver_major;
        current_world_ptr->h_ver_minor = old_h_ver_minor;
        current_world_ptr->h_ver_patch = old_h_ver_patch;
        current_world_ptr->h_ver_extra = old_h_ver_extra;
    }

    byte old_kanji_code = kanji_code;
    kanji_code = old_kanji_code;
    return is_save_successful;
}
