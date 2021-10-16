#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class RingEnchanter : EnchanterBase {
public:
    RingEnchanter(player_type *player_ptr, object_type *o_ptr, DEPTH level, int power);
    virtual ~RingEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant() override;
    void give_ego_index() override;
    void give_high_ego_index() override;
    void give_cursed() override;

private:
    player_type *player_ptr;
    object_type *o_ptr;
    DEPTH level;
    int power;
};
