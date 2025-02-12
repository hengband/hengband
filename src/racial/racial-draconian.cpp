#include "racial/racial-draconian.h"
#include "effect/attribute-types.h"
#include "mind/mind-elementalist.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"
#include <optional>
#include <string>
#include <utility>

static std::optional<std::pair<AttributeType, std::string>> decide_breath_kind(PlayerType *player_ptr)
{
    if (randint1(100) >= player_ptr->lev) {
        return std::nullopt;
    }

    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::BERSERKER:
    case PlayerClassType::RANGER:
    case PlayerClassType::TOURIST:
    case PlayerClassType::IMITATOR:
    case PlayerClassType::ARCHER:
    case PlayerClassType::SMITH:
        if (one_in_(3)) {
            return std::pair(AttributeType::MISSILE, _("エレメント", "the elements"));
        }

        return std::pair(AttributeType::SHARDS, _("破片", "shards"));
    case PlayerClassType::MAGE:
    case PlayerClassType::WARRIOR_MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
    case PlayerClassType::MAGIC_EATER:
    case PlayerClassType::RED_MAGE:
    case PlayerClassType::BLUE_MAGE:
    case PlayerClassType::MIRROR_MASTER:
        if (one_in_(3)) {
            return std::pair(AttributeType::MANA, _("魔力", "mana"));
        }

        return std::pair(AttributeType::DISENCHANT, _("劣化", "disenchantment"));
    case PlayerClassType::CHAOS_WARRIOR:
        if (one_in_(3)) {
            return std::pair(AttributeType::CHAOS, _("カオス", "chaos"));
        }

        return std::pair(AttributeType::CONFUSION, _("混乱", "confusion"));
    case PlayerClassType::MONK:
    case PlayerClassType::SAMURAI:
    case PlayerClassType::FORCETRAINER:
        if (one_in_(3)) {
            return std::pair(AttributeType::SOUND, _("轟音", "sound"));
        }

        return std::pair(AttributeType::CONFUSION, _("混乱", "confusion"));
    case PlayerClassType::MINDCRAFTER:
        if (one_in_(3)) {
            return std::pair(AttributeType::PSI, _("精神エネルギー", "mental energy"));
        }

        return std::pair(AttributeType::CONFUSION, _("混乱", "confusion"));
    case PlayerClassType::PRIEST:
    case PlayerClassType::PALADIN:
        if (one_in_(3)) {
            return std::pair(AttributeType::HELL_FIRE, _("地獄の劫火", "hellfire"));
        }

        return std::pair(AttributeType::HOLY_FIRE, _("聖なる炎", "holy fire"));
    case PlayerClassType::ROGUE:
    case PlayerClassType::NINJA:
        if (one_in_(3)) {
            return std::pair(AttributeType::DARK, _("暗黒", "darkness"));
        }

        return std::pair(AttributeType::POIS, _("毒", "poison"));
    case PlayerClassType::BARD:
        if (one_in_(3)) {
            return std::pair(AttributeType::CONFUSION, _("混乱", "confusion"));
        }

        return std::pair(AttributeType::SOUND, _("轟音", "sound"));
    case PlayerClassType::ELEMENTALIST: {
        const auto type = get_element_type(player_ptr->element, 0);
        const std::string name(get_element_name(player_ptr->element, 0));
        return std::pair(type, name);
    }
    default:
        return std::nullopt;
    }
}

bool draconian_breath(PlayerType *player_ptr)
{
    auto breath_type = one_in_(3) ? AttributeType::COLD : AttributeType::FIRE;
    std::string breath_type_description((breath_type == AttributeType::COLD) ? _("冷気", "cold") : _("炎", "fire"));
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    const auto special_breath = decide_breath_kind(player_ptr);
    if (special_breath) {
        breath_type = special_breath->first;
        breath_type_description = special_breath->second;
    }

    stop_mouth(player_ptr);
    msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), breath_type_description.data());
    fire_breath(player_ptr, breath_type, dir, player_ptr->lev * 2, (player_ptr->lev / 15) + 1);
    return true;
}
