#pragma once

#include "object-enchant/protector/apply-magic-armor.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class SoftArmorEnchanter : ArmorEnchanter {
public:
    SoftArmorEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);
    void apply_magic() override;
};
