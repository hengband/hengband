#pragma once

#include "object-enchant/enchanter-base.h"
#include "object-enchant/abstract-protector-enchanter.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class ArmorEnchanter : AbstractProtectorEnchanter {
public:
    ArmorEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
    ArmorEnchanter() = delete;
    virtual ~ArmorEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override;
    void give_cursed() override;

private:
    player_type *owner_ptr;
    bool is_high_ego_generated = false;
};
