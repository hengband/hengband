#include "mind/mind-mindcrafter.h"
#include "autopick/autopick.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/auto-destruction-options.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/item-use-flags.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "perception/simple-perception.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 超能力者のサイコメトリー処理/ Forcibly pseudo-identify an object in the inventory (or on the floor)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note
 * currently this function allows pseudo-id of any object,
 * including silly ones like potions & scrolls, which always
 * get '{average}'. This should be changed, either to stop such
 * items from being pseudo-id'd, or to allow psychometry to
 * detect whether the unidentified potion/scroll/etc is
 * good (Cure Light Wounds, Restore Strength, etc) or
 * bad (Poison, Weakness etc) or 'useless' (Slime Mold Juice, etc).
 */
bool psychometry(player_type *caster_ptr)
{
    concptr q = _("どのアイテムを調べますか？", "Meditate on which item? ");
    concptr s = _("調べるアイテムがありません。", "You have nothing appropriate.");
    object_type *o_ptr;
    OBJECT_IDX item;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return FALSE;

    if (object_is_known(o_ptr)) {
        msg_print(_("何も新しいことは判らなかった。", "You cannot find out anything more about that."));
        return TRUE;
    }

    item_feel_type feel = pseudo_value_check_heavy(o_ptr);
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    if (!feel) {
        msg_format(_("%sからは特に変わった事は感じとれなかった。", "You do not perceive anything unusual about the %s."), o_name);
        return TRUE;
    }

#ifdef JP
    msg_format("%sは%sという感じがする...", o_name, game_inscriptions[feel]);
#else
    msg_format("You feel that the %s %s %s...", o_name, ((o_ptr->number == 1) ? "is" : "are"), game_inscriptions[feel]);
#endif

    o_ptr->ident |= (IDENT_SENSE);
    o_ptr->feeling = feel;
    o_ptr->marked |= OM_TOUCHED;

    caster_ptr->update |= (PU_COMBINE | PU_REORDER);
    caster_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    bool okay = FALSE;
    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_CARD:
    case TV_RING:
    case TV_AMULET:
    case TV_LITE:
    case TV_FIGURINE:
        okay = TRUE;
        break;
    }

    autopick_alter_item(caster_ptr, item, (bool)(okay && destroy_feeling));
    return TRUE;
}
