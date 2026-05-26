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
    void overwrite_town_name();
    const std::string &get_name() const;
    bool is_alias_initialized() const;

    Store &get_store(StoreSaleType sst);
    const Store &get_store(StoreSaleType sst) const;
    Store &emplace(StoreSaleType sst);

private:
    std::string town_name;
    std::string town_alias; //!< 辺境の地の別名. 荒野なしオプションがONの時はこれを町の名前として表示する.
    std::map<StoreSaleType, Store> stores;
};
