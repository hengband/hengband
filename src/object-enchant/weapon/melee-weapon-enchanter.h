#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class MeleeWeaponEnchanter : AbstractWeaponEnchanter {
public:
    virtual ~MeleeWeaponEnchanter() = default;

    void apply_magic() override{};

protected:
    MeleeWeaponEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);

    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};
    void strengthen(){};

private:
    PlayerType *player_ptr;
};
