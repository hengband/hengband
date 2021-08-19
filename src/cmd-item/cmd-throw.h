#pragma once

#include "system/angband.h"

struct player_type;
class ThrowCommand {
public:
    ThrowCommand(player_type *creature_ptr);
    virtual ~ThrowCommand() = default;
    bool do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken);
    
private:
    player_type *creature_ptr;
};
