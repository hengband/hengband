#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

struct object_type;
class PlayerType;
class WeaponEnchanter : AbstractWeaponEnchanter {
public:
    WeaponEnchanter(PlayerType *player_ptr, object_type *o_ptr, DEPTH level, int power);
    void apply_magic() override;

protected:
    virtual void enchant() override {}
    virtual void give_ego_index() override {}
    virtual void give_high_ego_index() override {}
    virtual void give_cursed() override {}

private:
    PlayerType *player_ptr;
};
