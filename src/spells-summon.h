#pragma once
#include "monster.h"

extern bool summon_specific(MONSTER_IDX who, POSITION y1, POSITION x1, DEPTH lev, int type, BIT_FLAGS mode, SYMBOL_CODE symbol);
extern bool summon_named_creature(MONSTER_IDX who, POSITION oy, POSITION ox, MONRACE_IDX r_idx, BIT_FLAGS mode);

extern bool trump_summoning(int num, bool pet, POSITION y, POSITION x, DEPTH lev, int type, BIT_FLAGS mode);
extern bool cast_summon_demon(int power);
extern bool item_tester_offer(object_type *o_ptr);
extern bool cast_summon_greater_demon(void);
extern bool summon_kin_player(DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode);

