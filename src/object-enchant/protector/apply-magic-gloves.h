#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class GlovesEnchanter : AbstractProtectorEnchanter {
public:
    GlovesEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);
    virtual ~GlovesEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    PlayerType *player_ptr;
};
