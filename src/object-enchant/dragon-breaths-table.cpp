#include "object-enchant/dragon-breaths-table.h"
#include "effect/attribute-types.h"
#include "locale/language-switcher.h"
#include "object-enchant/tr-types.h"
#include <sstream>

/*!
 * @brief 装備耐性に準じたブレス効果の選択テーブル /
 * Define flags, effect type, name for dragon breath activation
 */
namespace {
const std::vector<DragonBreathType> dragon_breaths_info = {
    { TR_RES_ACID, AttributeType::ACID, _("酸", "acid") },
    { TR_RES_ELEC, AttributeType::ELEC, _("電撃", "lightning") },
    { TR_RES_FIRE, AttributeType::FIRE, _("火炎", "fire") },
    { TR_RES_COLD, AttributeType::COLD, _("冷気", "cold") },
    { TR_RES_POIS, AttributeType::POIS, _("毒", "poison") },
    { TR_RES_LITE, AttributeType::LITE, _("閃光", "light") },
    { TR_RES_DARK, AttributeType::DARK, _("暗黒", "dark") },
    { TR_RES_SHARDS, AttributeType::SHARDS, _("破片", "shards") },
    { TR_RES_CONF, AttributeType::CONFUSION, _("混乱", "confusion") },
    { TR_RES_SOUND, AttributeType::SOUND, _("轟音", "sound") },
    { TR_RES_NEXUS, AttributeType::NEXUS, _("因果混乱", "nexus") },
    { TR_RES_NETHER, AttributeType::NETHER, _("地獄", "nether") },
    { TR_RES_CHAOS, AttributeType::CHAOS, _("カオス", "chaos") },
    { TR_RES_DISEN, AttributeType::DISENCHANT, _("劣化", "disenchantment") },
};
}

std::vector<std::pair<AttributeType, std::string>> DragonBreaths::get_breaths(const TrFlags &flags)
{
    std::vector<std::pair<AttributeType, std::string>> breaths;
    for (const auto &dragon_breath : dragon_breaths_info) {
        if (flags.has(dragon_breath.flag)) {
            breaths.push_back({ dragon_breath.type, dragon_breath.name });
        }
    }

    return breaths;
}

std::string DragonBreaths::build_description(const TrFlags &flags)
{
    std::stringstream ss;
    ss << _("", "breathe ");
    auto has_multi_breaths = false;
    for (const auto &dragon_breath : dragon_breaths_info) {
        if (flags.has_not(dragon_breath.flag)) {
            continue;
        }

        if (has_multi_breaths) {
            ss << _("、", ", ");
        }

        ss << dragon_breath.name;
        has_multi_breaths = true;
    }

    ss << _("のブレス(250)", " (250)");
    return ss.str();
}
