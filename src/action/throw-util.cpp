/*!
 * @file throw-util.cpp
 * @brief 投擲処理関連クラス
 * @date 2021/08/20
 * @author Hourier
 */

#include "action/throw-util.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

it_type::it_type(player_type *creature_ptr, object_type *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken)
    : q_ptr(q_ptr)
    , mult(mult)
    , msec(delay_factor_val * delay_factor_val * delay_factor_val)
    , boomerang(boomerang)
    , shuriken(shuriken)
    , creature_ptr(creature_ptr)
{
}
