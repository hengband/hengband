#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
class ObjectQuaffEntity {
public:
    ObjectQuaffEntity(PlayerType *player_ptr);
    virtual ~ObjectQuaffEntity() = default;

    void execute(INVENTORY_IDX item);

private:
    PlayerType *player_ptr;

    bool can_quaff();
    ObjectType copy_object(const INVENTORY_IDX item);
    void moisten(const ObjectType &o_ref);
};
