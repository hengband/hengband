#pragma once

#include "object-enchant/protector/apply-magic-armor.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
class SoftArmorEnchanter : public ArmorEnchanter {
public:
    SoftArmorEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power);
    void apply_magic() override;

protected:
    void sval_enchant() override;
    void give_high_ego_index() override;

private:
    bool is_high_ego_generated = false;
};
