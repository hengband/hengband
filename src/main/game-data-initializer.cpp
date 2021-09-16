/*!
 * @file game-data-initializer.cpp
 * @brief 変愚蛮怒のゲームデータ初期化定義
 */

#include "main/game-data-initializer.h"
#include "cmd-io/macro-util.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-util.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "monster-race/monster-race.h"
#include "object/object-kind.h"
#include "system/alloc-entries.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "util/angband-files.h"
#include "util/quarks.h"
#include "util/tag-sorter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief マクロ登録の最大数 / Maximum number of macros (see "io.c")
 * @note Default: assume at most 256 macros are used
 */
static const int MACRO_MAX = 256;

/*!
 * @brief クエスト情報初期化のメインルーチン /
 * Initialize quest array
 * @return エラーコード
 */
errr init_quests(void)
{
    C_MAKE(quest, max_q_idx, quest_type);
    for (int i = 0; i < max_q_idx; i++)
        quest[i].status = QUEST_STATUS_UNTAKEN;

    return 0;
}

/*!
 * @brief その他の初期情報更新 /
 * Initialize some other arrays
 * @return エラーコード
 */
errr init_other(player_type *player_ptr)
{
    player_ptr->current_floor_ptr = &floor_info; // TODO:本当はこんなところで初期化したくない
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    C_MAKE(floor_ptr->o_list, w_ptr->max_o_idx, object_type);
    C_MAKE(floor_ptr->m_list, w_ptr->max_m_idx, monster_type);
    for (int i = 0; i < MAX_MTIMED; i++)
        C_MAKE(floor_ptr->mproc_list[i], w_ptr->max_m_idx, int16_t);

    C_MAKE(max_dlv, w_ptr->max_d_idx, DEPTH);
    for (int i = 0; i < MAX_HGT; i++)
        C_MAKE(floor_ptr->grid_array[i], MAX_WID, grid_type);

    C_MAKE(macro__pat, MACRO_MAX, concptr);
    C_MAKE(macro__act, MACRO_MAX, concptr);
    C_MAKE(macro__cmd, MACRO_MAX, bool);
    C_MAKE(macro__buf, FILE_READ_BUFF_SIZE, char);
    quark_init();

    for (int i = 0; option_info[i].o_desc; i++) {
        int os = option_info[i].o_set;
        int ob = option_info[i].o_bit;
        if (!option_info[i].o_var)
            continue;

        option_mask[os] |= (1UL << ob);
        if (option_info[i].o_norm)
            option_flag[os] |= (1UL << ob);
        else
            option_flag[os] &= ~(1UL << ob);
    }

    for (int n = 0; n < 8; n++)
        for (int i = 0; i < 32; i++)
            if (window_flag_desc[i])
                window_mask[n] |= (1UL << i);

    /*
     *  Set the "default" window flags
     *  Window 1 : Display messages
     *  Window 2 : Display inven/equip
     */
    window_flag[1] = 1UL << A_MAX;
    window_flag[2] = 1UL << 0;
    (void)format("%s (%s).", "Mr.Hoge", MAINTAINER);
    return 0;
}

/*!
 * @brief オブジェクト配列を初期化する /
 * Initialize some other arrays
 * @return エラーコード
 */
errr init_object_alloc(void)
{
    int16_t aux[MAX_DEPTH];
    (void)C_WIPE(aux, MAX_DEPTH, int16_t);

    int16_t num[MAX_DEPTH];
    (void)C_WIPE(num, MAX_DEPTH, int16_t);

    if (alloc_kind_table)
        C_KILL(alloc_kind_table, alloc_kind_size, alloc_entry);

    alloc_kind_size = 0;
    for (int i = 1; i < max_k_idx; i++) {
        object_kind *k_ptr;
        k_ptr = &k_info[i];
        for (int j = 0; j < 4; j++) {
            if (k_ptr->chance[j]) {
                alloc_kind_size++;
                num[k_ptr->locale[j]]++;
            }
        }
    }

    for (int i = 1; i < MAX_DEPTH; i++)
        num[i] += num[i - 1];

    if (!num[0])
        quit(_("町のアイテムがない！", "No town objects!"));

    C_MAKE(alloc_kind_table, alloc_kind_size, alloc_entry);
    alloc_entry *table;
    table = alloc_kind_table;
    for (int i = 1; i < max_k_idx; i++) {
        object_kind *k_ptr;
        k_ptr = &k_info[i];
        for (int j = 0; j < 4; j++) {
            if (k_ptr->chance[j] == 0)
                continue;

            int x = k_ptr->locale[j];
            int p = (100 / k_ptr->chance[j]);
            int y = (x > 0) ? num[x - 1] : 0;
            int z = y + aux[x];
            table[z].index = (KIND_OBJECT_IDX)i;
            table[z].level = (DEPTH)x;
            table[z].prob1 = (PROB)p;
            table[z].prob2 = (PROB)p;
            aux[x]++;
        }
    }

    return 0;
}

/*!
 * @brief モンスター配列と生成テーブルを初期化する /
 * Initialize some other arrays
 * @return エラーコード
 */
errr init_alloc(void)
{
    monster_race *r_ptr;
    tag_type *elements;
    C_MAKE(elements, max_r_idx, tag_type);
    for (int i = 1; i < max_r_idx; i++) {
        elements[i].tag = r_info[i].level;
        elements[i].index = i;
    }

    tag_sort(elements, max_r_idx);
    alloc_race_size = max_r_idx;
    C_MAKE(alloc_race_table, alloc_race_size, alloc_entry);
    for (int i = 1; i < max_r_idx; i++) {
        r_ptr = &r_info[elements[i].index];
        if (r_ptr->rarity == 0)
            continue;

        int x = r_ptr->level;
        int p = (100 / r_ptr->rarity);
        alloc_race_table[i].index = (KIND_OBJECT_IDX)elements[i].index;
        alloc_race_table[i].level = (DEPTH)x;
        alloc_race_table[i].prob1 = (PROB)p;
        alloc_race_table[i].prob2 = (PROB)p;
    }

    C_KILL(elements, max_r_idx, tag_type);
    (void)init_object_alloc();
    return 0;
}
