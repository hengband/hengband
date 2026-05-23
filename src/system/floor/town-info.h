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

    void init_name(std::string_view name);
    const std::string &get_name() const;

    Store &get_store(StoreSaleType sst);
    const Store &get_store(StoreSaleType sst) const;
    Store &emplace(StoreSaleType sst);

private:
    std::string town_name;
    std::map<StoreSaleType, Store> stores;
};
