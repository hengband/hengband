#pragma once

#include <string>

class player_type;
class ItemUseChecker {
public:
    ItemUseChecker(player_type *player_ptr);
    virtual ~ItemUseChecker() = default;

    bool check_stun(std::string_view mes) const;

private:
    player_type *player_ptr;
};
