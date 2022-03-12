#pragma once

#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/angband.h"

class ObjectType;
class PlayerType;
class ArmorEnchanter : public AbstractProtectorEnchanter {
public:
    virtual ~ArmorEnchanter() = default;

protected:
    ArmorEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);

    PlayerType *player_ptr;

    void give_ego_index() override;
    void give_cursed() override;
};
