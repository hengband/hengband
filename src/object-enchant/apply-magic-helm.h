#pragma once

#include "object-enchant/apply-magic-armors-base.h"
#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class HelmEnchanter : ArmorEnchanterBase {
public:
    HelmEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
    HelmEnchanter() = delete;
    virtual ~HelmEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant(){};
    void give_ego_index();
    void give_high_ego_index(){};
    void give_cursed();

private:
    player_type *owner_ptr;
};
