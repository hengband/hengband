#pragma once

#include "object-enchant/abstract-protector-enchanter.h"
#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

struct object_type;
struct player_type;
class CrownEnchanter : AbstractProtectorEnchanter {
public:
    CrownEnchanter(player_type *player_ptr, object_type *o_ptr, DEPTH level, int power);
    CrownEnchanter() = delete;
    virtual ~CrownEnchanter() = default;
    void apply_magic() override;

protected:
    void enchant() override{};
    void give_ego_index() override;
    void give_high_ego_index() override{};
    void give_cursed() override;

private:
    player_type *player_ptr;
};
