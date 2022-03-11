#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class BowEnchanter : AbstractWeaponEnchanter {
public:
    BowEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    PlayerType *player_ptr;
};
