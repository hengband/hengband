﻿#pragma once

#include "object-enchant/enchanter-base.h"
#include "system/angband.h"

class ObjectType;
class AbstractWeaponEnchanter : public EnchanterBase {
public:
    virtual ~AbstractWeaponEnchanter() = default;

protected:
    AbstractWeaponEnchanter(ObjectType *o_ptr, DEPTH level, int power);
    ObjectType *o_ptr;
    DEPTH level;
    int power;

    void give_killing_bonus();
};
