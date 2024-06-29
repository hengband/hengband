#include "player/player-realm.h"
#include "object/tval-types.h"
#include "player-info/class-info.h"
#include "realm/realm-types.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

/*!
 * 職業毎に選択可能な第一領域魔法テーブル
 */
const std::vector<BIT_FLAGS> realm_choices1 = {
    (CH_NONE), /* Warrior */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Mage */
    (CH_LIFE | CH_DEATH | CH_DAEMON | CH_CRUSADE), /* Priest */
    (CH_SORCERY | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT), /* Rogue */
    (CH_NATURE), /* Ranger */
    (CH_CRUSADE | CH_DEATH), /* Paladin */
    (CH_ARCANE), /* Warrior-Mage */
    (CH_CHAOS | CH_DAEMON), /* Chaos-Warrior */
    (CH_LIFE | CH_NATURE | CH_DEATH | CH_ENCHANT), /* Monk */
    (CH_NONE), /* Mindcrafter */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE | CH_HEX), /* High-Mage */
    (CH_ARCANE), /* Tourist */
    (CH_NONE), /* Imitator */
    (CH_TRUMP), /* Beastmaster */
    (CH_NONE), /* Sorcerer */
    (CH_NONE), /* Archer */
    (CH_NONE), /* Magic eater */
    (CH_MUSIC), /* Bard */
    (CH_NONE), /* Red Mage */
    (CH_HISSATSU), /* Samurai */
    (CH_LIFE | CH_NATURE | CH_DEATH | CH_ENCHANT | CH_CRUSADE), /* ForceTrainer */
    (CH_NONE), /* Blue Mage */
    (CH_NONE), /* Cavalry */
    (CH_NONE), /* Berserker */
    (CH_NONE), /* Weaponsmith */
    (CH_NONE), /* Mirror-master */
    (CH_NONE), /* Ninja */
    (CH_NONE), /* Sniper */
    (CH_NONE), /* Elementalist */
};

/*!
 * 職業毎に選択可能な第二領域魔法テーブル
 */
const std::vector<BIT_FLAGS> realm_choices2 = {
    (CH_NONE), /* Warrior */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Mage */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Priest */
    (CH_NONE), /* Rogue */
    (CH_SORCERY | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_DAEMON), /* Ranger */
    (CH_NONE), /* Paladin */
    (CH_LIFE | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_SORCERY | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Warrior-Mage */
    (CH_NONE), /* Chaos-Warrior */
    (CH_NONE), /* Monk */
    (CH_NONE), /* Mindcrafter */
    (CH_NONE), /* High-Mage */
    (CH_NONE), /* Tourist */
    (CH_NONE), /* Imitator */
    (CH_NONE), /* Beastmanster */
    (CH_NONE), /* Sorcerer */
    (CH_NONE), /* Archer */
    (CH_NONE), /* Magic eater */
    (CH_NONE), /* Bard */
    (CH_NONE), /* Red Mage */
    (CH_NONE), /* Samurai */
    (CH_NONE), /* ForceTrainer */
    (CH_NONE), /* Blue Mage */
    (CH_NONE), /* Cavalry */
    (CH_NONE), /* Berserker */
    (CH_NONE), /* Weaponsmith */
    (CH_NONE), /* Mirror-master */
    (CH_NONE), /* Ninja */
    (CH_NONE), /* Sniper */
    (CH_NONE), /* Elementalist */
};

PlayerRealm::PlayerRealm(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

const magic_type &PlayerRealm::get_spell_info(int realm, int spell_idx)
{
    if (spell_idx < 0 || 32 <= spell_idx) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid spell idx: %d", spell_idx));
    }

    const auto realm_enum = i2enum<magic_realm_type>(realm);

    if (MAGIC_REALM_RANGE.contains(realm_enum)) {
        return mp_ptr->info[realm - 1][spell_idx];
    }
    if (TECHNIC_REALM_RANGE.contains(realm_enum)) {
        return technic_info[realm - MIN_TECHNIC][spell_idx];
    }

    THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", realm));
}

const magic_type &PlayerRealm::get_realm1_spell_info(int num) const
{
    return PlayerRealm::get_spell_info(this->player_ptr->realm1, num);
}

const magic_type &PlayerRealm::get_realm2_spell_info(int num) const
{
    return PlayerRealm::get_spell_info(this->player_ptr->realm2, num);
}

ItemKindType get_realm1_book(PlayerType *player_ptr)
{
    return ItemKindType::LIFE_BOOK + player_ptr->realm1 - 1;
}

ItemKindType get_realm2_book(PlayerType *player_ptr)
{
    return ItemKindType::LIFE_BOOK + player_ptr->realm2 - 1;
}
