#include "inventory/inventory-describer.h"
#include "game-option/birth-options.h"
#include "inventory/inventory-slot-types.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief 所持/装備オブジェクトIDの部位表現を返す /
 * Return a string mentioning how a given item is carried
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i 部位表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 部位表現の文字列ポインタ
 */
concptr mention_use(PlayerType *player_ptr, int i)
{
    concptr p;

    /* Examine the location */
    switch (i) {
#ifdef JP
    case INVEN_MAIN_HAND:
        p = player_ptr->heavy_wield[0]
                ? "運搬中"
                : ((has_two_handed_weapons(player_ptr) && can_attack_with_main_hand(player_ptr)) ? " 両手" : (left_hander ? " 左手" : " 右手"));
        break;
#else
    case INVEN_MAIN_HAND:
        p = player_ptr->heavy_wield[0] ? "Just lifting" : (can_attack_with_main_hand(player_ptr) ? "Wielding" : "On arm");
        break;
#endif

#ifdef JP
    case INVEN_SUB_HAND:
        p = player_ptr->heavy_wield[1]
                ? "運搬中"
                : ((has_two_handed_weapons(player_ptr) && can_attack_with_sub_hand(player_ptr)) ? " 両手" : (left_hander ? " 右手" : " 左手"));
        break;
#else
    case INVEN_SUB_HAND:
        p = player_ptr->heavy_wield[1] ? "Just lifting" : (can_attack_with_sub_hand(player_ptr) ? "Wielding" : "On arm");
        break;
#endif

    case INVEN_BOW:
        p = (adj_str_hold[player_ptr->stat_index[A_STR]] < player_ptr->inventory_list[i].weight / 10) ? _("運搬中", "Just holding") : _("射撃用", "Shooting");
        break;
    case INVEN_MAIN_RING:
        p = (left_hander ? _("左手指", "On left hand") : _("右手指", "On right hand"));
        break;
    case INVEN_SUB_RING:
        p = (left_hander ? _("右手指", "On right hand") : _("左手指", "On left hand"));
        break;
    case INVEN_NECK:
        p = _("  首", "Around neck");
        break;
    case INVEN_LITE:
        p = _(" 光源", "Light source");
        break;
    case INVEN_BODY:
        p = _("  体", "On body");
        break;
    case INVEN_OUTER:
        p = _("体の上", "About body");
        break;
    case INVEN_HEAD:
        p = _("  頭", "On head");
        break;
    case INVEN_ARMS:
        p = _("  手", "On hands");
        break;
    case INVEN_FEET:
        p = _("  足", "On feet");
        break;
    default:
        p = _("ザック", "In pack");
        break;
    }

    return p;
}

/*!
 * @brief 所持/装備オブジェクトIDの現在の扱い方の状態表現を返す /
 * Return a string describing how a given item is being worn.
 * @param i 状態表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 状態表現内容の文字列ポインタ
 * @details
 * Currently, only used for items in the equipment, inventory.
 */
concptr describe_use(PlayerType *player_ptr, int i)
{
    concptr p;
    switch (i) {
#ifdef JP
    case INVEN_MAIN_HAND:
        p = player_ptr->heavy_wield[0]
                ? "運搬中の"
                : ((has_two_handed_weapons(player_ptr) && can_attack_with_main_hand(player_ptr)) ? "両手に装備している"
                                                                                                 : (left_hander ? "左手に装備している" : "右手に装備している"));
        break;
#else
    case INVEN_MAIN_HAND:
        p = player_ptr->heavy_wield[0] ? "just lifting" : (can_attack_with_main_hand(player_ptr) ? "attacking monsters with" : "wearing on your arm");
        break;
#endif

#ifdef JP
    case INVEN_SUB_HAND:
        p = player_ptr->heavy_wield[1]
                ? "運搬中の"
                : ((has_two_handed_weapons(player_ptr) && can_attack_with_sub_hand(player_ptr)) ? "両手に装備している"
                                                                                                : (left_hander ? "右手に装備している" : "左手に装備している"));
        break;
#else
    case INVEN_SUB_HAND:
        p = player_ptr->heavy_wield[1] ? "just lifting" : (can_attack_with_sub_hand(player_ptr) ? "attacking monsters with" : "wearing on your arm");
        break;
#endif

    case INVEN_BOW:
        p = (adj_str_hold[player_ptr->stat_index[A_STR]] < player_ptr->inventory_list[i].weight / 10) ? _("持つだけで精一杯の", "just holding")
                                                                                                      : _("射撃用に装備している", "shooting missiles with");
        break;
    case INVEN_MAIN_RING:
        p = (left_hander ? _("左手の指にはめている", "wearing on your left hand") : _("右手の指にはめている", "wearing on your right hand"));
        break;
    case INVEN_SUB_RING:
        p = (left_hander ? _("右手の指にはめている", "wearing on your right hand") : _("左手の指にはめている", "wearing on your left hand"));
        break;
    case INVEN_NECK:
        p = _("首にかけている", "wearing around your neck");
        break;
    case INVEN_LITE:
        p = _("光源にしている", "using to light the way");
        break;
    case INVEN_BODY:
        p = _("体に着ている", "wearing on your body");
        break;
    case INVEN_OUTER:
        p = _("身にまとっている", "wearing on your back");
        break;
    case INVEN_HEAD:
        p = _("頭にかぶっている", "wearing on your head");
        break;
    case INVEN_ARMS:
        p = _("手につけている", "wearing on your hands");
        break;
    case INVEN_FEET:
        p = _("足にはいている", "wearing on your feet");
        break;
    default:
        p = _("ザックに入っている", "carrying in your pack");
        break;
    }

    return p;
}
