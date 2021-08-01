#pragma once

#include "object-enchant/abstract-protector-enchanter.h"
#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class CloakEnchanter : AbstractProtectorEnchanter {
public:
    CloakEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
    CloakEnchanter() = delete;
    virtual ~CloakEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant(){};
    void give_ego_index(){};
    void give_high_ego_index(){};
    void give_cursed(){};

private:
    player_type *owner_ptr;
};
