/*!
 * @file game-data-initializer.cpp
 * @brief 変愚蛮怒のゲームデータ初期化定義
 */

#include "main/game-data-initializer.h"
#include "cmd-io/macro-util.h"
#include "dungeon/quest.h"
#include "floor/floor-util.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "monster-race/monster-race.h"
#include "system/alloc-entries.h"
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>

/*!
 * @brief マクロ登録の最大数 / Maximum number of macros (see "io.c")
 * @note Default: assume at most 256 macros are used
 */
constexpr int MACRO_MAX = 256;

static void init_gf_colors()
{
    for (auto i = 0; i < enum2i(AttributeType::MAX); i++) {
        gf_colors.emplace(i2enum<AttributeType>(i), "");
    }
}

/*!
 * @brief その他の初期情報更新 /
 * Initialize some other arrays
 * @return エラーコード
 */
void init_other(PlayerType *player_ptr)
{
    player_ptr->current_floor_ptr = &floor_info; // TODO:本当はこんなところで初期化したくない
    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->o_list.assign(w_ptr->max_o_idx, {});
    floor_ptr->m_list.assign(w_ptr->max_m_idx, {});
    for (auto &list : floor_ptr->mproc_list) {
        list.assign(w_ptr->max_m_idx, {});
    }

    max_dlv.assign(dungeons_info.size(), {});
    floor_ptr->grid_array.assign(MAX_HGT, std::vector<grid_type>(MAX_WID));
    init_gf_colors();

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
static void init_object_alloc()
{
    short num[MAX_DEPTH]{};
    auto alloc_kind_size = 0;
    for (const auto &baseitem : baseitems_info) {
        for (const auto &[level, chance] : baseitem.alloc_tables) {
            if (chance != 0) {
                alloc_kind_size++;
                num[level]++;
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
    short aux[MAX_DEPTH]{};
    for (const auto &baseitem : baseitems_info) {
        for (const auto &[level, chance] : baseitem.alloc_tables) {
            if (chance == 0) {
                continue;
            }

            const auto x = level;
            const short p = 100 / chance;
            const auto y = (x > 0) ? num[x - 1] : 0;
            const auto z = y + aux[x];
            alloc_kind_table[z].index = baseitem.idx;
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
    std::vector<const MonsterRaceInfo *> elements;
    for (const auto &[r_idx, r_ref] : monraces_info) {
        if (MonsterRace(r_ref.idx).is_valid()) {
            elements.push_back(&r_ref);
        }
    }

    std::sort(elements.begin(), elements.end(),
        [](const MonsterRaceInfo *r1_ptr, const MonsterRaceInfo *r2_ptr) {
            return r1_ptr->level < r2_ptr->level;
        });

    alloc_race_table.clear();
    for (const auto r_ptr : elements) {
        if (r_ptr->rarity == 0) {
            continue;
        }

        const auto index = static_cast<short>(r_ptr->idx);
        const auto level = r_ptr->level;
        const auto prob = static_cast<PROB>(100 / r_ptr->rarity);
        alloc_race_table.push_back({ index, level, prob, prob });
    }

    init_object_alloc();
}
