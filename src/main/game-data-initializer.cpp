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
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/tag-sorter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief マクロ登録の最大数 / Maximum number of macros (see "io.c")
 * @note Default: assume at most 256 macros are used
 */
constexpr int MACRO_MAX = 256;

/*!
 * @brief クエスト情報初期化のメインルーチン /
 * Initialize quest array
 * @return エラーコード
 */
void init_quests(void)
{
    quest.assign(max_q_idx, {});
    for (auto &q_ref : quest) {
        q_ref.status = QuestStatusType::UNTAKEN;
    }
}

/*!
 * @brief その他の初期情報更新 /
 * Initialize some other arrays
 * @return エラーコード
 */
void init_other(player_type *player_ptr)
{
    player_ptr->current_floor_ptr = &floor_info; // TODO:本当はこんなところで初期化したくない
    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->o_list.assign(w_ptr->max_o_idx, {});
    floor_ptr->m_list.assign(w_ptr->max_m_idx, {});
    for (auto &list : floor_ptr->mproc_list) {
        list.assign(w_ptr->max_m_idx, {});
    }

    max_dlv.assign(d_info.size(), {});
    floor_ptr->grid_array.assign(MAX_HGT, std::vector<grid_type>(MAX_WID));

    macro__pat.assign(MACRO_MAX, {});
    macro__act.assign(MACRO_MAX, {});
    macro__buf.assign(FILE_READ_BUFF_SIZE, {});
    quark_init();

    for (auto i = 0; option_info[i].o_desc; i++) {
        int os = option_info[i].o_set;
        int ob = option_info[i].o_bit;
        if (option_info[i].o_var == nullptr) {
            continue;
        }

        option_mask[os] |= (1UL << ob);
        if (option_info[i].o_norm) {
            set_bits(option_flag[os], 1U << ob);
        } else {
            reset_bits(option_flag[os], 1U << ob);
        }
    }

    for (auto n = 0; n < 8; n++) {
        for (auto i = 0; i < 32; i++) {
            if (window_flag_desc[i]) {
                set_bits(window_mask[n], 1U << i);
            }
        }
    }

    /*
     *  Set the "default" window flags
     *  Window 1 : Display messages
     *  Window 2 : Display inven/equip
     */
    window_flag[1] = 1U << A_MAX;
    window_flag[2] = 1U << 0;
    (void)format("%s (%s).", "Mr.Hoge", MAINTAINER);
}

/*!
 * @brief オブジェクト配列を初期化する /
 * Initialize some other arrays
 * @return エラーコード
 */
static void init_object_alloc(void)
{
    int16_t num[MAX_DEPTH]{};
    auto alloc_kind_size = 0;
    for (const auto &k_ref : k_info) {
        for (auto j = 0; j < 4; j++) {
            if (k_ref.chance[j]) {
                alloc_kind_size++;
                num[k_ref.locale[j]]++;
            }
        }
    }

    for (auto i = 1; i < MAX_DEPTH; i++) {
        num[i] += num[i - 1];
    }

    if (num[0] == 0) {
        quit(_("町のアイテムがない！", "No town objects!"));
    }

    alloc_kind_table.assign(alloc_kind_size, {});
    int16_t aux[MAX_DEPTH]{};
    for (const auto &k_ref : k_info) {
        for (auto j = 0; j < 4; j++) {
            if (k_ref.chance[j] == 0) {
                continue;
            }

            auto x = k_ref.locale[j];
            PROB p = (100 / k_ref.chance[j]);
            auto y = (x > 0) ? num[x - 1] : 0;
            auto z = y + aux[x];
            alloc_kind_table[z].index = k_ref.idx;
            alloc_kind_table[z].level = x;
            alloc_kind_table[z].prob1 = p;
            alloc_kind_table[z].prob2 = p;
            aux[x]++;
        }
    }
}

/*!
 * @brief モンスター配列と生成テーブルを初期化する /
 * Initialize some other arrays
 * @return エラーコード
 */
void init_alloc(void)
{
    std::vector<tag_type> elements(r_info.size());
    for (const auto &r_ref : r_info) {
        if (r_ref.idx > 0) {
            elements[r_ref.idx].tag = r_ref.level;
            elements[r_ref.idx].index = r_ref.idx;
        }
    }

    tag_sort(elements.data(), elements.size());
    alloc_race_table.assign(r_info.size(), {});
    for (auto i = 1U; i < r_info.size(); i++) {
        auto *r_ptr = &r_info[elements[i].index];
        if (r_ptr->rarity == 0) {
            continue;
        }

        auto x = r_ptr->level;
        PROB p = (100 / r_ptr->rarity);
        alloc_race_table[i].index = (KIND_OBJECT_IDX)elements[i].index;
        alloc_race_table[i].level = x;
        alloc_race_table[i].prob1 = p;
        alloc_race_table[i].prob2 = p;
    }

    init_object_alloc();
}
