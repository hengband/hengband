#pragma once

#include "system/angband.h"

class PlayerType;
class ThrowCommand {
public:
    ThrowCommand(PlayerType *player_ptr);
    virtual ~ThrowCommand() = default;
    bool do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken);

private:
    PlayerType *player_ptr;
};
