#include "mind/mind-magic-eater.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "player-status/player-energy.h"
#include "sv-definition/sv-staff-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 魔道具術師の魔力取り込み処理
 * @param user_ptr アイテムを取り込むクリーチャー
 * @return 取り込みを実行したらTRUE、キャンセルしたらFALSEを返す
 */
bool import_magic_device(player_type *user_ptr)
{
    concptr q = _("どのアイテムの魔力を取り込みますか? ", "Gain power of which item? ");
    concptr s = _("魔力を取り込めるアイテムがない。", "There's nothing with power to absorb.");
    OBJECT_IDX item;
    object_type *o_ptr = choose_object(user_ptr, &item, q, s, USE_INVEN | USE_FLOOR, FuncItemTester(&object_type::is_rechargeable));
    if (!o_ptr)
        return false;

    if (o_ptr->tval == TV_STAFF && o_ptr->sval == SV_STAFF_NOTHING) {
        msg_print(_("この杖には発動の為の能力は何も備わっていないようだ。", "This staff doesn't have any magical ability."));
        return false;
    }

    if (!o_ptr->is_known()) {
        msg_print(_("鑑定されていないと取り込めない。", "You need to identify before absorbing."));
        return false;
    }

    if (o_ptr->timeout) {
        msg_print(_("充填中のアイテムは取り込めない。", "This item is still charging."));
        return false;
    }

    PARAMETER_VALUE pval = o_ptr->pval;
    int ext = 0;
    if (o_ptr->tval == TV_ROD)
        ext = 72;
    else if (o_ptr->tval == TV_WAND)
        ext = 36;

    if (o_ptr->tval == TV_ROD) {
        user_ptr->magic_num2[o_ptr->sval + ext] += (byte)o_ptr->number;
        if (user_ptr->magic_num2[o_ptr->sval + ext] > 99)
            user_ptr->magic_num2[o_ptr->sval + ext] = 99;
    } else {
        int num;
        for (num = o_ptr->number; num; num--) {
            int gain_num = pval;
            if (o_ptr->tval == TV_WAND)
                gain_num = (pval + num - 1) / num;
            if (user_ptr->magic_num2[o_ptr->sval + ext]) {
                gain_num *= 256;
                gain_num = (gain_num / 3 + randint0(gain_num / 3)) / 256;
                if (gain_num < 1)
                    gain_num = 1;
            }
            user_ptr->magic_num2[o_ptr->sval + ext] += (byte)gain_num;
            if (user_ptr->magic_num2[o_ptr->sval + ext] > 99)
                user_ptr->magic_num2[o_ptr->sval + ext] = 99;
            user_ptr->magic_num1[o_ptr->sval + ext] += pval * 0x10000;
            if (user_ptr->magic_num1[o_ptr->sval + ext] > 99 * 0x10000)
                user_ptr->magic_num1[o_ptr->sval + ext] = 99 * 0x10000;
            if (user_ptr->magic_num1[o_ptr->sval + ext] > user_ptr->magic_num2[o_ptr->sval + ext] * 0x10000)
                user_ptr->magic_num1[o_ptr->sval + ext] = user_ptr->magic_num2[o_ptr->sval + ext] * 0x10000;
            if (o_ptr->tval == TV_WAND)
                pval -= (pval + num - 1) / num;
        }
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(user_ptr, o_name, o_ptr, 0);
    msg_format(_("%sの魔力を取り込んだ。", "You absorb magic of %s."), o_name);

    vary_item(user_ptr, item, -999);
    PlayerEnergy(user_ptr).set_player_turn_energy(100);
    return true;
}
