#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
class ObjectReadEntity {
public:
    ObjectReadEntity(PlayerType *player_ptr, INVENTORY_IDX i_idx);
    virtual ~ObjectReadEntity() = default;

    void execute(bool known);

private:
    PlayerType *player_ptr;
    INVENTORY_IDX i_idx;

    bool can_read() const;
    void change_virtue_as_read(ItemEntity &o_ref);
    void gain_exp_from_item_use(ItemEntity *o_ptr, bool is_identified);
};
