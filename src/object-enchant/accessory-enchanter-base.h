#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
class AccessoryEnchanterBase {
public:
    virtual void apply_magic_accessary() = 0;

protected:
    AccessoryEnchanterBase() = default;
    virtual ~AccessoryEnchanterBase() = default;
    virtual void enchant() = 0;
    virtual void give_ego_index() = 0;
    virtual void give_high_ego_index() = 0;
    virtual void give_cursed() = 0;
};
