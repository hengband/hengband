#pragma once

#include "system/angband.h"

/*
 *  Special essence id for Weapon smith
 */
#define MIN_SPECIAL_ESSENCE 200

#define ESSENCE_ATTACK        (MIN_SPECIAL_ESSENCE + 0)
#define ESSENCE_AC            (MIN_SPECIAL_ESSENCE + 1)
#define ESSENCE_TMP_RES_ACID  (MIN_SPECIAL_ESSENCE + 2)
#define ESSENCE_TMP_RES_ELEC  (MIN_SPECIAL_ESSENCE + 3)
#define ESSENCE_TMP_RES_FIRE  (MIN_SPECIAL_ESSENCE + 4)
#define ESSENCE_TMP_RES_COLD  (MIN_SPECIAL_ESSENCE + 5)
#define ESSENCE_SH_FIRE       (MIN_SPECIAL_ESSENCE + 6)
#define ESSENCE_SH_ELEC       (MIN_SPECIAL_ESSENCE + 7)
#define ESSENCE_SH_COLD       (MIN_SPECIAL_ESSENCE + 8)
#define ESSENCE_RESISTANCE    (MIN_SPECIAL_ESSENCE + 9)
#define ESSENCE_SUSTAIN       (MIN_SPECIAL_ESSENCE + 10)
#define ESSENCE_SLAY_GLOVE    (MIN_SPECIAL_ESSENCE + 11)

extern concptr essence_name[];

struct player_type;
void do_cmd_kaji(player_type *creature_ptr, bool only_browse);
