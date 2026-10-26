#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
class MeleeWeaponEnchanter : public AbstractWeaponEnchanter {
public:
    virtual ~MeleeWeaponEnchanter() = default;

    void apply_magic() override;

protected:
    MeleeWeaponEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power);

    PlayerType *player_ptr;

    void prepare_magic_application();

private:
    virtual void adjust_for_ego() {}
    void strengthen();
};
