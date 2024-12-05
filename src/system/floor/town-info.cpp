#include "system/floor/town-info.h"

store_type &TownInfo::get_store(StoreSaleType sst)
{
    return this->stores.at(sst);
}

const store_type &TownInfo::get_store(StoreSaleType sst) const
{
    return this->stores.at(sst);
}

store_type &TownInfo::emplace(StoreSaleType sst)
{
    return this->stores.emplace(std::piecewise_construct, std::make_tuple(sst), std::make_tuple()).first->second;
}
