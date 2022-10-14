#pragma once
#include "grid/feature-flag-types.h"
#include "system/angband.h"

/*
 * Terrain action flags
 */
#define FAF_DESTROY 0x01
#define FAF_NO_DROP 0x02
#define FAF_CRASH_GLASS 0x04

extern const byte terrain_action_flags[FF_FLAG_MAX];
