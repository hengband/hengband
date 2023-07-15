#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

#include "object-enchant/tr-flags.h"

class ItemEntity;
TrFlags object_flags(const ItemEntity *o_ptr);
TrFlags object_flags_known(const ItemEntity *o_ptr);
