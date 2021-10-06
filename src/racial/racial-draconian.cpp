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
    case PlayerClassType::WARRIOR:
    case PlayerClassType::BERSERKER:
    case PlayerClassType::RANGER:
    case PlayerClassType::TOURIST:
    case PlayerClassType::IMITATOR:
    case PlayerClassType::ARCHER:
    case PlayerClassType::SMITH:
        if (one_in_(3)) {
            *breath_type = GF_MISSILE;
            *breath_type_description = _("エレメント", "the elements");
        } else {
            *breath_type = GF_SHARDS;
            *breath_type_description = _("破片", "shards");
        }

        break;
    case PlayerClassType::MAGE:
    case PlayerClassType::WARRIOR_MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
    case PlayerClassType::MAGIC_EATER:
    case PlayerClassType::RED_MAGE:
    case PlayerClassType::BLUE_MAGE:
    case PlayerClassType::MIRROR_MASTER:
        if (one_in_(3)) {
            *breath_type = GF_MANA;
            *breath_type_description = _("魔力", "mana");
        } else {
            *breath_type = GF_DISENCHANT;
            *breath_type_description = _("劣化", "disenchantment");
        }

        break;
    case PlayerClassType::CHAOS_WARRIOR:
        if (!one_in_(3)) {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        } else {
            *breath_type = GF_CHAOS;
            *breath_type_description = _("カオス", "chaos");
        }

        break;
    case PlayerClassType::MONK:
    case PlayerClassType::SAMURAI:
    case PlayerClassType::FORCETRAINER:
        if (!one_in_(3)) {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        } else {
            *breath_type = GF_SOUND;
            *breath_type_description = _("轟音", "sound");
        }

        break;
    case PlayerClassType::MINDCRAFTER:
        if (!one_in_(3)) {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        } else {
            *breath_type = GF_PSI;
            *breath_type_description = _("精神エネルギー", "mental energy");
        }

        break;
    case PlayerClassType::PRIEST:
    case PlayerClassType::PALADIN:
        if (one_in_(3)) {
            *breath_type = GF_HELL_FIRE;
            *breath_type_description = _("地獄の劫火", "hellfire");
        } else {
            *breath_type = GF_HOLY_FIRE;
            *breath_type_description = _("聖なる炎", "holy fire");
        }

        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::NINJA:
        if (one_in_(3)) {
            *breath_type = GF_DARK;
            *breath_type_description = _("暗黒", "darkness");
        } else {
            *breath_type = GF_POIS;
            *breath_type_description = _("毒", "poison");
        }

        break;
    case PlayerClassType::BARD:
        if (!one_in_(3)) {
            *breath_type = GF_SOUND;
            *breath_type_description = _("轟音", "sound");
        } else {
            *breath_type = GF_CONFUSION;
            *breath_type_description = _("混乱", "confusion");
        }

        break;
    case PlayerClassType::ELEMENTALIST:
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
