#include "system/spell-info-list.h"
#include "realm/realm-types.h"
#include <algorithm>

SpellInfoList SpellInfoList::instance{};

void SpellInfoList::initiallize()
{
    this->spell_list.assign(magic_realm_type::REALM_MAX, std::vector<SpellInfo>(SPELLS_IN_REALM));
}

SpellInfoList &SpellInfoList::get_instance()
{
    return instance;
}

std::optional<short> SpellInfoList::get_spell_id(int realm_id, std::string_view spell_tag)
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
