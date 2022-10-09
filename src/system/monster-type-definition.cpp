#include "system/monster-type-definition.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-status.h"
#include "system/monster-race-definition.h"
#include "util/string-processor.h"

bool monster_type::is_friendly() const
{
    return this->mflag2.has(MonsterConstantFlagType::FRIENDLY);
}

bool monster_type::is_pet() const
{
    return this->mflag2.has(MonsterConstantFlagType::PET);
}

bool monster_type::is_hostile() const
{
    return !this->is_friendly() && !this->is_pet();
}

bool monster_type::is_original_ap() const
{
    return this->ap_r_idx == this->r_idx;
}

/*!
 * @brief モンスターがアイテム類に擬態しているかどうかを返す
 * @param m_ptr モンスターの参照ポインタ
 * @return モンスターがアイテム類に擬態しているならTRUE、そうでなければFALSE
 * @details
 * ユニークミミックは常時擬態状態
 * 一般モンスターもビハインダーだけ特別扱い
 * その他増やしたい時はis_special_mimic に「|=」で追加すること
 *
 */
bool monster_type::is_mimicry() const
{
    auto is_special_mimic = this->ap_r_idx == MonsterRaceId::BEHINDER;
    if (is_special_mimic) {
        return true;
    }

    const auto &r_ref = r_info[this->ap_r_idx];
    const auto mimic_symbols = "/|\\()[]=$,.!?&`#%<>+~";
    if (angband_strchr(mimic_symbols, r_ref.d_char) == nullptr) {
        return false;
    }

    if (r_ref.kind_flags.has(MonsterKindType::UNIQUE)) {
        return true;
    }

    return r_ref.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) || this->is_asleep();
}

bool monster_type::is_valid() const
{
    return MonsterRace(this->r_idx).is_valid();
}

MonsterRaceId monster_type::get_real_r_idx() const
{
    const auto &r_ref = r_info[this->r_idx];
    if (this->mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
        return this->r_idx;
    }

    return r_ref.kind_flags.has(MonsterKindType::UNIQUE) ? MonsterRaceId::CHAMELEON_K : MonsterRaceId::CHAMELEON;
}

/*!
 * @brief モンスターの真の種族を返す / Extract monster race pointer of a monster's true form
 * @return 本当のモンスター種族参照ポインタ
 */
monster_race &monster_type::get_real_r_ref() const
{
    return r_info[this->get_real_r_idx()];
}

short monster_type::get_remaining_sleep() const
{
    return this->mtimed[MTIMED_CSLEEP];
}

bool monster_type::is_asleep() const
{
    return this->get_remaining_sleep() > 0;
}

short monster_type::get_remaining_acceleration() const
{
    return this->mtimed[MTIMED_FAST];
}

bool monster_type::is_accelerated() const
{
    return this->get_remaining_acceleration() > 0;
}

short monster_type::get_remaining_deceleration() const
{
    return this->mtimed[MTIMED_SLOW];
}

bool monster_type::is_decelerated() const
{
    return this->get_remaining_deceleration() > 0;
}

short monster_type::get_remaining_stun() const
{
    return this->mtimed[MTIMED_STUNNED];
}

bool monster_type::is_stunned() const
{
    return this->get_remaining_stun() > 0;
}

short monster_type::get_remaining_confusion() const
{
    return this->mtimed[MTIMED_CONFUSED];
}

bool monster_type::is_confused() const
{
    return this->get_remaining_confusion() > 0;
}

short monster_type::get_remaining_fear() const
{
    return this->mtimed[MTIMED_MONFEAR];
}

bool monster_type::is_fearful() const
{
    return this->get_remaining_fear() > 0;
}

short monster_type::get_remaining_invulnerability() const
{
    return this->mtimed[MTIMED_INVULNER];
}

bool monster_type::is_invulnerable() const
{
    return this->get_remaining_invulnerability() > 0;
}

/*
 * @brief 悪夢モード、一時加速、一時減速に基づくモンスターの現在速度を返す
 */
byte monster_type::get_temporary_speed() const
{
    auto speed = this->mspeed;
    if (ironman_nightmare) {
        speed += 5;
    }

    if (this->is_accelerated()) {
        speed += 10;
    }

    if (this->is_decelerated()) {
        speed -= 10;
    }

    return speed;
}
