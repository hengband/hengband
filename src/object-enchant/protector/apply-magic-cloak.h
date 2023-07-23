#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
class CloakEnchanter : public AbstractProtectorEnchanter {
public:
    CloakEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power);
    void apply_magic() override;

protected:
    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    PlayerType *player_ptr;
};
