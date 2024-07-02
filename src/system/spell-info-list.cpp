#include "system/spell-info-list.h"
#include "realm/realm-types.h"

SpellInfoList SpellInfoList::instance{};

void SpellInfoList::initiallize()
{
    this->spell_list.assign(magic_realm_type::REALM_MAX, std::vector<SpellInfo>(SPELLS_IN_REALM));
}

SpellInfoList &SpellInfoList::get_instance()
{
    return instance;
}
