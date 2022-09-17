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
 *
 * モンスターがアイテム類に擬態しているかどうかを返す。
 * 擬態の条件:
 * - シンボルが以下のいずれかであること: /|\()[]=$,.!?&`#%<>+~
 * - 動かない、もしくは眠っていること
 *
 * 但し、以下のモンスターは例外的に擬態しているとする
 * それ・生ける虚無『ヌル』・ビハインダー
 *
 * @param m_ptr モンスターの参照ポインタ
 * @return モンスターがアイテム類に擬態しているならTRUE、そうでなければFALSE
 */
bool monster_type::is_mimicry() const
{
    if (this->ap_r_idx == MonsterRaceId::IT || this->ap_r_idx == MonsterRaceId::NULL_ || this->ap_r_idx == MonsterRaceId::BEHINDER) {
        return true;
    }

    auto *r_ptr = &r_info[this->ap_r_idx];

    if (angband_strchr("/|\\()[]=$,.!?&`#%<>+~", r_ptr->d_char) == nullptr) {
        return false;
    }

    if (r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE) && !monster_csleep_remaining(this)) {
        return false;
    }

    return true;
}
