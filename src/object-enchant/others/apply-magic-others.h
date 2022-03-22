#pragma once

#include "object-enchant/enchanter-base.h"

class ObjectType;
class PlayerType;
class OtherItemsEnchanter : public EnchanterBase {
public:
    OtherItemsEnchanter(PlayerType *player_ptr, ObjectType *o_ptr);
    void apply_magic() override;

    void sval_enchant() override{};
    void give_ego_index() override{};
    void give_high_ego_index() override{};
    void give_cursed() override{};

private:
    PlayerType *player_ptr;
    ObjectType *o_ptr;

    void enchant_wand_staff();
    void generate_figurine();
    void generate_corpse();
    void generate_statue();
    void generate_chest();
};
