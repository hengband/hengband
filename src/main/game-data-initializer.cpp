/*!
 * @file game-data-initializer.cpp
 * @brief 変愚蛮怒のゲームデータ初期化定義
 */

#include "main/game-data-initializer.h"
#include "cmd-io/macro-util.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "floor/floor-list.h"
#include "floor/floor-util.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "system/baseitem/baseitem-allocation.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-allocation.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
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
    auto &floor_data = FloorList::get_instance();
    auto *floor_ptr = &floor_data.get_floor(0);
    player_ptr->current_floor_ptr = floor_ptr; // TODO:本当はこんなところで初期化したくない ← FloorTypeの方で初期化するべき？

    max_dlv.assign(dungeons_info.size(), {});
    init_gf_colors();

    macro_patterns.assign(MACRO_MAX, {});
    macro_actions.assign(MACRO_MAX, {});
    macro_buffers.assign(FILE_READ_BUFF_SIZE, {});
    for (auto &option : option_info) {
        int os = option.flag_position;
        int ob = option.offset;
        g_option_masks[os] |= (1UL << ob);
        if (option.default_value) {
            set_bits(g_option_flags[os], 1U << ob);
        } else {
            reset_bits(g_option_flags[os], 1U << ob);
        }
    }

    for (auto &window_mask : g_window_masks) {
        for (auto i = 0; i < 32; i++) {
            if (window_flag_desc[i]) {
                window_mask.set(i2enum<SubWindowRedrawingFlag>(i));
            }
        }
    }

    g_window_flags[1].clear();
    g_window_flags[1].set(SubWindowRedrawingFlag::MESSAGE);
    g_window_flags[2].clear();
    g_window_flags[2].set(SubWindowRedrawingFlag::INVENTORY);
}

/*!
 * @brief モンスター生成テーブルを初期化する
 */
void init_monsters_alloc()
{
    auto &table = MonraceAllocationTable::get_instance();
    table.initialize();
}

/*!
 * @brief アイテム生成テーブルを初期化する
 */
void init_items_alloc()
{
    auto &table = BaseitemAllocationTable::get_instance();
    table.initialize();
}
