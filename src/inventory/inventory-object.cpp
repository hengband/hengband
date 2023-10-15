#include "inventory/inventory-object.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-weapon.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "player-info/equipment-info.h"
#include "spell-realm/spells-craft.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/object-sort.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

void vary_item(PlayerType *player_ptr, INVENTORY_IDX i_idx, ITEM_NUMBER num)
{
    if (i_idx >= 0) {
        inven_item_increase(player_ptr, i_idx, num);
        inven_item_describe(player_ptr, i_idx);
        inven_item_optimize(player_ptr, i_idx);
        return;
    }

    floor_item_increase(player_ptr, 0 - i_idx, num);
    floor_item_describe(player_ptr, 0 - i_idx);
    floor_item_optimize(player_ptr, 0 - i_idx);
}

/*!
 * @brief アイテムを増減させ残り所持数メッセージを表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 所持数を増やしたいプレイヤーのアイテム所持スロット
 * @param num 増やしたい量
 */
void inven_item_increase(PlayerType *player_ptr, INVENTORY_IDX i_idx, ITEM_NUMBER num)
{
    auto *o_ptr = &player_ptr->inventory_list[i_idx];
    num += o_ptr->number;
    if (num > 255) {
        num = 255;
    } else if (num < 0) {
        num = 0;
    }

    num -= o_ptr->number;
    if (num == 0) {
        return;
    }

    o_ptr->number += num;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::COMBINATION,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
    };
    rfu.set_flags(flags_swrf);
    if (o_ptr->number || !player_ptr->ele_attack) {
        return;
    }

    if (!(i_idx == INVEN_MAIN_HAND) && !(i_idx == INVEN_SUB_HAND)) {
        return;
    }

    if (has_melee_weapon(player_ptr, enum2i(INVEN_MAIN_HAND + INVEN_SUB_HAND) - i_idx)) {
        return;
    }

    set_ele_attack(player_ptr, 0, 0);
}

/*!
 * @brief 所持アイテムスロットから所持数のなくなったアイテムを消去する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 消去したいプレイヤーのアイテム所持スロット
 */
void inven_item_optimize(PlayerType *player_ptr, INVENTORY_IDX i_idx)
{
    auto *o_ptr = &player_ptr->inventory_list[i_idx];
    if (!o_ptr->is_valid()) {
        return;
    }
    if (o_ptr->number) {
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (i_idx >= INVEN_MAIN_HAND) {
        player_ptr->equip_cnt--;
        (&player_ptr->inventory_list[i_idx])->wipe();
        static constexpr auto flags_srf = {
            StatusRecalculatingFlag::BONUS,
            StatusRecalculatingFlag::TORCH,
            StatusRecalculatingFlag::MP,
        };
        rfu.set_flags(flags_srf);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::EQUIPMENT,
            SubWindowRedrawingFlag::SPELL,
        };
        rfu.set_flags(flags_swrf);
        return;
    }

    player_ptr->inven_cnt--;
    int i;
    for (i = i_idx; i < INVEN_PACK; i++) {
        player_ptr->inventory_list[i] = player_ptr->inventory_list[i + 1];
    }

    (&player_ptr->inventory_list[i])->wipe();
    static constexpr auto flags = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags);
}

/*!
 * @brief 所持スロットから床下にオブジェクトを落とすメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 所持テーブルのID
 * @param amt 落としたい個数
 */
void drop_from_inventory(PlayerType *player_ptr, INVENTORY_IDX i_idx, ITEM_NUMBER amt)
{
    ItemEntity forge;
    ItemEntity *q_ptr;
    auto *o_ptr = &player_ptr->inventory_list[i_idx];
    if (amt <= 0) {
        return;
    }

    if (amt > o_ptr->number) {
        amt = o_ptr->number;
    }

    if (i_idx >= INVEN_MAIN_HAND) {
        i_idx = inven_takeoff(player_ptr, i_idx, amt);
        o_ptr = &player_ptr->inventory_list[i_idx];
    }

    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    distribute_charges(o_ptr, q_ptr, amt);

    q_ptr->number = amt;
    const auto item_name = describe_flavor(player_ptr, q_ptr, 0);
    msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), item_name.data(), index_to_label(i_idx));
    (void)drop_near(player_ptr, q_ptr, 0, player_ptr->y, player_ptr->x);
    vary_item(player_ptr, i_idx, -amt);
}

/*!
 * @brief プレイヤーの所持スロットに存在するオブジェクトをまとめなおす /
 * Combine items in the pack
 * @details
 * Note special handling of the "overflow" slot
 */
void combine_pack(PlayerType *player_ptr)
{
    bool flag = false;
    bool is_first_combination = true;
    bool combined = true;
    while (is_first_combination || combined) {
        is_first_combination = false;
        combined = false;

        for (int i = INVEN_PACK; i > 0; i--) {
            ItemEntity *o_ptr;
            o_ptr = &player_ptr->inventory_list[i];
            if (!o_ptr->is_valid()) {
                continue;
            }
            for (int j = 0; j < i; j++) {
                ItemEntity *j_ptr;
                j_ptr = &player_ptr->inventory_list[j];
                if (!j_ptr->is_valid()) {
                    continue;
                }

                /*
                 * Get maximum number of the stack if these
                 * are similar, get zero otherwise.
                 */
                int max_num = object_similar_part(j_ptr, o_ptr);

                bool is_max = (max_num != 0) && (j_ptr->number < max_num);
                if (!is_max) {
                    continue;
                }

                if (o_ptr->number + j_ptr->number <= max_num) {
                    flag = true;
                    object_absorb(j_ptr, o_ptr);
                    player_ptr->inven_cnt--;
                    int k;
                    for (k = i; k < INVEN_PACK; k++) {
                        player_ptr->inventory_list[k] = player_ptr->inventory_list[k + 1];
                    }

                    (&player_ptr->inventory_list[k])->wipe();
                } else {
                    int old_num = o_ptr->number;
                    int remain = j_ptr->number + o_ptr->number - max_num;
                    object_absorb(j_ptr, o_ptr);
                    o_ptr->number = remain;
                    const auto tval = o_ptr->bi_key.tval();
                    if (tval == ItemKindType::ROD) {
                        o_ptr->pval = o_ptr->pval * remain / old_num;
                        o_ptr->timeout = o_ptr->timeout * remain / old_num;
                    }

                    if (tval == ItemKindType::WAND) {
                        o_ptr->pval = o_ptr->pval * remain / old_num;
                    }
                }

                RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::INVENTORY);
                combined = true;
                break;
            }
        }
    }

    if (flag) {
        msg_print(_("ザックの中のアイテムをまとめ直した。", "You combine some items in your pack."));
    }
}

/*!
 * @brief プレイヤーの所持スロットに存在するオブジェクトを並び替える /
 * Reorder items in the pack
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Note special handling of the "overflow" slot
 */
void reorder_pack(PlayerType *player_ptr)
{
    int i, j, k;
    int32_t o_value;
    ItemEntity forge;
    ItemEntity *q_ptr;
    ItemEntity *o_ptr;
    bool flag = false;

    for (i = 0; i < INVEN_PACK; i++) {
        if ((i == INVEN_PACK) && (player_ptr->inven_cnt == INVEN_PACK)) {
            break;
        }

        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        o_value = o_ptr->get_price();
        for (j = 0; j < INVEN_PACK; j++) {
            if (object_sort_comp(player_ptr, o_ptr, o_value, &player_ptr->inventory_list[j])) {
                break;
            }
        }

        if (j >= i) {
            continue;
        }

        flag = true;
        q_ptr = &forge;
        q_ptr->copy_from(&player_ptr->inventory_list[i]);
        for (k = i; k > j; k--) {
            (&player_ptr->inventory_list[k])->copy_from(&player_ptr->inventory_list[k - 1]);
        }

        (&player_ptr->inventory_list[j])->copy_from(q_ptr);
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::INVENTORY);
    }

    if (flag) {
        msg_print(_("ザックの中のアイテムを並べ直した。", "You reorder some items in your pack."));
    }
}

/*!
 * @brief オブジェクトをプレイヤーが拾って所持スロットに納めるメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 拾うオブジェクトの構造体参照ポインタ
 * @return 収められた所持スロットのID、拾うことができなかった場合-1を返す。
 */
int16_t store_item_to_inventory(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    INVENTORY_IDX i, j, k;
    INVENTORY_IDX n = -1;

    ItemEntity *j_ptr;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::PLAYER,
    };
    for (j = 0; j < INVEN_PACK; j++) {
        j_ptr = &player_ptr->inventory_list[j];
        if (!j_ptr->is_valid()) {
            continue;
        }

        n = j;
        if (object_similar(j_ptr, o_ptr)) {
            object_absorb(j_ptr, o_ptr);
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
            rfu.set_flags(flags_swrf);
            return j;
        }
    }

    if (player_ptr->inven_cnt > INVEN_PACK) {
        return -1;
    }

    for (j = 0; j <= INVEN_PACK; j++) {
        j_ptr = &player_ptr->inventory_list[j];
        if (!j_ptr->is_valid()) {
            break;
        }
    }

    i = j;
    if (i < INVEN_PACK) {
        const auto o_value = o_ptr->get_price();
        for (j = 0; j < INVEN_PACK; j++) {
            if (object_sort_comp(player_ptr, o_ptr, o_value, &player_ptr->inventory_list[j])) {
                break;
            }
        }

        i = j;
        for (k = n; k >= i; k--) {
            (&player_ptr->inventory_list[k + 1])->copy_from(&player_ptr->inventory_list[k]);
        }

        (&player_ptr->inventory_list[i])->wipe();
    }

    (&player_ptr->inventory_list[i])->copy_from(o_ptr);
    j_ptr = &player_ptr->inventory_list[i];
    j_ptr->held_m_idx = 0;
    j_ptr->iy = j_ptr->ix = 0;
    j_ptr->marked.clear().set(OmType::TOUCHED);

    player_ptr->inven_cnt++;
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flags(flags_swrf);
    return i;
}

/*!
 * @brief アイテムを拾う際にザックから溢れずに済むかを判定する /
 * Check if we have space for an item in the pack without overflow
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 拾いたいオブジェクトの構造体参照ポインタ
 * @return 溢れずに済むならTRUEを返す
 */
bool check_store_item_to_inventory(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (player_ptr->inven_cnt < INVEN_PACK) {
        return true;
    }

    for (int j = 0; j < INVEN_PACK; j++) {
        auto *j_ptr = &player_ptr->inventory_list[j];
        if (!j_ptr->is_valid()) {
            continue;
        }

        if (object_similar(j_ptr, o_ptr)) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief 装備スロットから装備を外すメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 装備を外したいインベントリのID
 * @param amt 外したい個数
 * @return 収められた所持スロットのID、拾うことができなかった場合-1を返す。
 */
INVENTORY_IDX inven_takeoff(PlayerType *player_ptr, INVENTORY_IDX i_idx, ITEM_NUMBER amt)
{
    INVENTORY_IDX slot;
    ItemEntity forge;
    ItemEntity *q_ptr;
    ItemEntity *o_ptr;
    concptr act;
    o_ptr = &player_ptr->inventory_list[i_idx];
    if (amt <= 0) {
        return -1;
    }

    if (amt > o_ptr->number) {
        amt = o_ptr->number;
    }
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = amt;
    const auto item_name = describe_flavor(player_ptr, q_ptr, 0);
    if (((i_idx == INVEN_MAIN_HAND) || (i_idx == INVEN_SUB_HAND)) && o_ptr->is_melee_weapon()) {
        act = _("を装備からはずした", "You were wielding");
    } else if (i_idx == INVEN_BOW) {
        act = _("を装備からはずした", "You were holding");
    } else if (i_idx == INVEN_LITE) {
        act = _("を光源からはずした", "You were holding");
    } else {
        act = _("を装備からはずした", "You were wearing");
    }

    inven_item_increase(player_ptr, i_idx, -amt);
    inven_item_optimize(player_ptr, i_idx);

    slot = store_item_to_inventory(player_ptr, q_ptr);
#ifdef JP
    msg_format("%s(%c)%s。", item_name.data(), index_to_label(slot), act);
#else
    msg_format("%s %s (%c).", act, item_name.data(), index_to_label(slot));
#endif

    return slot;
}
