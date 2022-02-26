#pragma once

#include "object/tval-types.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/angband.h"

#define STORE_CHOICES 48 /* Number of items to choose stock from */

struct store_stock_item_type {
    ItemKindType tval;
    OBJECT_SUBTYPE_VALUE sval;
};

extern store_stock_item_type store_regular_table[MAX_STORES][STORE_MAX_KEEP];
extern store_stock_item_type store_table[MAX_STORES][STORE_CHOICES];
