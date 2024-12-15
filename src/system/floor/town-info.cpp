#include "system/floor/town-info.h"

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
