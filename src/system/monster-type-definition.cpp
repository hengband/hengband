#include "system/monster-type-definition.h"
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

    return r_ref.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) || monster_csleep_remaining(this);
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
