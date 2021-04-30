#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
class AccessoryEnchanter {
public:
    AccessoryEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
    AccessoryEnchanter() = delete;
    virtual ~AccessoryEnchanter() = default;
    void apply_magic_accessary();
private:
    player_type *owner_ptr;
    object_type *o_ptr;
    DEPTH level;
    int power;
};
