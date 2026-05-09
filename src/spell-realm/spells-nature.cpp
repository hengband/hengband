#include "spell-realm/spells-nature.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "racial/racial-android.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 防具の錆止め防止処理
 * @param player_ptr 錆止め実行者の参照ポインタ
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool rustproof(PlayerType *player_ptr)
{
    constexpr auto q = _("どの防具に錆止めをしますか？", "Rustproof which piece of armour? ");
    constexpr auto s = _("錆止めできるものがありません。", "You have nothing to rustproof.");
    const auto options = USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT;
    const auto &[item, i_idx] = choose_item(player_ptr, q, s, options, FuncItemTester(&ItemEntity::is_protector));
    if (!item) {
        return false;
    }

    const auto item_name = describe_flavor(player_ptr, *item, OD_OMIT_PREFIX | OD_NAME_ONLY);
    item->art_flags.set(TR_IGNORE_ACID);
    if ((item->to_a < 0) && !item->is_cursed()) {
#ifdef JP
        msg_format("%sは新品同様になった！", item_name.data());
#else
        msg_format("%s %s look%s as good as new!", ((i_idx >= 0) ? "Your" : "The"), item_name.data(), ((item->number > 1) ? "" : "s"));
#endif
        item->to_a = 0;
    }

#ifdef JP
    msg_format("%sは腐食しなくなった。", item_name.data());
#else
    msg_format("%s %s %s now protected against corrosion.", ((i_idx >= 0) ? "Your" : "The"), item_name.data(), ((item->number > 1) ? "are" : "is"));
#endif
    calc_android_exp(player_ptr);
    return true;
}
