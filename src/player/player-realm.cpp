#include "player/player-realm.h"
#include "birth/birth-explanations-table.h"
#include "locale/localized-string.h"
#include "object/tval-types.h"
#include "player-info/class-info.h"
#include "realm/realm-types.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "system/spell-info-list.h"
#include "util/enum-converter.h"

namespace {

const std::map<RealmType, LocalizedString> realm_names = {
    { RealmType::NONE, { "魔法なし", "none" } },
    { RealmType::LIFE, { "生命", "Life" } },
    { RealmType::SORCERY, { "仙術", "Sorcery" } },
    { RealmType::NATURE, { "自然", "Nature" } },
    { RealmType::CHAOS, { "カオス", "Chaos" } },
    { RealmType::DEATH, { "暗黒", "Death" } },
    { RealmType::TRUMP, { "トランプ", "Trump" } },
    { RealmType::ARCANE, { "秘術", "Arcane" } },
    { RealmType::CRAFT, { "匠", "Craft" } },
    { RealmType::DAEMON, { "悪魔", "Daemon" } },
    { RealmType::CRUSADE, { "破邪", "Crusade" } },
    { RealmType::MUSIC, { "歌", "Music" } },
    { RealmType::HISSATSU, { "武芸", "Kendo" } },
    { RealmType::HEX, { "呪術", "Hex" } },
};

const std::map<RealmType, ItemKindType> realm_books = {
    { RealmType::NONE, ItemKindType::NONE },
    { RealmType::LIFE, ItemKindType::LIFE_BOOK },
    { RealmType::SORCERY, ItemKindType::SORCERY_BOOK },
    { RealmType::NATURE, ItemKindType::NATURE_BOOK },
    { RealmType::CHAOS, ItemKindType::CHAOS_BOOK },
    { RealmType::DEATH, ItemKindType::DEATH_BOOK },
    { RealmType::TRUMP, ItemKindType::TRUMP_BOOK },
    { RealmType::ARCANE, ItemKindType::ARCANE_BOOK },
    { RealmType::CRAFT, ItemKindType::CRAFT_BOOK },
    { RealmType::DAEMON, ItemKindType::DEMON_BOOK },
    { RealmType::CRUSADE, ItemKindType::CRUSADE_BOOK },
    { RealmType::MUSIC, ItemKindType::MUSIC_BOOK },
    { RealmType::HISSATSU, ItemKindType::HISSATSU_BOOK },
    { RealmType::HEX, ItemKindType::HEX_BOOK },
};

/*!
 * 職業毎に選択可能な第一領域魔法テーブル
 */
const std::map<PlayerClassType, RealmChoices> realm1_choices = {
    { PlayerClassType::MAGE, { RealmType::LIFE, RealmType::SORCERY, RealmType::NATURE, RealmType::CHAOS, RealmType::DEATH, RealmType::TRUMP, RealmType::ARCANE, RealmType::CRAFT, RealmType::DAEMON, RealmType::CRUSADE } },
    { PlayerClassType::PRIEST, { RealmType::LIFE, RealmType::DEATH, RealmType::DAEMON, RealmType::CRUSADE } },
    { PlayerClassType::ROGUE, { RealmType::SORCERY, RealmType::DEATH, RealmType::TRUMP, RealmType::ARCANE, RealmType::CRAFT } },
    { PlayerClassType::RANGER, { RealmType::NATURE } },
    { PlayerClassType::PALADIN, { RealmType::CRUSADE, RealmType::DEATH } },
    { PlayerClassType::WARRIOR_MAGE, { RealmType::ARCANE } },
    { PlayerClassType::CHAOS_WARRIOR, { RealmType::CHAOS, RealmType::DAEMON } },
    { PlayerClassType::MONK, { RealmType::LIFE, RealmType::NATURE, RealmType::DEATH, RealmType::CRAFT } },
    { PlayerClassType::HIGH_MAGE, { RealmType::LIFE, RealmType::SORCERY, RealmType::NATURE, RealmType::CHAOS, RealmType::DEATH, RealmType::TRUMP, RealmType::ARCANE, RealmType::CRAFT, RealmType::DAEMON, RealmType::CRUSADE, RealmType::HEX } },
    { PlayerClassType::TOURIST, { RealmType::ARCANE } },
    { PlayerClassType::BEASTMASTER, { RealmType::TRUMP } },
    { PlayerClassType::BARD, { RealmType::MUSIC } },
    { PlayerClassType::SAMURAI, { RealmType::HISSATSU } },
    { PlayerClassType::FORCETRAINER, { RealmType::LIFE, RealmType::NATURE, RealmType::DEATH, RealmType::CRAFT, RealmType::CRUSADE } },
};

/*!
 * 職業毎に選択可能な第二領域魔法テーブル
 */
const std::map<PlayerClassType, RealmChoices> realm2_choices = {
    { PlayerClassType::MAGE, { RealmType::LIFE, RealmType::SORCERY, RealmType::NATURE, RealmType::CHAOS, RealmType::DEATH, RealmType::TRUMP, RealmType::ARCANE, RealmType::CRAFT, RealmType::DAEMON, RealmType::CRUSADE } },
    { PlayerClassType::PRIEST, { RealmType::LIFE, RealmType::SORCERY, RealmType::NATURE, RealmType::CHAOS, RealmType::DEATH, RealmType::TRUMP, RealmType::ARCANE, RealmType::CRAFT, RealmType::DAEMON, RealmType::CRUSADE } },
    { PlayerClassType::RANGER, { RealmType::SORCERY, RealmType::CHAOS, RealmType::DEATH, RealmType::TRUMP, RealmType::ARCANE, RealmType::DAEMON } },
    { PlayerClassType::WARRIOR_MAGE, { RealmType::LIFE, RealmType::NATURE, RealmType::CHAOS, RealmType::DEATH, RealmType::TRUMP, RealmType::ARCANE, RealmType::SORCERY, RealmType::CRAFT, RealmType::DAEMON, RealmType::CRUSADE } },
};
}

PlayerRealm::PlayerRealm(PlayerType *player_ptr)
    : player_ptr(player_ptr)
    , realm1_(player_ptr->realm1)
    , realm2_(player_ptr->realm2)
{
}

const LocalizedString &PlayerRealm::get_name(RealmType realm)
{
    const auto it = realm_names.find(realm);
    if (it == realm_names.end()) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", enum2i(realm)));
    }
    return it->second;
}

std::string_view PlayerRealm::get_explanation(RealmType realm)
{
    if (is_magic(realm)) {
        return magic_explanations[enum2i(realm) - 1];
    }
    if (is_technic(realm)) {
        return technic_explanations[enum2i(realm) - MIN_TECHNIC];
    }

    THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", enum2i(realm)));
}

std::string_view PlayerRealm::get_subinfo(RealmType realm)
{
    if (is_magic(realm)) {
        return magic_subinfo[enum2i(realm) - 1];
    }
    if (is_technic(realm)) {
        return technic_subinfo[enum2i(realm) - MIN_TECHNIC];
    }

    THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", enum2i(realm)));
}

const magic_type &PlayerRealm::get_spell_info(RealmType realm, int spell_id, tl::optional<PlayerClassType> pclass)
{
    if (spell_id < 0 || 32 <= spell_id) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid spell id: %d", spell_id));
    }

    if (is_magic(realm)) {
        if (pclass) {
            return class_magics_info.at(enum2i(*pclass)).info[enum2i(realm)][spell_id];
        }
        return mp_ptr->info[enum2i(realm)][spell_id];
    }
    if (is_technic(realm)) {
        return technic_info[enum2i(realm) - MIN_TECHNIC][spell_id];
    }

    THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", enum2i(realm)));
}

const std::string &PlayerRealm::get_spell_name(RealmType realm, int spell_id)
{
    if (spell_id < 0 || 32 <= spell_id) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid spell id: %d", spell_id));
    }

    if (!is_magic(realm) && !is_technic(realm)) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", enum2i(realm)));
    }

    const auto &spell_info = SpellInfoList::get_instance().get_spell_info(realm, spell_id);
    return spell_info.name;
}

const std::string &PlayerRealm::get_spell_description(RealmType realm, int spell_id)
{
    if (spell_id < 0 || 32 <= spell_id) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid spell id: %d", spell_id));
    }

    if (!is_magic(realm) && !is_technic(realm)) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", enum2i(realm)));
    }

    const auto &spell_info = SpellInfoList::get_instance().get_spell_info(realm, spell_id);
    return spell_info.description;
}

ItemKindType PlayerRealm::get_book(RealmType realm)
{
    const auto it = realm_books.find(realm);
    if (it == realm_books.end()) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid realm: %d", enum2i(realm)));
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

RealmType PlayerRealm::get_realm_of_book(ItemKindType book)
{
    auto it = std::find_if(realm_books.begin(), realm_books.end(), [book](const auto &entry) { return entry.second == book; });
    return it == realm_books.end() ? RealmType::NONE : it->first;
}

bool PlayerRealm::is_magic(RealmType realm)
{
    return MAGIC_REALM_RANGE.contains(realm);
}

bool PlayerRealm::is_technic(RealmType realm)
{
    return TECHNIC_REALM_RANGE.contains(realm);
}

const PlayerRealm::Realm &PlayerRealm::realm1() const
{
    return this->realm1_;
}

const PlayerRealm::Realm &PlayerRealm::realm2() const
{
    return this->realm2_;
}

bool PlayerRealm::is_realm_hex() const
{
    return this->realm1_.equals(RealmType::HEX);
}

void PlayerRealm::reset()
{
    this->set_(RealmType::NONE, RealmType::NONE);
}

void PlayerRealm::set(RealmType realm1, RealmType realm2)
{
    if (!is_magic(realm1) && !is_technic(realm1)) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid realm1: %d", enum2i(realm1)));
    }
    if (realm2 != RealmType::NONE && !is_magic(realm2) && !is_technic(realm2)) {
        THROW_EXCEPTION(std::invalid_argument, format("Invalid realm2: %d", enum2i(realm2)));
    }

    this->set_(realm1, realm2);
}

void PlayerRealm::set_(RealmType realm1, RealmType realm2)
{
    this->player_ptr->realm1 = realm1;
    this->player_ptr->realm2 = realm2;
    this->realm1_ = Realm(realm1);
    this->realm2_ = Realm(realm2);
}

PlayerRealm::Realm::Realm(RealmType realm)
    : realm_(realm)
{
}

const LocalizedString &PlayerRealm::Realm::get_name() const
{
    return PlayerRealm::get_name(this->realm_);
}

std::string_view PlayerRealm::Realm::get_explanation() const
{
    return PlayerRealm::get_explanation(this->realm_);
}

std::string_view PlayerRealm::Realm::get_subinfo() const
{
    return PlayerRealm::get_subinfo(this->realm_);
}

const magic_type &PlayerRealm::Realm::get_spell_info(int spell_id) const
{
    return PlayerRealm::get_spell_info(this->realm_, spell_id);
}

const std::string &PlayerRealm::Realm::get_spell_name(int spell_id) const
{
    return PlayerRealm::get_spell_name(this->realm_, spell_id);
}

const std::string &PlayerRealm::Realm::get_spell_description(int spell_id) const
{
    return PlayerRealm::get_spell_description(this->realm_, spell_id);
}

ItemKindType PlayerRealm::Realm::get_book() const
{
    return PlayerRealm::get_book(this->realm_);
}

bool PlayerRealm::Realm::is_available() const
{
    return this->realm_ != RealmType::NONE;
}

bool PlayerRealm::Realm::is_good_attribute() const
{
    return this->realm_ == RealmType::LIFE || this->realm_ == RealmType::CRUSADE;
}

bool PlayerRealm::Realm::equals(RealmType realm) const
{
    return this->realm_ == realm;
}

RealmType PlayerRealm::Realm::to_enum() const
{
    return this->realm_;
}
