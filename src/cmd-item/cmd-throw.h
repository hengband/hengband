﻿#pragma once

#include "system/angband.h"

class player_type;
class ThrowCommand {
public:
    ThrowCommand(player_type *player_ptr);
    virtual ~ThrowCommand() = default;
    bool do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken);
    
private:
    player_type *player_ptr;
};
