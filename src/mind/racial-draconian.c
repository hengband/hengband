#include "mind/racial-draconian.h"
#include "io/targeting.h"
#include "spell/spells-launcher.h"
#include "spell/spells-type.h"

bool draconian_breath(player_type *creature_ptr)
{
    int Type = (one_in_(3) ? GF_COLD : GF_FIRE);
    concptr Type_desc = ((Type == GF_COLD) ? _("冷気", "cold") : _("炎", "fire"));
    DIRECTION dir;
    if (!get_aim_dir(creature_ptr, &dir))
        return FALSE;

    if (randint1(100) < creature_ptr->lev) {
        switch (creature_ptr->pclass) {
        case CLASS_WARRIOR:
        case CLASS_BERSERKER:
        case CLASS_RANGER:
        case CLASS_TOURIST:
        case CLASS_IMITATOR:
        case CLASS_ARCHER:
        case CLASS_SMITH:
            if (one_in_(3)) {
                Type = GF_MISSILE;
                Type_desc = _("エレメント", "the elements");
            } else {
                Type = GF_SHARDS;
                Type_desc = _("破片", "shards");
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
                Type = GF_MANA;
                Type_desc = _("魔力", "mana");
            } else {
                Type = GF_DISENCHANT;
                Type_desc = _("劣化", "disenchantment");
            }

            break;
        case CLASS_CHAOS_WARRIOR:
            if (!one_in_(3)) {
                Type = GF_CONFUSION;
                Type_desc = _("混乱", "confusion");
            } else {
                Type = GF_CHAOS;
                Type_desc = _("カオス", "chaos");
            }

            break;
        case CLASS_MONK:
        case CLASS_SAMURAI:
        case CLASS_FORCETRAINER:
            if (!one_in_(3)) {
                Type = GF_CONFUSION;
                Type_desc = _("混乱", "confusion");
            } else {
                Type = GF_SOUND;
                Type_desc = _("轟音", "sound");
            }

            break;
        case CLASS_MINDCRAFTER:
            if (!one_in_(3)) {
                Type = GF_CONFUSION;
                Type_desc = _("混乱", "confusion");
            } else {
                Type = GF_PSI;
                Type_desc = _("精神エネルギー", "mental energy");
            }

            break;
        case CLASS_PRIEST:
        case CLASS_PALADIN:
            if (one_in_(3)) {
                Type = GF_HELL_FIRE;
                Type_desc = _("地獄の劫火", "hellfire");
            } else {
                Type = GF_HOLY_FIRE;
                Type_desc = _("聖なる炎", "holy fire");
            }

            break;
        case CLASS_ROGUE:
        case CLASS_NINJA:
            if (one_in_(3)) {
                Type = GF_DARK;
                Type_desc = _("暗黒", "darkness");
            } else {
                Type = GF_POIS;
                Type_desc = _("毒", "poison");
            }

            break;
        case CLASS_BARD:
            if (!one_in_(3)) {
                Type = GF_SOUND;
                Type_desc = _("轟音", "sound");
            } else {
                Type = GF_CONFUSION;
                Type_desc = _("混乱", "confusion");
            }

            break;
        }
    }

    stop_mouth(creature_ptr);
    msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), Type_desc);

    fire_breath(creature_ptr, Type, dir, creature_ptr->lev * 2, (creature_ptr->lev / 15) + 1);
    return TRUE;
}
