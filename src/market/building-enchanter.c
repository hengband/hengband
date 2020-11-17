#include "market/building-enchanter.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-slot-types.h"
#include "market/building-util.h"
#include "object/item-use-flags.h"
#include "racial/racial-android.h"
#include "spell/spells-object.h"
#include "system/object-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief アイテムの強化を行う。 / Enchant item
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param cost 1回毎の費用
 * @param to_hit 命中をアップさせる量
 * @param to_dam ダメージをアップさせる量
 * @param to_ac ＡＣをアップさせる量
 * @return 実際に行ったらTRUE
 */
bool enchant_item(player_type *player_ptr, PRICE cost, HIT_PROB to_hit, HIT_POINT to_dam, ARMOUR_CLASS to_ac, tval_type item_tester_tval)
{
    clear_bldg(4, 18);
    int maxenchant = (player_ptr->lev / 5);
    prt(format(_("現在のあなたの技量だと、+%d まで改良できます。", "  Based on your skill, we can improve up to +%d."), maxenchant), 5, 0);
    prt(format(_(" 改良の料金は一個につき＄%d です。", "  The price for the service is %d gold per item."), cost), 7, 0);

    concptr q = _("どのアイテムを改良しますか？", "Improve which item? ");
    concptr s = _("改良できるものがありません。", "You have nothing to improve.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_EQUIP | IGNORE_BOTHHAND_SLOT), item_tester_tval);
    if (!o_ptr)
        return FALSE;

    char tmp_str[MAX_NLEN];
    if (player_ptr->au < (cost * o_ptr->number)) {
        describe_flavor(player_ptr, tmp_str, o_ptr, OD_NAME_ONLY);
        msg_format(_("%sを改良するだけのゴールドがありません！", "You do not have the gold to improve %s!"), tmp_str);
        return FALSE;
    }

    bool okay = FALSE;
    for (int i = 0; i < to_hit; i++) {
        if ((o_ptr->to_h < maxenchant) && enchant(player_ptr, o_ptr, 1, (ENCH_TOHIT | ENCH_FORCE))) {
            okay = TRUE;
            break;
        }
    }

    for (int i = 0; i < to_dam; i++) {
        if ((o_ptr->to_d < maxenchant) && enchant(player_ptr, o_ptr, 1, (ENCH_TODAM | ENCH_FORCE))) {
            okay = TRUE;
            break;
        }
    }

    for (int i = 0; i < to_ac; i++) {
        if ((o_ptr->to_a < maxenchant) && enchant(player_ptr, o_ptr, 1, (ENCH_TOAC | ENCH_FORCE))) {
            okay = TRUE;
            break;
        }
    }

    if (!okay) {
        if (flush_failure)
            flush();
        msg_print(_("改良に失敗した。", "The improvement failed."));
        return FALSE;
    }

    describe_flavor(player_ptr, tmp_str, o_ptr, OD_NAME_AND_ENCHANT);
#ifdef JP
    msg_format("＄%dで%sに改良しました。", cost * o_ptr->number, tmp_str);
#else
    msg_format("Improved into %s for %d gold.", tmp_str, cost * o_ptr->number);
#endif

    player_ptr->au -= (cost * o_ptr->number);
    if (item >= INVEN_RARM)
        calc_android_exp(player_ptr);
    return TRUE;
}
