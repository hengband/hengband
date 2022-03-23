#pragma once

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class HaftedEnchanter : public MeleeWeaponEnchanter {
public:
    HaftedEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);

protected:
    void decide_skip() override;
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;
};
