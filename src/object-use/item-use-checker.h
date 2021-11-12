#pragma once

#include <string>

class PlayerType;
class ItemUseChecker {
public:
    ItemUseChecker(PlayerType *player_ptr);
    virtual ~ItemUseChecker() = default;

    bool check_stun(std::string_view mes) const;

private:
    PlayerType *player_ptr;
};
