#include "spell-realm/spells-nature.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "racial/racial-android.h"
#include "system/item-entity.h"
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
    concptr q = _("どの防具に錆止めをしますか？", "Rustproof which piece of armour? ");
    concptr s = _("錆止めできるものがありません。", "You have nothing to rustproof.");
    OBJECT_IDX item;
    auto *o_ptr = choose_object(player_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, FuncItemTester(&ItemEntity::is_armour));
    if (o_ptr == nullptr) {
        return false;
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    o_ptr->art_flags.set(TR_IGNORE_ACID);
    if ((o_ptr->to_a < 0) && !o_ptr->is_cursed()) {
#ifdef JP
        msg_format("%sは新品同様になった！", o_name);
#else
        msg_format("%s %s look%s as good as new!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
        o_ptr->to_a = 0;
    }

#ifdef JP
    msg_format("%sは腐食しなくなった。", o_name);
#else
    msg_format("%s %s %s now protected against corrosion.", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "are" : "is"));
#endif
    calc_android_exp(player_ptr);
    return true;
}
