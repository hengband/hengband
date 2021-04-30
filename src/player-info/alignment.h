#pragma once

#include "system/angband.h"

struct player_type;
class PlayerAlignment {
public:
    PlayerAlignment(player_type *creature_ptr);
    PlayerAlignment() = delete;
    virtual ~PlayerAlignment() = default;
    concptr get_alignment_description(bool with_value = false);
    void update_alignment();
private:
    player_type *creature_ptr;
    concptr alignment_label();
    void bias_good_alignment(int value);
    void bias_evil_alignment(int value);
    void reset_alignment();
};
