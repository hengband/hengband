/*!
 * @brief フロア生成時にアイテムを配置する
 * @date 2020/06/01
 * @author Hourier
 * @todo ちょっとギリギリ。後で分割を検討する
 */

#include "floor/floor-object.h"
#include "artifact/fixed-art-generator.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/item-getter.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "object/object-stack.h"
#include "perception/object-perception.h"
#include "system/alloc-entries.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "wizard/wizard-messages.h"
#include "world/world-object.h"
#include "world/world.h"

#define MAX_GOLD 18 /* Number of "gold" entries */

/*!
 * @brief オブジェクト生成テーブルに生成制約を加える /
 * Apply a "object restriction function" to the "object allocation table"
 * @return 常に0を返す。
 * @details 生成の制約はグローバルのget_obj_index_hook関数ポインタで加える
 */
static errr get_obj_index_prep(void)
{
    for (auto &entry : alloc_kind_table) {
        if (!get_obj_index_hook || (*get_obj_index_hook)(entry.index)) {
            entry.prob2 = entry.prob1;
        } else {
            entry.prob2 = 0;
        }
    }

    return 0;
}

/*!
 * @brief デバッグ時にアイテム生成情報をメッセージに出力する / Cheat -- describe a created object for the user
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr デバッグ出力するオブジェクトの構造体参照ポインタ
 */
static void object_mention(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);

    o_ptr->ident |= (IDENT_FULL_KNOWN);
    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
    msg_format_wizard(player_ptr, CHEAT_OBJECT, _("%sを生成しました。", "%s was generated."), item_name.data());
}

static int get_base_floor(FloorType *floor_ptr, BIT_FLAGS mode, std::optional<int> rq_mon_level)
{
    if (any_bits(mode, AM_GREAT)) {
        if (rq_mon_level.has_value()) {
            return rq_mon_level.value() + 10 + randint1(10);
        } else {
            return floor_ptr->object_level + 15;
        }
    }

    if (any_bits(mode, AM_GOOD)) {
        return floor_ptr->object_level + 10;
    }

    return floor_ptr->object_level;
}

static void set_ammo_quantity(ItemEntity *j_ptr)
{
    auto is_ammo = j_ptr->is_ammo();
    is_ammo |= j_ptr->bi_key.tval() == ItemKindType::SPIKE;
    if (is_ammo && !j_ptr->is_fixed_artifact()) {
        j_ptr->number = damroll(6, 7);
    }
}

/*!
 * @brief 生成階に応じたベースアイテムの生成を行う。
 * Attempt to make an object (normal or good/great)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param j_ptr 生成結果を収めたいオブジェクト構造体の参照ポインタ
 * @param mode オプションフラグ
 * @param rq_mon_level ランダムクエスト討伐対象のレベル。ランダムクエスト以外の生成であれば無効値
 * @return アイテムの生成成功可否
 */
bool make_object(PlayerType *player_ptr, ItemEntity *j_ptr, BIT_FLAGS mode, std::optional<int> rq_mon_level)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto prob = any_bits(mode, AM_GOOD) ? 10 : 1000;
    auto base = get_base_floor(floor_ptr, mode, rq_mon_level);
    if (!one_in_(prob) || !make_artifact_special(player_ptr, j_ptr)) {
        if (any_bits(mode, AM_GOOD) && !get_obj_index_hook) {
            get_obj_index_hook = kind_is_good;
        }

        if (get_obj_index_hook) {
            get_obj_index_prep();
        }

        auto bi_id = get_obj_index(floor_ptr, base, mode);
        if (get_obj_index_hook) {
            get_obj_index_hook = nullptr;
            get_obj_index_prep();
        }

        if (bi_id == 0) {
            return false;
        }

        j_ptr->prep(bi_id);
    }

    ItemMagicApplier(player_ptr, j_ptr, floor_ptr->object_level, mode).execute();
    set_ammo_quantity(j_ptr);
    if (cheat_peek) {
        object_mention(player_ptr, j_ptr);
    }

    return true;
}

/*!
 * @brief 生成階に応じた財宝オブジェクトの生成を行う。
 * Make a treasure object
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param j_ptr 生成結果を収めたいオブジェクト構造体の参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
bool make_gold(PlayerType *player_ptr, ItemEntity *j_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    int i = ((randint1(floor_ptr->object_level + 2) + 2) / 2) - 1;
    if (one_in_(CHANCE_BASEITEM_LEVEL_BOOST)) {
        i += randint1(floor_ptr->object_level + 1);
    }

    if (coin_type) {
        i = coin_type;
    }
    if (i >= MAX_GOLD) {
        i = MAX_GOLD - 1;
    }
    j_ptr->prep(OBJ_GOLD_LIST + i);

    int32_t base = baseitems_info[OBJ_GOLD_LIST + i].cost;
    j_ptr->pval = (base + (8L * randint1(base)) + randint1(8));

    return true;
}

/*!
 * @brief フロア中のアイテムを全て削除する / Deletes all objects at given location
 * Delete a dungeon object
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 削除したフロアマスのY座標
 * @param x 削除したフロアマスのX座標
 */
void delete_all_items_from_floor(PlayerType *player_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x)) {
        return;
    }

    g_ptr = &floor_ptr->grid_array[y][x];
    for (const auto this_o_idx : g_ptr->o_idx_list) {
        ItemEntity *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        o_ptr->wipe();
        floor_ptr->o_cnt--;
    }

    g_ptr->o_idx_list.clear();
    lite_spot(player_ptr, y, x);
}

/*!
 * @brief 床上のアイテムの数を増やす /
 * Increase the "number" of an item on the floor
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 増やしたいアイテムの所持スロット
 * @param num 増やしたいアイテムの数
 */
void floor_item_increase(PlayerType *player_ptr, INVENTORY_IDX i_idx, ITEM_NUMBER num)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;

    auto *o_ptr = &floor_ptr->o_list[i_idx];
    num += o_ptr->number;
    if (num > 255) {
        num = 255;
    } else if (num < 0) {
        num = 0;
    }

    num -= o_ptr->number;
    o_ptr->number += num;
    static constexpr auto flags = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
}

/*!
 * @brief 床上の数の無くなったアイテムスロットを消去する /
 * Optimize an item on the floor (destroy "empty" items)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 消去したいアイテムの所持スロット
 */
void floor_item_optimize(PlayerType *player_ptr, INVENTORY_IDX i_idx)
{
    auto *o_ptr = &player_ptr->current_floor_ptr->o_list[i_idx];
    if (!o_ptr->is_valid()) {
        return;
    }
    if (o_ptr->number) {
        return;
    }

    delete_object_idx(player_ptr, i_idx);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
}

/*!
 * @brief オブジェクトを削除する /
 * Delete a dungeon object
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_idx 削除対象のオブジェクト構造体ポインタ
 * @details
 * Handle "stacks" of objects correctly.
 */
void delete_object_idx(PlayerType *player_ptr, OBJECT_IDX o_idx)
{
    ItemEntity *j_ptr;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    excise_object_idx(floor_ptr, o_idx);
    j_ptr = &floor_ptr->o_list[o_idx];
    if (!j_ptr->is_held_by_monster()) {
        POSITION y, x;
        y = j_ptr->iy;
        x = j_ptr->ix;
        lite_spot(player_ptr, y, x);
    }

    j_ptr->wipe();
    floor_ptr->o_cnt--;
    static constexpr auto flags = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
}

/*!
 * @brief 床上、モンスター所持でスタックされたアイテムを削除しスタックを補完する / Excise a dungeon object from any stacks
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param o_idx 削除対象のオブジェクト構造体ポインタ
 */
void excise_object_idx(FloorType *floor_ptr, OBJECT_IDX o_idx)
{
    auto &list = get_o_idx_list_contains(floor_ptr, o_idx);
    list.remove(o_idx);
}

/*!
 * @brief 指定したOBJECT_IDXを含むリスト(モンスター所持リスト or 床上スタックリスト)への参照を得る
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param o_idx 参照を得るリストに含まれるOBJECT_IDX
 * @return o_idxを含む ObjectIndexList への参照
 */
ObjectIndexList &get_o_idx_list_contains(FloorType *floor_ptr, OBJECT_IDX o_idx)
{
    auto *o_ptr = &floor_ptr->o_list[o_idx];

    if (o_ptr->is_held_by_monster()) {
        return floor_ptr->m_list[o_ptr->held_m_idx].hold_o_idx_list;
    } else {
        return floor_ptr->grid_array[o_ptr->iy][o_ptr->ix].o_idx_list;
    }
}

/*!
 * @brief 生成済のオブジェクトをフロアの所定の位置に落とす。
 * Let an object fall to the ground at or near a location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param j_ptr 落としたいオブジェクト構造体の参照ポインタ
 * @param chance ドロップの消滅率(%)
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらオブジェクトのIDを返す。
 * @details
 * The initial location is assumed to be "in_bounds(floor_ptr, )".\n
 *\n
 * This function takes a parameter "chance".  This is the percentage\n
 * chance that the item will "disappear" instead of drop.  If the object\n
 * has been thrown, then this is the chance of disappearance on contact.\n
 *\n
 * Hack -- this function uses "chance" to determine if it should produce\n
 * some form of "description" of the drop event (under the player).\n
 *\n
 * We check several locations to see if we can find a location at which\n
 * the object can combine, stack, or be placed.  Artifacts will try very\n
 * hard to be placed, including "teleporting" to a useful grid if needed.\n
 */
OBJECT_IDX drop_near(PlayerType *player_ptr, ItemEntity *j_ptr, PERCENTAGE chance, POSITION y, POSITION x)
{
    int i, k, d, s;
    POSITION dy, dx;
    POSITION ty, tx = 0;
    OBJECT_IDX o_idx = 0;
    grid_type *g_ptr;
    bool flag = false;
    bool done = false;
#ifdef JP
#else
    bool plural = (j_ptr->number != 1);
#endif
    const auto item_name = describe_flavor(player_ptr, j_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (!j_ptr->is_fixed_or_random_artifact() && (randint0(100) < chance)) {
#ifdef JP
        msg_format("%sは消えた。", item_name.data());
#else
        msg_format("The %s disappear%s.", item_name.data(), (plural ? "" : "s"));
#endif
        if (w_ptr->wizard) {
            msg_print(_("(破損)", "(breakage)"));
        }

        return 0;
    }

    int bs = -1;
    int bn = 0;

    POSITION by = y;
    POSITION bx = x;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (dy = -3; dy <= 3; dy++) {
        for (dx = -3; dx <= 3; dx++) {
            bool comb = false;
            d = (dy * dy) + (dx * dx);
            if (d > 10) {
                continue;
            }

            ty = y + dy;
            tx = x + dx;
            if (!in_bounds(floor_ptr, ty, tx)) {
                continue;
            }
            if (!projectable(player_ptr, y, x, ty, tx)) {
                continue;
            }

            g_ptr = &floor_ptr->grid_array[ty][tx];
            if (!cave_drop_bold(floor_ptr, ty, tx)) {
                continue;
            }

            k = 0;
            for (const auto this_o_idx : g_ptr->o_idx_list) {
                ItemEntity *o_ptr;
                o_ptr = &floor_ptr->o_list[this_o_idx];
                if (object_similar(o_ptr, j_ptr)) {
                    comb = true;
                }

                k++;
            }

            if (!comb) {
                k++;
            }
            if (k > 99) {
                continue;
            }

            s = 1000 - (d + k * 5);
            if (s < bs) {
                continue;
            }

            if (s > bs) {
                bn = 0;
            }

            if ((++bn >= 2) && !one_in_(bn)) {
                continue;
            }

            bs = s;
            by = ty;
            bx = tx;

            flag = true;
        }
    }

    if (!flag && !j_ptr->is_fixed_or_random_artifact()) {
#ifdef JP
        msg_format("%sは消えた。", item_name.data());
#else
        msg_format("The %s disappear%s.", item_name.data(), (plural ? "" : "s"));
#endif
        if (w_ptr->wizard) {
            msg_print(_("(床スペースがない)", "(no floor space)"));
        }

        return 0;
    }

    for (i = 0; !flag && (i < 1000); i++) {
        ty = rand_spread(by, 1);
        tx = rand_spread(bx, 1);

        if (!in_bounds(floor_ptr, ty, tx)) {
            continue;
        }

        by = ty;
        bx = tx;

        if (!cave_drop_bold(floor_ptr, by, bx)) {
            continue;
        }

        flag = true;
    }

    auto &artifact = j_ptr->get_fixed_artifact();
    if (!flag) {
        int candidates = 0, pick;
        for (ty = 1; ty < floor_ptr->height - 1; ty++) {
            for (tx = 1; tx < floor_ptr->width - 1; tx++) {
                if (cave_drop_bold(floor_ptr, ty, tx)) {
                    candidates++;
                }
            }
        }

        if (!candidates) {
#ifdef JP
            msg_format("%sは消えた。", item_name.data());
#else
            msg_format("The %s disappear%s.", item_name.data(), (plural ? "" : "s"));
#endif

            if (w_ptr->wizard) {
                msg_print(_("(床スペースがない)", "(no floor space)"));
            }

            if (preserve_mode) {
                if (j_ptr->is_fixed_artifact() && !j_ptr->is_known()) {
                    artifact.is_generated = false;
                }
            }

            return 0;
        }

        pick = randint1(candidates);
        for (ty = 1; ty < floor_ptr->height - 1; ty++) {
            for (tx = 1; tx < floor_ptr->width - 1; tx++) {
                if (cave_drop_bold(floor_ptr, ty, tx)) {
                    pick--;
                    if (!pick) {
                        break;
                    }
                }
            }

            if (!pick) {
                break;
            }
        }

        by = ty;
        bx = tx;
    }

    g_ptr = &floor_ptr->grid_array[by][bx];
    for (const auto this_o_idx : g_ptr->o_idx_list) {
        ItemEntity *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (object_similar(o_ptr, j_ptr)) {
            object_absorb(o_ptr, j_ptr);
            done = true;
            break;
        }
    }

    if (!done) {
        o_idx = o_pop(floor_ptr);
    }

    if (!done && !o_idx) {
#ifdef JP
        msg_format("%sは消えた。", item_name.data());
#else
        msg_format("The %s disappear%s.", item_name.data(), (plural ? "" : "s"));
#endif
        if (w_ptr->wizard) {
            msg_print(_("(アイテムが多過ぎる)", "(too many objects)"));
        }

        if (j_ptr->is_fixed_artifact()) {
            artifact.is_generated = false;
        }

        return 0;
    }

    if (!done) {
        (&floor_ptr->o_list[o_idx])->copy_from(j_ptr);
        j_ptr = &floor_ptr->o_list[o_idx];
        j_ptr->iy = by;
        j_ptr->ix = bx;
        j_ptr->held_m_idx = 0;
        g_ptr->o_idx_list.add(floor_ptr, o_idx);
        done = true;
    }

    if (j_ptr->is_fixed_artifact() && w_ptr->character_dungeon) {
        artifact.floor_id = player_ptr->floor_id;
    }

    note_spot(player_ptr, by, bx);
    lite_spot(player_ptr, by, bx);
    sound(SOUND_DROP);

    if (player_bold(player_ptr, by, bx)) {
        static constexpr auto flags = {
            SubWindowRedrawingFlag::FLOOR_ITEMS,
            SubWindowRedrawingFlag::FOUND_ITEMS,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
    }

    if (chance && player_bold(player_ptr, by, bx)) {
        msg_print(_("何かが足下に転がってきた。", "You feel something roll beneath your feet."));
    }

    return o_idx;
}

/*!
 * @brief 床上の魔道具の残り残量メッセージを表示する
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param i_idx メッセージの対象にしたいアイテム所持スロット
 */
void floor_item_charges(FloorType *floor_ptr, INVENTORY_IDX i_idx)
{
    const auto &item = floor_ptr->o_list[i_idx];
    if (!item.is_wand_staff() || !item.is_known()) {
        return;
    }

#ifdef JP
    if (item.pval <= 0) {
        msg_print("この床上のアイテムは、もう魔力が残っていない。");
    } else {
        msg_format("この床上のアイテムは、あと %d 回分の魔力が残っている。", item.pval);
    }
#else
    if (item.pval != 1) {
        msg_format("There are %d charges remaining.", item.pval);
    } else {
        msg_format("There is %d charge remaining.", item.pval);
    }
#endif
}

/*!
 * @brief 床上のアイテムの残り数メッセージを表示する /
 * Describe the charges on an item on the floor.
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param i_idx メッセージの対象にしたいアイテム所持スロット
 */
void floor_item_describe(PlayerType *player_ptr, INVENTORY_IDX i_idx)
{
    auto *o_ptr = &player_ptr->current_floor_ptr->o_list[i_idx];
    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
#ifdef JP
    if (o_ptr->number <= 0) {
        msg_format("床上には、もう%sはない。", item_name.data());
    } else {
        msg_format("床上には、まだ %sがある。", item_name.data());
    }
#else
    msg_format("You see %s.", item_name.data());
#endif
}

/*
 * @brief Choose an item and get auto-picker entry from it.
 * @todo initial_i_idx をポインタではなく値に変え、戻り値をstd::pairに変える
 */
ItemEntity *choose_object(PlayerType *player_ptr, short *initial_i_idx, concptr q, concptr s, BIT_FLAGS option, const ItemTester &item_tester)
{
    if (initial_i_idx) {
        *initial_i_idx = INVEN_NONE;
    }

    FixItemTesterSetter setter(item_tester);
    short i_idx;
    if (!get_item(player_ptr, &i_idx, q, s, option, item_tester)) {
        return nullptr;
    }

    if (initial_i_idx) {
        *initial_i_idx = i_idx;
    }

    if (i_idx == INVEN_FORCE) {
        return nullptr;
    }

    return ref_item(player_ptr, i_idx);
}
