/*!
 * @brief フロア生成時にアイテムを配置する
 * @date 2020/06/01
 * @author Hourier
 * @todo ちょっとギリギリ。後で分割を検討する
 */

#include "floor/floor-object.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
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
#include "perception/object-perception.h"
#include "range/v3/range/conversion.hpp"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-allocation.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"
#include <range/v3/algorithm.hpp>
#include <range/v3/functional.hpp>
#include <range/v3/view.hpp>

/*!
 * @brief デバッグ時にアイテム生成情報をメッセージに出力する / Cheat -- describe a created object for the user
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr デバッグ出力するオブジェクトの構造体参照ポインタ
 */
static void object_mention(PlayerType *player_ptr, ItemEntity &item)
{
    object_aware(player_ptr, item);
    item.mark_as_known();
    item.ident |= (IDENT_FULL_KNOWN);
    const auto item_name = describe_flavor(player_ptr, item, 0);
    msg_format_wizard(player_ptr, CHEAT_OBJECT, _("%sを生成しました。", "%s was generated."), item_name.data());
}

static int get_base_floor(const FloorType &floor, BIT_FLAGS mode, tl::optional<int> rq_mon_level)
{
    if (any_bits(mode, AM_GREAT)) {
        if (rq_mon_level) {
            return *rq_mon_level + 10 + randint1(10);
        }

        return floor.object_level + 15;
    }

    if (any_bits(mode, AM_GOOD)) {
        return floor.object_level + 10;
    }

    return floor.object_level;
}

static void set_ammo_quantity(ItemEntity *j_ptr)
{
    auto is_ammo = j_ptr->is_ammo();
    is_ammo |= j_ptr->bi_key.tval() == ItemKindType::SPIKE;
    if (is_ammo && !j_ptr->is_fixed_artifact()) {
        j_ptr->number = Dice::roll(6, 7);
    }
}

/*!
 * @brief 生成階に応じたベースアイテムの生成を行う。
 * Attempt to make an object (normal or good/great)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mode オプションフラグ
 * @param restrict ベースアイテム制約関数。see BaseitemAllocationTable::set_restriction()
 * @param rq_mon_level ランダムクエスト討伐対象のレベル。ランダムクエスト以外の生成であれば無効値
 * @return 生成したアイテム。ベースアイテム制約やアイテム生成レベルなどの要因で生成に失敗した場合はtl::nullopt。
 */
tl::optional<ItemEntity> make_object(PlayerType *player_ptr, BIT_FLAGS mode, BaseitemRestrict restrict, tl::optional<int> rq_mon_level)
{
    const auto apply_magic_to = [player_ptr, mode](ItemEntity &item) {
        ItemMagicApplier(player_ptr, &item, player_ptr->current_floor_ptr->object_level, mode).execute();
        set_ammo_quantity(&item);
        if (cheat_peek) {
            object_mention(player_ptr, item);
        }
    };
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto prob = any_bits(mode, AM_GOOD) ? 10 : 1000;
    const auto base = get_base_floor(floor, mode, rq_mon_level);
    if (!restrict && one_in_(prob)) {
        auto fa_opt = floor.try_make_instant_artifact();
        if (fa_opt) {
            apply_magic_to(*fa_opt);
            return fa_opt;
        }
    }

    if (any_bits(mode, AM_GOOD) && !restrict) {
        restrict = kind_is_good;
    }

    auto &table = BaseitemAllocationTable::get_instance();
    if (restrict) {
        // 制約が指定されている場合、ここで制約を加え、次のfloor.select_baseitem_id()で制約を使用する
        table.set_restriction(restrict);
    }

    const auto bi_id = floor.select_baseitem_id(base, mode);
    if (restrict) {
        // 他で影響がないように、制約を解除しておく
        table.reset_restriction();
    }

    if (bi_id == 0) {
        return tl::nullopt;
    }

    ItemEntity item(bi_id);
    apply_magic_to(item);
    return item;
}

/*!
 * @brief フロア中のアイテムを全て削除する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 削除したフロアマスの座標
 */
void delete_all_items_from_floor(PlayerType *player_ptr, const Pos2D &pos)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.contains(pos)) {
        return;
    }

    auto &grid = floor.get_grid(pos);
    delete_items(player_ptr, grid.o_idx_list);

    lite_spot(player_ptr, pos);
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
    auto &floor = *player_ptr->current_floor_ptr;

    auto *o_ptr = floor.o_list[i_idx].get();
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
    auto *o_ptr = player_ptr->current_floor_ptr->o_list[i_idx].get();
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
    auto &floor = *player_ptr->current_floor_ptr;
    excise_object_idx(floor, o_idx);
    auto &item_ptr = floor.o_list[o_idx];
    if (!item_ptr->is_held_by_monster()) {
        lite_spot(player_ptr, item_ptr->get_position());
    }

    // 最後尾のアイテムを削除対象の要素に移動することで配列を詰める
    const auto back_i_idx = static_cast<OBJECT_IDX>(floor.o_list.size() - 1);
    auto &list = get_o_idx_list_contains(floor, back_i_idx);
    ranges::replace(list, back_i_idx, o_idx);
    item_ptr = floor.o_list.back();
    floor.o_list.pop_back();

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
void excise_object_idx(FloorType &floor, OBJECT_IDX o_idx)
{
    auto &list = get_o_idx_list_contains(floor, o_idx);
    list.remove(o_idx);
}

/*!
 * @brief 複数のアイテムを削除する
 * @details 処理中に削除対象のインデックスが変わらないようにするため、削除対象のインデックスは降順にソートして処理される
 * @param delete_i_idx_list 削除するアイテムの参照IDのリスト
 */
void delete_items(PlayerType *player_ptr, std::vector<OBJECT_IDX> delete_i_idx_list)
{
    ranges::sort(delete_i_idx_list, ranges::greater{});

    for (const auto delete_i_idx : delete_i_idx_list) {
        delete_object_idx(player_ptr, delete_i_idx);
    }
}

/*!
 * @brief ObjectIndexListが管理しているアイテムをすべて削除する
 * @param o_idx_list 管理しているアイテムをすべて削除するObjectIndexListオブジェクト
 * @details 結果としてo_idx_listは空になるので、あえて引数は非const参照としている
 */
void delete_items(PlayerType *player_ptr, ObjectIndexList &o_idx_list)
{
    auto delete_i_idx_list = o_idx_list | ranges::to_vector;
    delete_items(player_ptr, std::move(delete_i_idx_list));
}

/*!
 * @brief 指定したOBJECT_IDXを含むリスト(モンスター所持リスト or 床上スタックリスト)への参照を得る
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param o_idx 参照を得るリストに含まれるOBJECT_IDX
 * @return o_idxを含む ObjectIndexList への参照
 */
ObjectIndexList &get_o_idx_list_contains(FloorType &floor, OBJECT_IDX o_idx)
{
    auto *o_ptr = floor.o_list[o_idx].get();

    if (o_ptr->is_held_by_monster()) {
        return floor.m_list[o_ptr->held_m_idx].hold_o_idx_list;
    } else {
        return floor.grid_array[o_ptr->iy][o_ptr->ix].o_idx_list;
    }
}

/*!
 * @brief アイテムを所定の位置に落とす。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param drop_item 落としたいアイテムへの参照
 * @param pos 配置したい座標
 * @param show_drop_message 足下に転がってきたアイテムのメッセージを表示するかどうか (デフォルトは表示する)
 */
short drop_near(PlayerType *player_ptr, ItemEntity &drop_item, const Pos2D &pos, bool show_drop_message)
{
#ifdef JP
#else
    const auto plural = (drop_item.number != 1);
#endif
    const auto &world = AngbandWorld::get_instance();
    const auto item_name = describe_flavor(player_ptr, drop_item, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    Pos2D pos_drop = pos; //!< @details 実際に落ちる座標.
    auto bs = -1;
    auto bn = 0;
    auto &floor = *player_ptr->current_floor_ptr;
    auto has_floor_space = false;
    for (auto dy = -3; dy <= 3; dy++) {
        for (auto dx = -3; dx <= 3; dx++) {
            const Pos2DVec vec(dy, dx);
            auto comb = false;
            const auto d = (dy * dy) + (dx * dx);
            if (d > 10) {
                continue;
            }

            const auto pos_target = pos + vec;
            if (!floor.contains(pos_target)) {
                continue;
            }
            if (!projectable(floor, pos, pos_target)) {
                continue;
            }

            if (!floor.can_drop_item_at(pos_target)) {
                continue;
            }

            const auto &grid = floor.get_grid(pos_target);
            auto k = 0;
            for (const auto this_o_idx : grid.o_idx_list) {
                const auto &floor_item = *floor.o_list[this_o_idx];
                if (floor_item.is_similar(drop_item)) {
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

            const auto s = 1000 - (d + k * 5);
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
            pos_drop = pos_target;
            has_floor_space = true;
        }
    }

    if (!has_floor_space && !drop_item.is_fixed_or_random_artifact()) {
#ifdef JP
        msg_format("%sは消えた。", item_name.data());
#else
        msg_format("The %s disappear%s.", item_name.data(), (plural ? "" : "s"));
#endif
        if (world.wizard) {
            msg_print(_("(床スペースがない)", "(no floor space)"));
        }

        return 0;
    }

    for (auto i = 0; !has_floor_space && (i < 1000); i++) {
        const auto ty = rand_spread(pos_drop.y, 1);
        const auto tx = rand_spread(pos_drop.x, 1);
        Pos2D pos_target(ty, tx); //!< @details 乱数引数の評価順を固定する.
        if (!floor.contains(pos_target)) {
            continue;
        }

        pos_drop = pos_target;
        if (!floor.can_drop_item_at(pos_drop)) {
            continue;
        }

        has_floor_space = true;
    }

    auto &artifact = drop_item.get_fixed_artifact();
    if (!has_floor_space) {
        const auto can_drop = [&](const Pos2D &pos) { return floor.can_drop_item_at(pos); };
        const auto pos_drop_candidates = floor.get_area(FloorBoundary::OUTER_WALL_EXCLUSIVE) | ranges::views::filter(can_drop) | ranges::to_vector;

        if (pos_drop_candidates.empty()) {
#ifdef JP
            msg_format("%sは消えた。", item_name.data());
#else
            msg_format("The %s disappear%s.", item_name.data(), (plural ? "" : "s"));
#endif

            if (world.wizard) {
                msg_print(_("(床スペースがない)", "(no floor space)"));
            }

            if (preserve_mode) {
                if (drop_item.is_fixed_artifact() && !drop_item.is_known()) {
                    artifact.is_generated = false;
                }
            }

            return 0;
        }

        pos_drop = rand_choice(pos_drop_candidates);
    }

    auto is_absorbed = false;
    auto &grid = floor.get_grid(pos_drop);
    for (const auto this_o_idx : grid.o_idx_list) {
        auto &floor_item = *floor.o_list[this_o_idx];
        if (floor_item.is_similar(drop_item)) {
            floor_item.absorb(drop_item);
            is_absorbed = true;
            break;
        }
    }

    short item_idx = is_absorbed ? 0 : floor.pop_empty_index_item();
    if (!is_absorbed && (item_idx == 0)) {
#ifdef JP
        msg_format("%sは消えた。", item_name.data());
#else
        msg_format("The %s disappear%s.", item_name.data(), (plural ? "" : "s"));
#endif
        if (world.wizard) {
            msg_print(_("(アイテムが多過ぎる)", "(too many objects)"));
        }

        if (drop_item.is_fixed_artifact()) {
            artifact.is_generated = false;
        }

        return 0;
    }

    if (!is_absorbed) {
        auto &floor_item = *floor.o_list[item_idx];
        floor_item = drop_item.clone();
        floor_item.set_position(pos_drop);
        floor_item.held_m_idx = 0;
        grid.o_idx_list.add(floor, item_idx);
    }

    if (drop_item.is_fixed_artifact() && world.character_dungeon) {
        artifact.floor_id = player_ptr->floor_id;
    }

    note_spot(player_ptr, pos_drop);
    lite_spot(player_ptr, pos_drop);
    sound(SoundKind::DROP);

    const auto is_located = player_ptr->is_located_at(pos_drop);
    if (is_located) {
        static constexpr auto flags = {
            SubWindowRedrawingFlag::FLOOR_ITEMS,
            SubWindowRedrawingFlag::FOUND_ITEMS,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
    }

    if (show_drop_message && is_located) {
        msg_print(_("何かが足下に転がってきた。", "You feel something roll beneath your feet."));
    }

    return item_idx;
}

/*!
 * @brief 矢弾アイテムを所定の位置に落とす。(指定した確率で壊れて消滅する)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param drop_item 落としたいアイテムへの参照
 * @param pos 配置したい座標
 * @param destruction_chance 消滅率(%)
 */
void drop_ammo_near(PlayerType *player_ptr, ItemEntity &drop_item, const Pos2D &pos, int destruction_chance)
{
    if (!drop_item.is_fixed_or_random_artifact() && evaluate_percent(destruction_chance)) {
        const auto item_name = describe_flavor(player_ptr, drop_item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
        msg_print("{}は消えた。", item_name);
#else
        const auto plural = (drop_item.number != 1);
        msg_print("The {} disappear{}.", item_name, (plural ? "" : "s"));
#endif
        if (AngbandWorld::get_instance().wizard) {
            msg_print(_("(破損)", "(breakage)"));
        }

        return;
    }

    (void)drop_near(player_ptr, drop_item, pos, true);
}

/*!
 * @brief 床上の魔道具の残り残量メッセージを表示する
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param i_idx メッセージの対象にしたいアイテム所持スロット
 */
void floor_item_charges(const FloorType &floor, INVENTORY_IDX i_idx)
{
    const auto &item = *floor.o_list[i_idx];
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
    const auto &item = *player_ptr->current_floor_ptr->o_list[i_idx];
    const auto item_name = describe_flavor(player_ptr, item, 0);
#ifdef JP
    if (item.number <= 0) {
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
