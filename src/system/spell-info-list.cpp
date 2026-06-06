#include "system/spell-info-list.h"
#include "realm/realm-types.h"
#include "system/angband-exceptions.h"
#include <algorithm>
#include <fmt/format.h>
#include <stdexcept>
#include <utility>

SpellInfoList SpellInfoList::instance{};

void SpellInfoList::initialize()
{
    this->spell_list.assign(enum2i(RealmType::MAX), std::vector<SpellInfo>(SPELLS_IN_REALM));
}

SpellInfoList &SpellInfoList::get_instance()
{
    return instance;
}

tl::optional<short> SpellInfoList::get_spell_id(RealmType realm, std::string_view spell_tag) const
{
    const auto &spells = this->spell_list[enum2i(realm)];
    const auto result = std::find_if(spells.begin(), spells.end(), [spell_tag](auto &spell_info) {
        return spell_info.tag == spell_tag;
    });

    if (result == spells.end()) {
        return tl::nullopt;
    }
    return (*result).idx;
}

const SpellInfo &SpellInfoList::get_spell_info(RealmType realm, int spell_id) const
{
    return this->spell_list[enum2i(realm)][spell_id];
}

void SpellInfoList::set_spell_info(RealmType realm, int spell_id, SpellInfo &&spell_info)
{
    const auto realm_index = enum2i(realm);
    if (realm_index < 0 || realm_index >= static_cast<int>(this->spell_list.size())) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid realm: {}", realm_index));
    }

    auto &spells = this->spell_list[realm_index];
    if (spell_id < 0 || spell_id >= static_cast<int>(spells.size())) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid spell ID: {}", spell_id));
    }

    spells[spell_id] = std::move(spell_info);
}
