#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
class ObjectReadEntity {
public:
    ObjectReadEntity(PlayerType *player_ptr, INVENTORY_IDX item);
    virtual ~ObjectReadEntity() = default;

    void execute(bool known);

private:
    PlayerType *player_ptr;
    INVENTORY_IDX item;

    bool can_read() const;
    void change_virtue_as_read(ObjectType &o_ref);
    void gain_exp_from_item_use(ObjectType *o_ptr, bool is_identified);
};
