#include "system/floor/town-info.h"

store_type &town_type::get_store(StoreSaleType sst)
{
    return this->stores.at(sst);
}

const store_type &town_type::get_store(StoreSaleType sst) const
{
    return this->stores.at(sst);
}
