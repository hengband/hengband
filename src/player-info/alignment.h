#pragma once

#include "system/angband.h"
#include <string>

class PlayerType;
class PlayerAlignment {
public:
    PlayerAlignment(PlayerType *player_ptr);
    virtual ~PlayerAlignment() = default;
    std::string get_alignment_description(bool with_value = false);
    void update_alignment();

private:
    PlayerType *player_ptr;
    concptr alignment_label();
    void bias_good_alignment(int value);
    void bias_evil_alignment(int value);
    void reset_alignment();
};
