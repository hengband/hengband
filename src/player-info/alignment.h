#pragma once

#include "system/angband.h"

struct player_type;
class PlayerAlignment {
public:
    PlayerAlignment(player_type *creature_ptr);
    PlayerAlignment() = delete;
    virtual ~PlayerAlignment() = default;
    concptr your_alignment(bool with_value = false);
private:
    player_type *creature_ptr;
    concptr alignment_label();
};
