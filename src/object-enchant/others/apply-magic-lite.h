#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class LiteEnchanter : EnchanterBase {
public:
    LiteEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, int power);
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;

private:
    PlayerType *player_ptr;
    ObjectType *o_ptr;
    int power;

    void add_dark_flag();
};
