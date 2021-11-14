#pragma once

#include "system/angband.h"

#define SCROBJ_FAKE_OBJECT 0x00000001
#define SCROBJ_FORCE_DETAIL 0x00000002
struct object_type;
class PlayerType;
bool screen_object(PlayerType *player_ptr, object_type *o_ptr, BIT_FLAGS mode);
