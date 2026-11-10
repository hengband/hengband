#pragma once

#include "realm/realm-types.h"
#include "spell/magic-type.h"
#include "system/angband.h"

constexpr auto NUM_TECHNIC = std::ssize(TECHNIC_REALM_RANGE);

extern const magic_type technic_info[NUM_TECHNIC][32];
