#pragma once

#include "object-enchant/accessory-enchanter-base.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class RingEnchanter : AccessoryEnchanterBase {
public:
    RingEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
    RingEnchanter() = delete;
    virtual ~RingEnchanter() = default;
    void apply_magic_accessary() override;

protected:
    void enchant() override;
    void give_ego_index() override;
    void give_high_ego_index() override;
    void give_cursed() override;

private:
    player_type *owner_ptr;
    object_type *o_ptr;
    DEPTH level;
    int power;
};
