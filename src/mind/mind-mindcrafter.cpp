#include "mind/mind-mindcrafter.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
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
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 超能力者のサイコメトリー処理/ Forcibly pseudo-identify an object in the inventory (or on the floor)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @note
 * currently this function allows pseudo-id of any object,
 * including silly ones like potions & scrolls, which always
 * get '{average}'. This should be changed, either to stop such
 * items from being pseudo-id'd, or to allow psychometry to
 * detect whether the unidentified potion/scroll/etc is
 * good (Cure Light Wounds, Restore Strength, etc) or
 * bad (Poison, Weakness etc) or 'useless' (Slime Mold Juice, etc).
 */
bool psychometry(PlayerType *player_ptr)
{
    constexpr auto q = _("どのアイテムを調べますか？", "Meditate on which item? ");
    constexpr auto s = _("調べるアイテムがありません。", "You have nothing appropriate.");
    short i_idx;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return false;
    }

    if (o_ptr->is_known()) {
        msg_print(_("何も新しいことは判らなかった。", "You cannot find out anything more about that."));
        return true;
    }

    item_feel_type feel = pseudo_value_check_heavy(o_ptr);
    const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (!feel) {
        msg_format(_("%sからは特に変わった事は感じとれなかった。", "You do not perceive anything unusual about the %s."), item_name.data());
        return true;
    }

#ifdef JP
    msg_format("%sは%sという感じがする...", item_name.data(), game_inscriptions[feel]);
#else
    msg_format("You feel that the %s %s %s...", item_name.data(), ((o_ptr->number == 1) ? "is" : "are"), game_inscriptions[feel]);
#endif

    set_bits(o_ptr->ident, IDENT_SENSE);
    o_ptr->feeling = feel;
    o_ptr->marked.set(OmType::TOUCHED);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);

    bool okay = false;
    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::CARD:
    case ItemKindType::RING:
    case ItemKindType::AMULET:
    case ItemKindType::LITE:
    case ItemKindType::FIGURINE:
        okay = true;
        break;

    default:
        break;
    }

    autopick_alter_item(player_ptr, i_idx, (bool)(okay && destroy_feeling));
    return true;
}

/*!
 * @brief 超能力の発動 /
 * do_cmd_cast calls this function if the player's class is 'mindcrafter'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_mindcrafter_spell(PlayerType *player_ptr, MindMindcrafterType spell)
{
    bool b = false;
    int dam = 0;
    DIRECTION dir;
    TIME_EFFECT t;
    PLAYER_LEVEL plev = player_ptr->lev;
    switch (spell) {
    case MindMindcrafterType::PRECOGNITION:
        if (plev > 44) {
            chg_virtue(player_ptr, Virtue::KNOWLEDGE, 1);
            chg_virtue(player_ptr, Virtue::ENLIGHTEN, 1);
            wiz_lite(player_ptr, false);
        } else if (plev > 19) {
            map_area(player_ptr, DETECT_RAD_MAP);
        }

        if (plev < 30) {
            b = detect_monsters_normal(player_ptr, DETECT_RAD_DEFAULT);
            if (plev > 14) {
                b |= detect_monsters_invis(player_ptr, DETECT_RAD_DEFAULT);
            }
            if (plev > 4) {
                b |= detect_traps(player_ptr, DETECT_RAD_DEFAULT, true);
                b |= detect_doors(player_ptr, DETECT_RAD_DEFAULT);
            }
        } else {
            b = detect_all(player_ptr, DETECT_RAD_DEFAULT);
        }

        if ((plev > 24) && (plev < 40)) {
            set_tim_esp(player_ptr, (TIME_EFFECT)plev, false);
        }

        if (!b) {
            msg_print(_("安全な気がする。", "You feel safe."));
        }

        break;
    case MindMindcrafterType::NEURAL_BLAST:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        if (randint1(100) < plev * 2) {
            fire_beam(player_ptr, AttributeType::PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)));
        } else {
            fire_ball(player_ptr, AttributeType::PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)), 0);
        }
        break;
    case MindMindcrafterType::MINOR_DISPLACEMENT:
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case MindMindcrafterType::MAJOR_DISPLACEMENT:
        teleport_player(player_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case MindMindcrafterType::DOMINATION:
        if (plev < 30) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return false;
            }

            fire_ball(player_ptr, AttributeType::DOMINATION, dir, plev, 0);
        } else {
            charm_monsters(player_ptr, plev * 2);
        }

        break;
    case MindMindcrafterType::PLUVERISE:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_ball(player_ptr, AttributeType::TELEKINESIS, dir, damroll(8 + ((plev - 5) / 4), 8), (plev > 20 ? (plev - 20) / 8 + 1 : 0));
        break;
    case MindMindcrafterType::CHARACTER_ARMOR:
        set_shield(player_ptr, (TIME_EFFECT)plev, false);
        if (plev > 14) {
            set_oppose_acid(player_ptr, (TIME_EFFECT)plev, false);
        }
        if (plev > 19) {
            set_oppose_fire(player_ptr, (TIME_EFFECT)plev, false);
        }
        if (plev > 24) {
            set_oppose_cold(player_ptr, (TIME_EFFECT)plev, false);
        }
        if (plev > 29) {
            set_oppose_elec(player_ptr, (TIME_EFFECT)plev, false);
        }
        if (plev > 34) {
            set_oppose_pois(player_ptr, (TIME_EFFECT)plev, false);
        }

        break;
    case MindMindcrafterType::PSYCHOMETRY:
        if (plev < 25) {
            return psychometry(player_ptr);
        } else {
            return ident_spell(player_ptr, false);
        }
    case MindMindcrafterType::MIND_WAVE:
        msg_print(_("精神を捻じ曲げる波動を発生させた！", "Mind-warping forces emanate from your brain!"));
        if (plev < 25) {
            project(player_ptr, 0, 2 + plev / 10, player_ptr->y, player_ptr->x, (plev * 3), AttributeType::PSI, PROJECT_KILL);
        } else {
            (void)mindblast_monsters(player_ptr, randint1(plev * ((plev - 5) / 10 + 1)));
        }

        break;
    case MindMindcrafterType::ADRENALINE_CHANNELING: {
        BadStatusSetter bss(player_ptr);
        (void)bss.set_fear(0);
        (void)bss.set_stun(0);
        if (!is_fast(player_ptr) || !is_hero(player_ptr)) {
            hp_player(player_ptr, plev);
        }

        t = 10 + randint1((plev * 3) / 2);
        set_hero(player_ptr, t, false);
        (void)set_acceleration(player_ptr, t, false);
        break;
    }
    case MindMindcrafterType::TELEKINESIS:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fetch_item(player_ptr, dir, plev * 15, false);
        break;
    case MindMindcrafterType::PSYCHIC_DRAIN:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        dam = damroll(plev / 2, 6);
        if (fire_ball(player_ptr, AttributeType::PSI_DRAIN, dir, dam, 0)) {
            player_ptr->energy_need += randint1(150);
        }

        break;
    case MindMindcrafterType::PSYCHO_SPEAR:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_beam(player_ptr, AttributeType::PSY_SPEAR, dir, randint1(plev * 3) + plev * 3);
        break;
    case MindMindcrafterType::THE_WORLD:
        time_walk(player_ptr);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    return true;
}
