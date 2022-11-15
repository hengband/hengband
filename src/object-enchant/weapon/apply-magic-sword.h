﻿#pragma once

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class SwordEnchanter : public MeleeWeaponEnchanter {
public:
    SwordEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);

protected:
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;

private:
    MeleeWeaponEnchantFlags enchant_flags() const override;
};
