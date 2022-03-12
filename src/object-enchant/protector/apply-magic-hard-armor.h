#pragma once

#include "object-enchant/protector/apply-magic-armor.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class HardArmorEnchanter : ArmorEnchanter {
public:
    HardArmorEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_high_ego_index() override{};
};
