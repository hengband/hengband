#include "info-reader/magic-info-tokens-table.h"
#include "object/tval-types.h"
#include "player-ability/player-ability-types.h"
#include "player-info/class-types.h"

/*!
 * @brief 魔法タイプ名とtvalの対応表
 */
const std::unordered_map<std::string_view, ItemKindType> magic_info_spelltype_to_tval = {
    { "SORCERY", ItemKindType::SORCERY_BOOK },
    { "LIFE", ItemKindType::LIFE_BOOK },
    { "MUSIC", ItemKindType::MUSIC_BOOK },
    { "HISSATSU", ItemKindType::HISSATSU_BOOK },
    { "NONE", ItemKindType::NONE },
};

/*!
 * @brief 魔法必須能力とenumの対応表
 */
const std::unordered_map<std::string_view, int> magic_info_name_to_stat = {
    { "STR", A_STR },
    { "INT", A_INT },
    { "WIS", A_WIS },
    { "DEX", A_DEX },
    { "CON", A_CON },
    { "CHR", A_CHR },
};

/*!
 * @brief 職業名とenumの対応表
 */
const std::unordered_map<std::string_view, PlayerClassType> magic_info_class_list = {
    { "WARRIOR", PlayerClassType::WARRIOR },
    { "MAGE", PlayerClassType::MAGE },
    { "PRIEST", PlayerClassType::PRIEST },
    { "ROGUE", PlayerClassType::ROGUE },
    { "RANGER", PlayerClassType::RANGER },
    { "PALADIN", PlayerClassType::PALADIN },
    { "WARRIOR_MAGE", PlayerClassType::WARRIOR_MAGE },
    { "CHAOS_WARRIOR", PlayerClassType::CHAOS_WARRIOR },
    { "MONK", PlayerClassType::MONK },
    { "MINDCRAFTER", PlayerClassType::MINDCRAFTER },
    { "HIGH_MAGE", PlayerClassType::HIGH_MAGE },
    { "TOURIST", PlayerClassType::TOURIST },
    { "IMITATOR", PlayerClassType::IMITATOR },
    { "BEASTMASTER", PlayerClassType::BEASTMASTER },
    { "SORCERER", PlayerClassType::SORCERER },
    { "ARCHER", PlayerClassType::ARCHER },
    { "MAGIC_EATER", PlayerClassType::MAGIC_EATER },
    { "BARD", PlayerClassType::BARD },
    { "RED_MAGE", PlayerClassType::RED_MAGE },
    { "SAMURAI", PlayerClassType::SAMURAI },
    { "FORCETRAINER", PlayerClassType::FORCETRAINER },
    { "BLUE_MAGE", PlayerClassType::BLUE_MAGE },
    { "CAVALRY", PlayerClassType::CAVALRY },
    { "BERSERKER", PlayerClassType::BERSERKER },
    { "SMITH", PlayerClassType::SMITH },
    { "MIRROR_MASTER", PlayerClassType::MIRROR_MASTER },
    { "NINJA", PlayerClassType::NINJA },
    { "SNIPER", PlayerClassType::SNIPER },
    { "ELEMENTALIST", PlayerClassType::ELEMENTALIST },
};
