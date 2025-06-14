#include "system/spell-info-list.h"
#include "info-reader/spell-reader.h"
#include "realm/realm-types.h"
#include <algorithm>

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

errr SpellInfoList::parse(nlohmann::json &spell_data)
{
    return parse_spell_info(spell_data, this->spell_list);
}
