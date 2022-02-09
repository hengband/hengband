#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class ShieldEnchanter : AbstractProtectorEnchanter {
public:
    ShieldEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);
    virtual ~ShieldEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    PlayerType *player_ptr;
};
