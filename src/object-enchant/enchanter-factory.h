#pragma once

#include <memory>

class EnchanterBase;
class ObjectType;
class PlayerType;
class EnchanterFactory {
public:
    static std::unique_ptr<EnchanterBase> create_enchanter(PlayerType *player_ptr, ObjectType *o_ptr, int lev, int power);

private:
    EnchanterFactory() = delete;
};
