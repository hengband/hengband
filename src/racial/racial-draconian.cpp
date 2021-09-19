#include "racial/racial-draconian.h"
#include "mind/mind-elementalist.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

static void decide_breath_kind(player_type *player_ptr, int *breath_type, concptr *breath_type_description)
{
    if (randint1(100) >= player_ptr->lev)
        return;

    switch (player_ptr->pclass) {
    case CLASS_WARRIOR:
    case CLASS_BERSERKER:
    case CLASS_RANGER:
    case CLASS_TOURIST:
    case CLASS_IMITATOR:
    case CLASS_ARCHER:
    case CLASS_SMITH:
        if (one_in_(3)) {
            *breath_type = GF_MISSILE;
            *breath_type_description = _("エレメント", "the elements");
        } else {
            *breath_type = GF_SHARDS;
            *breath_type_description = _("破片", "shards");
        }

        break;
    case CLASS_MAGE:
    case CLASS_WARRIOR_MAGE:
    case CLASS_HIGH_MAGE:
    case CLASS_SORCERER:
    case CLASS_MAGIC_EATER:
    case CLASS_RED_MAGE:
    case CLASS_BLUE_MAGE:
    case CLASS_MIRROR_MASTER:
        if (one_in_(3)) {
            *breath_type = GF_MANA;
            *breath_type_description = _("魔力", "mana");
        } else {
            *breath_type = GF_DISENCHANT;
            *breath_type_description = _("劣化", "disenchantment");
        }

        break;
    case CLASS_CHAOS_WARRIOR:
        if (!one_in_(3)) {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        } else {
            *breath_type = GF_CHAOS;
            *breath_type_description = _("カオス", "chaos");
        }

        break;
    case CLASS_MONK:
    case CLASS_SAMURAI:
    case CLASS_FORCETRAINER:
        if (!one_in_(3)) {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        } else {
            *breath_type = GF_SOUND;
            *breath_type_description = _("轟音", "sound");
        }

        break;
    case CLASS_MINDCRAFTER:
        if (!one_in_(3)) {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        } else {
            *breath_type = GF_PSI;
            *breath_type_description = _("精神エネルギー", "mental energy");
        }

        break;
    case CLASS_PRIEST:
    case CLASS_PALADIN:
        if (one_in_(3)) {
            *breath_type = GF_HELL_FIRE;
            *breath_type_description = _("地獄の劫火", "hellfire");
        } else {
            *breath_type = GF_HOLY_FIRE;
            *breath_type_description = _("聖なる炎", "holy fire");
        }

        break;
    case CLASS_ROGUE:
    case CLASS_NINJA:
        if (one_in_(3)) {
            *breath_type = GF_DARK;
            *breath_type_description = _("暗黒", "darkness");
        } else {
            *breath_type = GF_POIS;
            *breath_type_description = _("毒", "poison");
        }

        break;
    case CLASS_BARD:
        if (!one_in_(3)) {
            *breath_type = GF_SOUND;
            *breath_type_description = _("轟音", "sound");
        } else {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        }

        break;
    case CLASS_ELEMENTALIST:
        *breath_type = get_element_type(player_ptr->element, 0);
        *breath_type_description = get_element_name(player_ptr->element, 0);
        break;
    default:
        break;
    }
}

bool draconian_breath(player_type *player_ptr)
{
    int breath_type = (one_in_(3) ? GF_COLD : GF_FIRE);
    concptr breath_type_description = ((breath_type == GF_COLD) ? _("冷気", "cold") : _("炎", "fire"));
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir))
        return false;

    decide_breath_kind(player_ptr, &breath_type, &breath_type_description);
    stop_mouth(player_ptr);
    msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), breath_type_description);

    fire_breath(player_ptr, breath_type, dir, player_ptr->lev * 2, (player_ptr->lev / 15) + 1);
    return true;
}
