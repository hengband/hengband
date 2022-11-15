#pragma once

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
class SwordEnchanter : public MeleeWeaponEnchanter {
public:
    SwordEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power);

    void apply_magic() override;

protected:
    void decide_skip() override;
    void sval_enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;
};
