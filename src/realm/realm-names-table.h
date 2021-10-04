#pragma once

#include "system/angband.h"
#include "realm/realm-types.h"

#define VALID_REALM        (MAX_REALM + MAX_MAGIC - MIN_TECHNIC + 1)

#define is_magic(A) (((A) > REALM_NONE) && ((A) < MAX_MAGIC + 1))
#define tval2realm(A) ((A) - TV_LIFE_BOOK + 1)
#define technic2magic(A)      (is_magic(A) ? (A) : (A) - MIN_TECHNIC + 1 + MAX_MAGIC)
#define is_good_realm(REALM)   ((REALM) == REALM_LIFE || (REALM) == REALM_CRUSADE)

extern const concptr realm_names[];
#ifdef JP
extern const concptr E_realm_names[];
#endif
