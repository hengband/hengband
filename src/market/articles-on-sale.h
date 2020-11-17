#pragma once

#include "store/store-owners.h"
#include "system/angband.h"

#define STORE_CHOICES   48 /* Number of items to choose stock from */

extern byte store_table[MAX_STORES][STORE_CHOICES][2];
