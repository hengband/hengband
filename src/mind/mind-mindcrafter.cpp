#include "mind/mind-mindcrafter.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/auto-destruction-options.h"
#include "hpmp/hp-mp-processor.h"
#include "mind/mind-mindcrafter.h"
#include "mind/mind-numbers.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/item-use-flags.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "perception/simple-perception.h"
#include "player/player-status.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 超能力者のサイコメトリー処理/ Forcibly pseudo-identify an object in the inventory (or on the floor)
 * @param caster_ptr プレーヤーへの参照ポインタ
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
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr)
        return false;

    if (object_is_known(o_ptr)) {
        msg_print(_("何も新しいことは判らなかった。", "You cannot find out anything more about that."));
        return true;
    }

    item_feel_type feel = pseudo_value_check_heavy(o_ptr);
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    if (!feel) {
        msg_format(_("%sからは特に変わった事は感じとれなかった。", "You do not perceive anything unusual about the %s."), o_name);
        return true;
    }

#ifdef JP
    msg_format("%sは%sという感じがする...", o_name, game_inscriptions[feel]);
#else
    msg_format("You feel that the %s %s %s...", o_name, ((o_ptr->number == 1) ? "is" : "are"), game_inscriptions[feel]);
#endif

    set_bits(o_ptr->ident, IDENT_SENSE);
    o_ptr->feeling = feel;
    set_bits(o_ptr->marked, OM_TOUCHED);

    set_bits(caster_ptr->update, PU_COMBINE | PU_REORDER);
    set_bits(caster_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);

    bool okay = false;
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
        okay = true;
        break;

    default:
        break;
    }

    autopick_alter_item(caster_ptr, item, (bool)(okay && destroy_feeling));
    return true;
}

/*!
 * @brief 超能力の発動 /
 * do_cmd_cast calls this function if the player's class is 'mindcrafter'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_mindcrafter_spell(player_type *caster_ptr, mind_mindcrafter_type spell)
{
    bool b = false;
    int dam = 0;
    DIRECTION dir;
    TIME_EFFECT t;
    PLAYER_LEVEL plev = caster_ptr->lev;
    switch (spell) {
    case PRECOGNITION:
        if (plev > 44) {
            chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
            chg_virtue(caster_ptr, V_ENLIGHTEN, 1);
            wiz_lite(caster_ptr, false);
        } else if (plev > 19)
            map_area(caster_ptr, DETECT_RAD_MAP);

        if (plev < 30) {
            b = detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
            if (plev > 14)
                b |= detect_monsters_invis(caster_ptr, DETECT_RAD_DEFAULT);
            if (plev > 4) {
                b |= detect_traps(caster_ptr, DETECT_RAD_DEFAULT, true);
                b |= detect_doors(caster_ptr, DETECT_RAD_DEFAULT);
            }
        } else {
            b = detect_all(caster_ptr, DETECT_RAD_DEFAULT);
        }

        if ((plev > 24) && (plev < 40))
            set_tim_esp(caster_ptr, (TIME_EFFECT)plev, false);

        if (!b)
            msg_print(_("安全な気がする。", "You feel safe."));

        break;
    case NEURAL_BLAST:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        if (randint1(100) < plev * 2)
            fire_beam(caster_ptr, GF_PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)));
        else
            fire_ball(caster_ptr, GF_PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)), 0);
        break;
    case MINOR_DISPLACEMENT:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case MAJOR_DISPLACEMENT:
        teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case DOMINATION:
        if (plev < 30) {
            if (!get_aim_dir(caster_ptr, &dir))
                return false;

            fire_ball(caster_ptr, GF_DOMINATION, dir, plev, 0);
        } else {
            charm_monsters(caster_ptr, plev * 2);
        }

        break;
    case PLUVERISE:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        fire_ball(caster_ptr, GF_TELEKINESIS, dir, damroll(8 + ((plev - 5) / 4), 8), (plev > 20 ? (plev - 20) / 8 + 1 : 0));
        break;
    case CHARACTER_ARMOR:
        set_shield(caster_ptr, (TIME_EFFECT)plev, false);
        if (plev > 14)
            set_oppose_acid(caster_ptr, (TIME_EFFECT)plev, false);
        if (plev > 19)
            set_oppose_fire(caster_ptr, (TIME_EFFECT)plev, false);
        if (plev > 24)
            set_oppose_cold(caster_ptr, (TIME_EFFECT)plev, false);
        if (plev > 29)
            set_oppose_elec(caster_ptr, (TIME_EFFECT)plev, false);
        if (plev > 34)
            set_oppose_pois(caster_ptr, (TIME_EFFECT)plev, false);

        break;
    case PSYCHOMETRY:
        if (plev < 25)
            return psychometry(caster_ptr);
        else
            return ident_spell(caster_ptr, false);
    case MIND_WAVE:
        msg_print(_("精神を捻じ曲げる波動を発生させた！", "Mind-warping forces emanate from your brain!"));
        if (plev < 25)
            project(caster_ptr, 0, 2 + plev / 10, caster_ptr->y, caster_ptr->x, (plev * 3), GF_PSI, PROJECT_KILL);
        else
            (void)mindblast_monsters(caster_ptr, randint1(plev * ((plev - 5) / 10 + 1)));

        break;
    case ADRENALINE_CHANNELING:
        set_afraid(caster_ptr, 0);
        set_stun(caster_ptr, 0);
        if (!is_fast(caster_ptr) || !is_hero(caster_ptr))
            hp_player(caster_ptr, plev);

        t = 10 + randint1((plev * 3) / 2);
        set_hero(caster_ptr, t, false);
        (void)set_fast(caster_ptr, t, false);
        break;
    case TELEKINESIS:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        fetch_item(caster_ptr, dir, plev * 15, false);
        break;
    case PSYCHIC_DRAIN:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        dam = damroll(plev / 2, 6);
        if (fire_ball(caster_ptr, GF_PSI_DRAIN, dir, dam, 0))
            caster_ptr->energy_need += randint1(150);

        break;
    case PSYCHO_SPEAR:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        fire_beam(caster_ptr, GF_PSY_SPEAR, dir, randint1(plev * 3) + plev * 3);
        break;
    case THE_WORLD:
        time_walk(caster_ptr);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    return true;
}
