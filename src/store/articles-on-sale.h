#pragma once

#include "object/tval-types.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/angband.h"
#include <map>

struct store_stock_item_type {
    ItemKindType tval;
    OBJECT_SUBTYPE_VALUE sval;
};

extern const std::map<StoreSaleType, std::vector<store_stock_item_type>> store_regular_sale_table;
extern const std::map<StoreSaleType, std::vector<store_stock_item_type>> store_sale_table;
