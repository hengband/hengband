#pragma once

#include "store/store-util.h"
#include <map>
#include <string>

/*
 * A structure describing a town with
 * stores and buildings
 */
enum class StoreSaleType : int;
class TownInfo {
public:
    TownInfo() = default;

    std::string name;

    Store &get_store(StoreSaleType sst);
    const Store &get_store(StoreSaleType sst) const;
    Store &emplace(StoreSaleType sst);

private:
    std::map<StoreSaleType, Store> stores;
};
