#include "mspell/mspell-attack/abstract-mspell.h"
#include "monster/monster-update.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "system/player-type-definition.h"

AbstractMSpellAttack::AbstractMSpellAttack(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire)
    : player_ptr(player_ptr)
    , m_idx(m_idx)
    , t_idx(0)
    , ability(ability)
    , data(std::move(data))
    , target_type(target_type)
    , fire(std::move(fire))
{
}

AbstractMSpellAttack::AbstractMSpellAttack(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire)
    : player_ptr(player_ptr)
    , m_idx(m_idx)
    , t_idx(t_idx)
    , ability(ability)
    , data(std::move(data))
    , target_type(target_type)
    , fire(std::move(fire))
{
}

MonsterSpellResult AbstractMSpellAttack::shoot(POSITION y, POSITION x)
{
    if (!this->data.contain) {
        return MonsterSpellResult::make_invalid();
    }

    this->data.msg.output(this->player_ptr, this->m_idx, this->t_idx, this->target_type);

    const auto dam = monspell_damage(this->player_ptr, this->ability, this->m_idx, DAM_ROLL);
    const auto proj_res = fire(y, x, dam, data.type);
    if (this->target_type == MONSTER_TO_PLAYER) {
        this->data.drs.execute(this->player_ptr, this->m_idx);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}
