#pragma once

#include "object/tval-types.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/angband.h"
#include <map>

struct BaseitemKey {
    ItemKindType tval;
    OBJECT_SUBTYPE_VALUE sval;
};

extern const std::map<StoreSaleType, std::vector<BaseitemKey>> store_regular_sale_table;
extern const std::map<StoreSaleType, std::vector<BaseitemKey>> store_sale_table;
