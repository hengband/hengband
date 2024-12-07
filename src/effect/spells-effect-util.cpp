#include "effect/spells-effect-util.h"
#include "monster/monster-describer.h"
#include "pet/pet-fall-off.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include <algorithm>

bool sukekaku;

int project_length = 0;

int project_m_n;
POSITION project_m_x;
POSITION project_m_y;
POSITION monster_target_x;
POSITION monster_target_y;

CapturedMonsterType::CapturedMonsterType()
    : r_idx(MonraceId::PLAYER)
{
}

FallOffHorseEffect::FallOffHorseEffect(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

void FallOffHorseEffect::set_shake_off(int damage)
{
    this->shake_off_damage = std::min(damage, FALL_OFF_DAMAGE_MAX);
}

void FallOffHorseEffect::set_fall_off(int damage)
{
    this->fall_off_damage = std::min(damage, FALL_OFF_DAMAGE_MAX);
}

void FallOffHorseEffect::apply() const
{
    if (!this->player_ptr->riding) {
        return;
    }

    if (this->shake_off_damage <= 0 && this->fall_off_damage <= 0) {
        return;
    }

    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto m_name = monster_desc(this->player_ptr, &floor.m_list[player_ptr->riding], 0);

    if (this->shake_off_damage > 0) {
        if (process_fall_off_horse(this->player_ptr, this->shake_off_damage, false)) {
            msg_format(_("%s^に振り落とされた！", "%s^ has thrown you off!"), m_name.data());
            return;
        }
    }

    if (this->fall_off_damage > 0) {
        if (process_fall_off_horse(this->player_ptr, this->fall_off_damage, false)) {
            msg_format(_("%s^から落ちてしまった！", "You have fallen from %s."), m_name.data());
        }
    }
}
