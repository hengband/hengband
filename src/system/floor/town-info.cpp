#include "system/floor/town-info.h"

void TownInfo::init_name(std::string_view name)
{
    if (this->town_name.empty()) {
        this->town_name = name;
        return;
    }

    this->town_alias = name;
}

void TownInfo::overwrite_town_name()
{
    this->town_name = this->town_alias;
}

const std::string &TownInfo::get_name() const
{
    return this->town_name;
}

bool TownInfo::is_alias_initialized() const
{
    return !this->town_alias.empty();
}

Store &TownInfo::get_store(StoreSaleType sst)
{
    return this->stores.at(sst);
}

const Store &TownInfo::get_store(StoreSaleType sst) const
{
    return this->stores.at(sst);
}

Store &TownInfo::emplace(StoreSaleType sst)
{
    return this->stores.emplace(std::piecewise_construct, std::make_tuple(sst), std::make_tuple()).first->second;
}
