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
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-perception.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "racial/racial-android.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief アーティファクト生成の巻物処理 /
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 生成が実際に試みられたらTRUEを返す
 */
bool artifact_scroll(player_type *caster_ptr)
{
    item_tester_hook = make_item_tester(object_is_nameless_weapon_armour);

    concptr q = _("どのアイテムを強化しますか? ", "Enchant which item? ");
    concptr s = _("強化できるアイテムがない。", "You have nothing to enchant.");
    object_type *o_ptr;
    OBJECT_IDX item;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), TV_NONE);
    if (!o_ptr)
        return false;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
    msg_format("%s は眩い光を発した！", o_name);
#else
    msg_format("%s %s radiate%s a blinding light!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

    bool okay = false;
    if (object_is_artifact(o_ptr)) {
#ifdef JP
        msg_format("%sは既に伝説のアイテムです！", o_name);
#else
        msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"), ((o_ptr->number > 1) ? "artifacts" : "an artifact"));
#endif
        okay = false;
    } else if (object_is_ego(o_ptr)) {
#ifdef JP
        msg_format("%sは既に名のあるアイテムです！", o_name);
#else
        msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"), ((o_ptr->number > 1) ? "ego items" : "an ego item"));
#endif
        okay = false;
    } else if (o_ptr->xtra3) {
#ifdef JP
        msg_format("%sは既に強化されています！", o_name);
#else
        msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"), ((o_ptr->number > 1) ? "customized items" : "a customized item"));
#endif
    } else {
        if (o_ptr->number > 1) {
            msg_print(_("複数のアイテムに魔法をかけるだけのエネルギーはありません！", "Not enough energy to enchant more than one object!"));
#ifdef JP
            msg_format("%d 個の%sが壊れた！", (o_ptr->number) - 1, o_name);
#else
            msg_format("%d of your %s %s destroyed!", (o_ptr->number) - 1, o_name, (o_ptr->number > 2 ? "were" : "was"));
#endif

            if (item >= 0) {
                inven_item_increase(caster_ptr, item, 1 - (o_ptr->number));
            } else {
                floor_item_increase(caster_ptr, 0 - item, 1 - (o_ptr->number));
            }
        }

        okay = become_random_artifact(caster_ptr, o_ptr, true);
    }

    if (!okay) {
        if (flush_failure)
            flush();

        msg_print(_("強化に失敗した。", "The enchantment failed."));
        if (one_in_(3))
            chg_virtue(caster_ptr, V_ENCHANT, -1);

        calc_android_exp(caster_ptr);
        return true;
    }

    if (record_rand_art) {
        describe_flavor(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
        exe_write_diary(caster_ptr, DIARY_ART_SCROLL, 0, o_name);
    }

    chg_virtue(caster_ptr, V_ENCHANT, 1);
    calc_android_exp(caster_ptr);
    return true;
}

/*!
 * @brief アイテム凡庸化のメインルーチン処理 /
 * Identify an object in the inventory (or on the floor)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に凡庸化をを行ったならばTRUEを返す
 * @details
 * <pre>
 * Mundanify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was mundanified, else FALSE.
 * </pre>
 */
bool mundane_spell(player_type *owner_ptr, bool only_equip)
{
    if (only_equip)
        item_tester_hook = make_item_tester(object_is_weapon_armour_ammo);

    OBJECT_IDX item;
    object_type *o_ptr;
    concptr q = _("どのアイテムを凡庸化しますか？", "Mundanify which item? ");
    concptr s = _("凡庸化できるアイテムがない。", "You have nothing to mundanify.");

    o_ptr = choose_object(owner_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), TV_NONE);
    if (!o_ptr)
        return false;

    msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
    POSITION iy = o_ptr->iy;
    POSITION ix = o_ptr->ix;
    byte marked = o_ptr->marked;
    uint16_t inscription = o_ptr->inscription;

    o_ptr->prep(o_ptr->k_idx);

    o_ptr->iy = iy;
    o_ptr->ix = ix;
    o_ptr->marked = marked;
    o_ptr->inscription = inscription;

    calc_android_exp(owner_ptr);
    return true;
}
