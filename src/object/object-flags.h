#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

#include "object-enchant/tr-flags.h"

class ObjectType;
TrFlags object_flags(const ObjectType *o_ptr);
TrFlags object_flags_known(const ObjectType *o_ptr);
