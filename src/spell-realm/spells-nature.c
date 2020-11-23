#include "spell-realm/spells-nature.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "racial/racial-android.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief –h‹ï‚ÌŽKŽ~‚ß–hŽ~ˆ—
 * @param caster_ptr ŽKŽ~‚ßŽÀsŽÒ‚ÌŽQÆƒ|ƒCƒ“ƒ^
 * @return ƒ^[ƒ“Á”ï‚ð—v‚·‚éˆ—‚ðs‚Á‚½‚È‚ç‚ÎTRUE‚ð•Ô‚·
 */
bool rustproof(player_type *caster_ptr)
{
    item_tester_hook = object_is_armour;
    concptr q = _("‚Ç‚Ì–h‹ï‚ÉŽKŽ~‚ß‚ð‚µ‚Ü‚·‚©H", "Rustproof which piece of armour? ");
    concptr s = _("ŽKŽ~‚ß‚Å‚«‚é‚à‚Ì‚ª‚ ‚è‚Ü‚¹‚ñB", "You have nothing to rustproof.");
    OBJECT_IDX item;
    object_type *o_ptr = choose_object(caster_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, 0);
    if (o_ptr == NULL)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
    if ((o_ptr->to_a < 0) && !object_is_cursed(o_ptr)) {
#ifdef JP
        msg_format("%s‚ÍV•i“¯—l‚É‚È‚Á‚½I", o_name);
#else
        msg_format("%s %s look%s as good as new!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
        o_ptr->to_a = 0;
    }

#ifdef JP
    msg_format("%s‚Í•…H‚µ‚È‚­‚È‚Á‚½B", o_name);
#else
    msg_format("%s %s %s now protected against corrosion.", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "are" : "is"));
#endif
    calc_android_exp(caster_ptr);
    return TRUE;
}
