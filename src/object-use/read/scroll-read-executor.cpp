/*
 * @brief 読むことができるアイテム群の内、巻物を読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/scroll-read-executor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-enchant.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-object.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "store/rumor.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

ScrollReadExecutor::ScrollReadExecutor(PlayerType *player_ptr, ObjectType *o_ptr, bool known)
    : player_ptr(player_ptr)
    , o_ptr(o_ptr)
    , known(known)
{
}

bool ScrollReadExecutor::is_identified() const
{
    return this->ident;
}

bool ScrollReadExecutor::read()
{
    auto used_up = true;
    switch (this->o_ptr->sval) {
    case SV_SCROLL_DARKNESS:
        if (!has_resist_blind(this->player_ptr) && !has_resist_dark(this->player_ptr)) {
            (void)BadStatusSetter(this->player_ptr).mod_blindness(3 + randint1(5));
        }

        if (unlite_area(this->player_ptr, 10, 3)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_AGGRAVATE_MONSTER:
        msg_print(_("カン高くうなる様な音が辺りを覆った。", "There is a high pitched humming noise."));
        aggravate_monsters(this->player_ptr, 0);
        this->ident = true;
        break;
    case SV_SCROLL_CURSE_ARMOR:
        if (curse_armor(this->player_ptr)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_CURSE_WEAPON: {
        auto k = 0;
        if (has_melee_weapon(this->player_ptr, INVEN_MAIN_HAND)) {
            k = INVEN_MAIN_HAND;
            if (has_melee_weapon(this->player_ptr, INVEN_SUB_HAND) && one_in_(2)) {
                k = INVEN_SUB_HAND;
            }
        } else if (has_melee_weapon(this->player_ptr, INVEN_SUB_HAND)) {
            k = INVEN_SUB_HAND;
        }

        if (k && curse_weapon_object(this->player_ptr, false, &this->player_ptr->inventory_list[k])) {
            this->ident = true;
        }

        break;
    }
    case SV_SCROLL_SUMMON_MONSTER:
        for (auto k = 0; k < randint1(3); k++) {
            if (summon_specific(this->player_ptr, 0, this->player_ptr->y, this->player_ptr->x, this->player_ptr->current_floor_ptr->dun_level, SUMMON_NONE,
                    PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)) {
                this->ident = true;
            }
        }

        break;
    case SV_SCROLL_SUMMON_UNDEAD:
        for (auto k = 0; k < randint1(3); k++) {
            if (summon_specific(this->player_ptr, 0, this->player_ptr->y, this->player_ptr->x, this->player_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD,
                    PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)) {
                this->ident = true;
            }
        }

        break;
    case SV_SCROLL_SUMMON_PET:
        if (summon_specific(
                this->player_ptr, -1, this->player_ptr->y, this->player_ptr->x, this->player_ptr->current_floor_ptr->dun_level, SUMMON_NONE, PM_ALLOW_GROUP | PM_FORCE_PET)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_SUMMON_KIN:
        if (summon_kin_player(this->player_ptr, this->player_ptr->lev, this->player_ptr->y, this->player_ptr->x, PM_FORCE_PET | PM_ALLOW_GROUP)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_TRAP_CREATION:
        if (trap_creation(this->player_ptr, this->player_ptr->y, this->player_ptr->x)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_PHASE_DOOR:
        teleport_player(this->player_ptr, 10, TELEPORT_SPONTANEOUS);
        this->ident = true;
        break;
    case SV_SCROLL_TELEPORT:
        teleport_player(this->player_ptr, 100, TELEPORT_SPONTANEOUS);
        this->ident = true;
        break;
    case SV_SCROLL_TELEPORT_LEVEL: {
        (void)teleport_level(this->player_ptr, 0);
        this->ident = true;
        break;
    }
    case SV_SCROLL_WORD_OF_RECALL:
        if (!recall_player(this->player_ptr, randint0(21) + 15)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_IDENTIFY:
        if (!ident_spell(this->player_ptr, false)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_STAR_IDENTIFY:
        if (!identify_fully(this->player_ptr, false)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_REMOVE_CURSE:
        if (remove_curse(this->player_ptr)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_STAR_REMOVE_CURSE:
        if (remove_all_curse(this->player_ptr)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_ENCHANT_ARMOR:
        this->ident = true;
        if (!enchant_spell(this->player_ptr, 0, 0, 1)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
        if (!enchant_spell(this->player_ptr, 1, 0, 0)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
        if (!enchant_spell(this->player_ptr, 0, 1, 0)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_STAR_ENCHANT_ARMOR:
        if (!enchant_spell(this->player_ptr, 0, 0, randint1(3) + 2)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_STAR_ENCHANT_WEAPON:
        if (!enchant_spell(this->player_ptr, randint1(3), randint1(3), 0)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_RECHARGING:
        if (!recharge(this->player_ptr, 130)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_MUNDANITY:
        this->ident = true;
        if (!mundane_spell(this->player_ptr, false)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_LIGHT:
        if (lite_area(this->player_ptr, damroll(2, 8), 2)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_MAPPING:
        map_area(this->player_ptr, DETECT_RAD_MAP);
        this->ident = true;
        break;
    case SV_SCROLL_DETECT_GOLD:
        if (detect_treasure(this->player_ptr, DETECT_RAD_DEFAULT) || detect_objects_gold(this->player_ptr, DETECT_RAD_DEFAULT)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_DETECT_ITEM:
        if (detect_objects_normal(this->player_ptr, DETECT_RAD_DEFAULT)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_DETECT_TRAP:
        if (detect_traps(this->player_ptr, DETECT_RAD_DEFAULT, this->known)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_DETECT_DOOR:
        if (detect_doors(this->player_ptr, DETECT_RAD_DEFAULT) || detect_stairs(this->player_ptr, DETECT_RAD_DEFAULT)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_DETECT_INVIS:
        if (detect_monsters_invis(this->player_ptr, DETECT_RAD_DEFAULT)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_SATISFY_HUNGER: {
        if (set_food(this->player_ptr, PY_FOOD_MAX - 1)) {
            this->ident = true;
        }

        break;
    }
    case SV_SCROLL_BLESSING:
        if (set_blessed(this->player_ptr, this->player_ptr->blessed + randint1(12) + 6, false)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_HOLY_CHANT:
        if (set_blessed(this->player_ptr, this->player_ptr->blessed + randint1(24) + 12, false)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_HOLY_PRAYER:
        if (set_blessed(this->player_ptr, this->player_ptr->blessed + randint1(48) + 24, false)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_MONSTER_CONFUSION:
        if (any_bits(this->player_ptr->special_attack, ATTACK_CONFUSE)) {
            break;
        }

        msg_print(_("手が輝き始めた。", "Your hands begin to glow."));
        this->player_ptr->special_attack |= ATTACK_CONFUSE;
        this->player_ptr->redraw |= PR_STATUS;
        this->ident = true;
        break;
    case SV_SCROLL_PROTECTION_FROM_EVIL: {
        auto k = 3 * this->player_ptr->lev;
        if (set_protevil(this->player_ptr, this->player_ptr->protevil + randint1(25) + k, false)) {
            this->ident = true;
        }

        break;
    }
    case SV_SCROLL_RUNE_OF_PROTECTION:
        create_rune_protection_one(this->player_ptr);
        this->ident = true;
        break;
    case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
        if (destroy_doors_touch(this->player_ptr)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_STAR_DESTRUCTION:
        if (destroy_area(this->player_ptr, this->player_ptr->y, this->player_ptr->x, 13 + randint0(5), false)) {
            this->ident = true;
        } else {
            msg_print(_("ダンジョンが揺れた...", "The dungeon trembles..."));
        }

        break;
    case SV_SCROLL_DISPEL_UNDEAD:
        if (dispel_undead(this->player_ptr, 80)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_SPELL:
        if (!PlayerClass(this->player_ptr).has_number_of_spells_learned()) {
            break;
        }

        this->player_ptr->add_spells++;
        this->player_ptr->update |= PU_SPELLS;
        this->ident = true;
        break;
    case SV_SCROLL_GENOCIDE:
        (void)symbol_genocide(this->player_ptr, 300, true);
        this->ident = true;
        break;
    case SV_SCROLL_MASS_GENOCIDE:
        (void)mass_genocide(this->player_ptr, 300, true);
        this->ident = true;
        break;
    case SV_SCROLL_ACQUIREMENT:
        acquirement(this->player_ptr, this->player_ptr->y, this->player_ptr->x, 1, true, false, false);
        this->ident = true;
        break;
    case SV_SCROLL_STAR_ACQUIREMENT:
        acquirement(this->player_ptr, this->player_ptr->y, this->player_ptr->x, randint1(2) + 1, true, false, false);
        this->ident = true;
        break;
    case SV_SCROLL_FIRE:
        fire_ball(this->player_ptr, AttributeType::FIRE, 0, 666, 4);
        if (!(is_oppose_fire(this->player_ptr) || has_resist_fire(this->player_ptr) || has_immune_fire(this->player_ptr))) {
            take_hit(this->player_ptr, DAMAGE_NOESCAPE, 50 + randint1(50), _("炎の巻物", "a Scroll of Fire"));
        }

        this->ident = true;
        break;
    case SV_SCROLL_ICE:
        fire_ball(this->player_ptr, AttributeType::ICE, 0, 777, 4);
        if (!(is_oppose_cold(this->player_ptr) || has_resist_cold(this->player_ptr) || has_immune_cold(this->player_ptr))) {
            take_hit(this->player_ptr, DAMAGE_NOESCAPE, 100 + randint1(100), _("氷の巻物", "a Scroll of Ice"));
        }

        this->ident = true;
        break;
    case SV_SCROLL_CHAOS:
        fire_ball(this->player_ptr, AttributeType::CHAOS, 0, 1000, 4);
        if (!has_resist_chaos(this->player_ptr)) {
            take_hit(this->player_ptr, DAMAGE_NOESCAPE, 111 + randint1(111), _("ログルスの巻物", "a Scroll of Logrus"));
        }

        this->ident = true;
        break;
    case SV_SCROLL_RUMOR:
        msg_print(_("巻物にはメッセージが書かれている:", "There is message on the scroll. It says:"));
        msg_print(nullptr);
        display_rumor(this->player_ptr, true);
        msg_print(nullptr);
        msg_print(_("巻物は煙を立てて消え去った！", "The scroll disappears in a puff of smoke!"));
        this->ident = true;
        break;
    case SV_SCROLL_ARTIFACT:
        this->ident = true;
        if (!artifact_scroll(this->player_ptr)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_RESET_RECALL:
        this->ident = true;
        if (!reset_recall(this->player_ptr)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_AMUSEMENT:
        this->ident = true;
        generate_amusement(this->player_ptr, 1, false);
        break;
    case SV_SCROLL_STAR_AMUSEMENT:
        this->ident = true;
        generate_amusement(this->player_ptr, randint1(2) + 1, false);
        break;
    default:
        break;
    }

    return used_up;
}
