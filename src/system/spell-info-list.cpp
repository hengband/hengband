#include "system/spell-info-list.h"
#include "info-reader/spell-reader.h"
#include "realm/realm-types.h"
#include <algorithm>

SpellInfoList SpellInfoList::instance{};

void SpellInfoList::initialize()
{
    this->spell_list.assign(magic_realm_type::REALM_MAX, std::vector<SpellInfo>(SPELLS_IN_REALM));
}

SpellInfoList &SpellInfoList::get_instance()
{
    return instance;
}

std::optional<short> SpellInfoList::get_spell_id(int realm_id, std::string_view spell_tag) const
{
    const auto &realm = this->spell_list[realm_id];
    const auto result = std::find_if(realm.begin(), realm.end(), [spell_tag](auto &spell_info) {
        return spell_info.tag == spell_tag;
    });

    if (result == realm.end()) {
        return std::nullopt;
    }
    return (*result).idx;
}

const SpellInfo &SpellInfoList::get_spell_info(int realm_id, int spell_id) const
{
    return this->spell_list[realm_id][spell_id];
}

errr SpellInfoList::parse(nlohmann::json &spell_data)
{
    return parse_spell_info(spell_data, this->spell_list);
}
