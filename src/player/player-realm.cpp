#include "player/player-realm.h"
#include "object/tval-types.h"
#include "player-info/class-info.h"
#include "realm/realm-types.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

namespace {

const std::map<magic_realm_type, ItemKindType> realm_books = {
    { REALM_NONE, ItemKindType::NONE },
    { REALM_LIFE, ItemKindType::LIFE_BOOK },
    { REALM_SORCERY, ItemKindType::SORCERY_BOOK },
    { REALM_NATURE, ItemKindType::NATURE_BOOK },
    { REALM_CHAOS, ItemKindType::CHAOS_BOOK },
    { REALM_DEATH, ItemKindType::DEATH_BOOK },
    { REALM_TRUMP, ItemKindType::TRUMP_BOOK },
    { REALM_ARCANE, ItemKindType::ARCANE_BOOK },
    { REALM_CRAFT, ItemKindType::CRAFT_BOOK },
    { REALM_DAEMON, ItemKindType::DEMON_BOOK },
    { REALM_CRUSADE, ItemKindType::CRUSADE_BOOK },
    { REALM_MUSIC, ItemKindType::MUSIC_BOOK },
    { REALM_HISSATSU, ItemKindType::HISSATSU_BOOK },
    { REALM_HEX, ItemKindType::HEX_BOOK },
};

/*!
 * 職業毎に選択可能な第一領域魔法テーブル
 */
const std::map<PlayerClassType, RealmChoices> realm1_choices = {
    { PlayerClassType::MAGE, { REALM_LIFE, REALM_SORCERY, REALM_NATURE, REALM_CHAOS, REALM_DEATH, REALM_TRUMP, REALM_ARCANE, REALM_CRAFT, REALM_DAEMON, REALM_CRUSADE } },
    { PlayerClassType::PRIEST, { REALM_LIFE, REALM_DEATH, REALM_DAEMON, REALM_CRUSADE } },
    { PlayerClassType::ROGUE, { REALM_SORCERY, REALM_DEATH, REALM_TRUMP, REALM_ARCANE, REALM_CRAFT } },
    { PlayerClassType::RANGER, { REALM_NATURE } },
    { PlayerClassType::PALADIN, { REALM_CRUSADE, REALM_DEATH } },
    { PlayerClassType::WARRIOR_MAGE, { REALM_ARCANE } },
    { PlayerClassType::CHAOS_WARRIOR, { REALM_CHAOS, REALM_DAEMON } },
    { PlayerClassType::MONK, { REALM_LIFE, REALM_NATURE, REALM_DEATH, REALM_CRAFT } },
    { PlayerClassType::HIGH_MAGE, { REALM_LIFE, REALM_SORCERY, REALM_NATURE, REALM_CHAOS, REALM_DEATH, REALM_TRUMP, REALM_ARCANE, REALM_CRAFT, REALM_DAEMON, REALM_CRUSADE, REALM_HEX } },
    { PlayerClassType::TOURIST, { REALM_ARCANE } },
    { PlayerClassType::BEASTMASTER, { REALM_TRUMP } },
    { PlayerClassType::BARD, { REALM_MUSIC } },
    { PlayerClassType::SAMURAI, { REALM_HISSATSU } },
    { PlayerClassType::FORCETRAINER, { REALM_LIFE, REALM_NATURE, REALM_DEATH, REALM_CRAFT, REALM_CRUSADE } },
};

/*!
 * 職業毎に選択可能な第二領域魔法テーブル
 */
const std::map<PlayerClassType, RealmChoices> realm2_choices = {
    { PlayerClassType::MAGE, { REALM_LIFE, REALM_SORCERY, REALM_NATURE, REALM_CHAOS, REALM_DEATH, REALM_TRUMP, REALM_ARCANE, REALM_CRAFT, REALM_DAEMON, REALM_CRUSADE } },
    { PlayerClassType::PRIEST, { REALM_LIFE, REALM_SORCERY, REALM_NATURE, REALM_CHAOS, REALM_DEATH, REALM_TRUMP, REALM_ARCANE, REALM_CRAFT, REALM_DAEMON, REALM_CRUSADE } },
    { PlayerClassType::RANGER, { REALM_SORCERY, REALM_CHAOS, REALM_DEATH, REALM_TRUMP, REALM_ARCANE, REALM_DAEMON } },
    { PlayerClassType::WARRIOR_MAGE, { REALM_LIFE, REALM_NATURE, REALM_CHAOS, REALM_DEATH, REALM_TRUMP, REALM_ARCANE, REALM_SORCERY, REALM_CRAFT, REALM_DAEMON, REALM_CRUSADE } },
};
}

PlayerRealm::PlayerRealm(PlayerType *player_ptr)
    : realm1_(player_ptr->realm1)
    , realm2_(player_ptr->realm2)
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

ItemKindType PlayerRealm::get_book(int realm)
{
    const auto it = realm_books.find(i2enum<magic_realm_type>(realm));
    if (it == realm_books.end()) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", realm));
    }
    return it->second;
}

RealmChoices PlayerRealm::get_realm1_choices(PlayerClassType pclass)
{
    const auto it = realm1_choices.find(pclass);
    if (it == realm1_choices.end()) {
        return {};
    }
    return it->second;
}

RealmChoices PlayerRealm::get_realm2_choices(PlayerClassType pclass)
{
    const auto it = realm2_choices.find(pclass);
    if (it == realm2_choices.end()) {
        return {};
    }
    return it->second;
}

const PlayerRealm::Realm &PlayerRealm::realm1() const
{
    return this->realm1_;
}

const PlayerRealm::Realm &PlayerRealm::realm2() const
{
    return this->realm2_;
}

PlayerRealm::Realm::Realm(int realm)
    : realm(realm)
{
}

const magic_type &PlayerRealm::Realm::get_spell_info(int num) const
{
    return PlayerRealm::get_spell_info(this->realm, num);
}

ItemKindType PlayerRealm::Realm::get_book() const
{
    return PlayerRealm::get_book(this->realm);
}
