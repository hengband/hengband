#pragma once

#include "store/store-util.h"
#include <map>
#include <string>

/*
 * A structure describing a town with
 * stores and buildings
 */
enum class StoreSaleType : int;
struct town_type {
    std::string name;
    std::map<StoreSaleType, store_type> stores;

    store_type &get_store(StoreSaleType sst);
    const store_type &get_store(StoreSaleType sst) const;
};
