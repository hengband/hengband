#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void dump_mutations(player_type *creature_ptr, FILE *out_file);
