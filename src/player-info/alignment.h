#pragma once

#include "system/angband.h"

enum class update_alignment_type {
    ALIGNMENT_SUBSTITUTION,
    ALIGNMENT_ADDTION,
    ALIGNMENT_SUBTRACTION,
};

struct player_type;
class PlayerAlignment {
public:
    PlayerAlignment(player_type *creature_ptr);
    PlayerAlignment() = delete;
    virtual ~PlayerAlignment() = default;
    concptr get_alignment_description(bool with_value = false);
private:
    player_type *creature_ptr;
    concptr alignment_label();
};
