#include "angband.h"
#include "floor.h"
#include "floor-save.h"

/*
 * The array of floor [MAX_WID][MAX_HGT].
 * Not completely allocated, that would be inefficient
 * Not completely hardcoded, that would overflow memory
 */
floor_type floor_info;

/*
 * The array of saved floors
 */
saved_floor_type saved_floors[MAX_SAVED_FLOORS];

