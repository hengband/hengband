#pragma once

#include <memory>

class EnchanterBase;
class ItemEntity;
class PlayerType;
class EnchanterFactory {
public:
    static std::unique_ptr<EnchanterBase> create_enchanter(PlayerType *player_ptr, ItemEntity *o_ptr, int lev, int power);

private:
    EnchanterFactory() = delete;
};
