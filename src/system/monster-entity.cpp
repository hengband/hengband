#include "system/monster-entity.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-status.h"
#include "system/monster-race-info.h"
#include "term/term-color-types.h"
#include "util/string-processor.h"
#include <algorithm>

bool MonsterEntity::is_friendly() const
{
    return this->mflag2.has(MonsterConstantFlagType::FRIENDLY);
}

bool MonsterEntity::is_pet() const
{
    return this->mflag2.has(MonsterConstantFlagType::PET);
}

bool MonsterEntity::is_hostile() const
{
    return !this->is_friendly() && !this->is_pet();
}

bool MonsterEntity::is_named() const
{
    return !this->nickname.empty();
}

bool MonsterEntity::is_named_pet() const
{
    return this->is_pet() && this->is_named();
}

bool MonsterEntity::is_original_ap() const
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
bool MonsterEntity::is_mimicry() const
{
    auto is_special_mimic = this->ap_r_idx == MonsterRaceId::BEHINDER;
    if (is_special_mimic) {
        return true;
    }

    const auto &r_ref = monraces_info[this->ap_r_idx];
    const auto mimic_symbols = "/|\\()[]=$,.!?&`#%<>+~";
    if (angband_strchr(mimic_symbols, r_ref.d_char) == nullptr) {
        return false;
    }

    if (r_ref.kind_flags.has(MonsterKindType::UNIQUE)) {
        return true;
    }

    return r_ref.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) || this->is_asleep();
}

bool MonsterEntity::is_valid() const
{
    return MonsterRace(this->r_idx).is_valid();
}

MonsterRaceId MonsterEntity::get_real_r_idx() const
{
    const auto &r_ref = monraces_info[this->r_idx];
    if (this->mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
        return this->r_idx;
    }

    return r_ref.kind_flags.has(MonsterKindType::UNIQUE) ? MonsterRaceId::CHAMELEON_K : MonsterRaceId::CHAMELEON;
}

/*!
 * @brief モンスターの真の種族を返す / Extract monster race pointer of a monster's true form
 * @return 本当のモンスター種族参照ポインタ
 */
MonsterRaceInfo &MonsterEntity::get_real_r_ref() const
{
    return monraces_info[this->get_real_r_idx()];
}

short MonsterEntity::get_remaining_sleep() const
{
    return this->mtimed[MTIMED_CSLEEP];
}

bool MonsterEntity::is_dead() const
{
    return this->hp < 0;
}

bool MonsterEntity::is_asleep() const
{
    return this->get_remaining_sleep() > 0;
}

short MonsterEntity::get_remaining_acceleration() const
{
    return this->mtimed[MTIMED_FAST];
}

bool MonsterEntity::is_accelerated() const
{
    return this->get_remaining_acceleration() > 0;
}

short MonsterEntity::get_remaining_deceleration() const
{
    return this->mtimed[MTIMED_SLOW];
}

bool MonsterEntity::is_decelerated() const
{
    return this->get_remaining_deceleration() > 0;
}

short MonsterEntity::get_remaining_stun() const
{
    return this->mtimed[MTIMED_STUNNED];
}

bool MonsterEntity::is_stunned() const
{
    return this->get_remaining_stun() > 0;
}

short MonsterEntity::get_remaining_confusion() const
{
    return this->mtimed[MTIMED_CONFUSED];
}

bool MonsterEntity::is_confused() const
{
    return this->get_remaining_confusion() > 0;
}

short MonsterEntity::get_remaining_fear() const
{
    return this->mtimed[MTIMED_MONFEAR];
}

bool MonsterEntity::is_fearful() const
{
    return this->get_remaining_fear() > 0;
}

short MonsterEntity::get_remaining_invulnerability() const
{
    return this->mtimed[MTIMED_INVULNER];
}

bool MonsterEntity::is_invulnerable() const
{
    return this->get_remaining_invulnerability() > 0;
}

/*
 * @brief 悪夢モード、一時加速、一時減速に基づくモンスターの現在速度を返す
 */
byte MonsterEntity::get_temporary_speed() const
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

/*!
 * @brief モンスターが生命体かどうかを返す
 * @param is_apperance たぬき、カメレオン、各種誤認ならtrue
 * @return 生命体ならばtrue
 * @todo kind_flags をMonsterEntityへコピーする (将来的なモンスター仕様の拡張)
 */
bool MonsterEntity::has_living_flag(bool is_apperance) const
{
    const auto &monrace = monraces_info[is_apperance ? this->ap_r_idx : this->r_idx];
    return monrace.has_living_flag();
}

bool MonsterEntity::is_explodable() const
{
    const auto &monrace = monraces_info[this->r_idx];
    return monrace.is_explodable();
}

std::string MonsterEntity::get_died_message() const
{
    const auto &monrace = monraces_info[this->r_idx];
    return monrace.get_died_message();
}

/*!
 * @brief モンスターの状態（無敵、起きているか、HPの割合）に応じてHPバーの色と長さを算出する
 * @return HPバーの色と長さ(1-10)のペア
 */
std::pair<TERM_COLOR, int> MonsterEntity::get_hp_bar_data() const
{
    const auto percent = (this->maxhp > 0) ? (100 * this->hp / this->maxhp) : 0;
    const auto len = std::clamp(percent / 10 + 1, 1, 10);

    if (this->is_invulnerable()) {
        return { TERM_WHITE, len };
    }
    if (this->is_asleep()) {
        return { TERM_BLUE, len };
    }
    if (percent >= 100) {
        return { TERM_L_GREEN, len };
    }
    if (percent >= 60) {
        return { TERM_YELLOW, len };
    }
    if (percent >= 25) {
        return { TERM_ORANGE, len };
    }
    if (percent >= 10) {
        return { TERM_L_RED, len };
    }
    return { TERM_RED, len };
}
