#pragma once

#include "object-enchant/apply-magic-armors-base.h"
#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class CrownEnchanter : AbstractProtectorEnchanter {
public:
    CrownEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
    CrownEnchanter() = delete;
    virtual ~CrownEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant(){};
    void give_ego_index();
    void give_high_ego_index(){};
    void give_cursed();

private:
    player_type *owner_ptr;
};
