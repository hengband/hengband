#pragma once

#include "object-enchant/enchanter-base.h"
#include "object-enchant/abstract-protector-enchanter.h"
#include "system/angband.h"

struct object_type;
class PlayerType;
class ArmorEnchanter : AbstractProtectorEnchanter {
public:
    ArmorEnchanter(PlayerType *player_ptr, object_type *o_ptr, DEPTH level, int power);
    virtual ~ArmorEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override;
    void give_cursed() override;

private:
    PlayerType *player_ptr;
    bool is_high_ego_generated = false;
};
