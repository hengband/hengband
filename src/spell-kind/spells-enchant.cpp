#include "spell-kind/spells-enchant.h"
#include "artifact/random-art-generator.h"
#include "avatar/avatar.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-object.h"
#include "io/write-diary.h"
#include "object-hook/hook-perception.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "racial/racial-android.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include <memory>

/*!
 * @brief アーティファクト生成の巻物処理 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 生成が実際に試みられたらTRUEを返す
 */
bool artifact_scroll(PlayerType *player_ptr)
{
    constexpr auto q = _("どのアイテムを強化しますか? ", "Enchant which item? ");
    constexpr auto s = _("強化できるアイテムがない。", "You have nothing to enchant.");
    const auto &[item, i_idx] = choose_item(player_ptr, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), FuncItemTester(object_is_nameless_weapon_armour));
    if (!item) {
        return false;
    }

    const auto item_name = describe_flavor(player_ptr, *item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
    msg_format("%s は眩い光を発した！", item_name.data());
#else
    msg_format("%s %s radiate%s a blinding light!", ((i_idx >= 0) ? "Your" : "The"), item_name.data(), ((item->number > 1) ? "" : "s"));
#endif

    bool okay = false;
    if (item->is_fixed_or_random_artifact()) {
#ifdef JP
        msg_format("%sは既に伝説のアイテムです！", item_name.data());
#else
        msg_format("The %s %s already %s!", item_name.data(), ((item->number > 1) ? "are" : "is"), ((item->number > 1) ? "artifacts" : "an artifact"));
#endif
        okay = false;
    } else if (item->is_ego()) {
#ifdef JP
        msg_format("%sは既に名のあるアイテムです！", item_name.data());
#else
        msg_format("The %s %s already %s!", item_name.data(), ((item->number > 1) ? "are" : "is"), ((item->number > 1) ? "ego items" : "an ego item"));
#endif
        okay = false;
    } else if (item->is_smith()) {
#ifdef JP
        msg_format("%sは既に強化されています！", item_name.data());
#else
        msg_format("The %s %s already %s!", item_name.data(), ((item->number > 1) ? "are" : "is"), ((item->number > 1) ? "customized items" : "a customized item"));
#endif
    } else {
        if (item->number > 1) {
            msg_print(_("複数のアイテムに魔法をかけるだけのエネルギーはありません！", "Not enough energy to enchant more than one object!"));
#ifdef JP
            msg_format("%d 個の%sが壊れた！", (item->number) - 1, item_name.data());
#else
            msg_format("%d of your %s %s destroyed!", (item->number) - 1, item_name.data(), (item->number > 2 ? "were" : "was"));
#endif

            if (i_idx >= 0) {
                inven_item_increase(player_ptr, i_idx, 1 - (item->number));
            } else {
                floor_item_increase(player_ptr, 0 - i_idx, 1 - (item->number));
            }
        }

        okay = become_random_artifact(player_ptr, item.get(), true);
    }

    if (!okay) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("強化に失敗した。", "The enchantment failed."));
        if (one_in_(3)) {
            chg_virtue(player_ptr, Virtue::ENCHANT, -1);
        }

        calc_android_exp(player_ptr);
        return true;
    }

    if (record_rand_art) {
        const auto diary_item_name = describe_flavor(player_ptr, *item, OD_NAME_ONLY);
        exe_write_diary(*player_ptr->current_floor_ptr, DiaryKind::ART_SCROLL, 0, diary_item_name);
    }

    chg_virtue(player_ptr, Virtue::ENCHANT, 1);
    calc_android_exp(player_ptr);
    return true;
}

/*!
 * @brief アイテム凡庸化のメインルーチン処理 /
 * Identify an object in the inventory (or on the floor)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に凡庸化をを行ったならばTRUEを返す
 * @details
 * <pre>
 * Mundanify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was mundanified, else FALSE.
 * </pre>
 */
bool mundane_spell(PlayerType *player_ptr, bool only_equip)
{
    std::unique_ptr<ItemTester> item_tester =
        only_equip ? std::make_unique<FuncItemTester>(&ItemEntity::is_weapon_armour_ammo)
                   : std::make_unique<FuncItemTester>([](const ItemEntity *o_ptr) { return !o_ptr->bi_key.is_monster(); });

    constexpr auto q = _("どのアイテムを凡庸化しますか？", "Mundanify which item? ");
    constexpr auto s = _("凡庸化できるアイテムがない。", "You have nothing to mundanify.");
    const auto &[item, i_idx] = choose_item(player_ptr, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), *item_tester);
    if (!item) {
        return false;
    }

    msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
    auto pos = item->get_position();
    auto marked = item->marked;
    auto &&inscription = std::move(item->inscription);
    item->generate(item->bi_id);
    item->set_position(pos);
    item->marked = marked;
    item->inscription = std::move(inscription);
    calc_android_exp(player_ptr);
    return true;
}
