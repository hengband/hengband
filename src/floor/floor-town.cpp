#include "floor/floor-town.h"
#include "store/store-util.h"
#include "system/object-type-definition.h"

/* Maximum number of towns */
int16_t max_towns;

/* The towns [max_towns] */
std::vector<town_type> town_info;
