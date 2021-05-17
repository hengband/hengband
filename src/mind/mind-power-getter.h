#pragma once

#include "system/angband.h"

struct player_type;
class MindPowerGetter {
public:
    MindPowerGetter(player_type *caster_ptr);
    virtual ~MindPowerGetter() = default;
    bool get_mind_power(SPELL_IDX *sn, bool only_browse);

private:
    player_type *caster_ptr;
};
