#pragma once

#include <string>

struct player_type;
class ItemUseChecker {
public:
    ItemUseChecker(player_type *player_ptr);
    virtual ~ItemUseChecker() = default;

    bool check_stun(std::string_view mes) const;

private:
    player_type *player_ptr;
};
