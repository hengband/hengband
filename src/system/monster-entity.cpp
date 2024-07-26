#include "system/monster-entity.h"
#include "core/speed-table.h"
#include "game-option/birth-options.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-pain-describer.h"
#include "monster/monster-status.h"
#include "system/angband-system.h"
#include "system/monster-race-info.h"
#include "system/redrawing-flags-updater.h"
#include "term/term-color-types.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <algorithm>

/*!
 * @brief モンスターの属性に基づいた敵対関係の有無を返す
 * @param sub_align1 モンスター1のサブフラグ
 * @param sub_align2 モンスター2のサブフラグ
 * @return 敵対関係にあるか否か
 */
bool MonsterEntity::check_sub_alignments(const byte sub_align1, const byte sub_align2)
{
    if (sub_align1 == sub_align2) {
        return false;
    }

    auto this_evil = any_bits(sub_align1, SUB_ALIGN_EVIL);
    this_evil &= any_bits(sub_align2, SUB_ALIGN_GOOD);
    if (this_evil) {
        return true;
    }

    auto this_good = any_bits(sub_align1, SUB_ALIGN_GOOD);
    this_good &= any_bits(sub_align2, SUB_ALIGN_EVIL);
    return this_good;
}

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

/*!
 * @brief モンスターの属性に基づいた敵対関係の有無を返す
 * @param other 比較対象モンスターへの参照
 * @return 敵対関係にあるか否か
 */
bool MonsterEntity::is_hostile_to_melee(const MonsterEntity &other) const
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return !this->is_pet() && !other.is_pet();
    }

    const auto &monrace1 = this->get_monrace();
    const auto &monrace2 = other.get_monrace();
    const auto is_m1_wild = monrace1.wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
    const auto is_m2_wild = monrace2.wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
    if (is_m1_wild && is_m2_wild) {
        if (!this->is_pet() && !other.is_pet()) {
            return false;
        }
    }

    if (this->is_hostile_align(other.sub_align)) {
        if (this->mflag2.has_not(MonsterConstantFlagType::CHAMELEON) || other.mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
            return true;
        }
    }

    return this->is_hostile() != other.is_hostile();
}

/*!
 * @brief モンスターの属性に基づいた敵対関係の有無を返す
 * @param other 比較対象のサブフラグ
 * @return 敵対関係にあるか否か
 */
bool MonsterEntity::is_hostile_align(const byte other_sub_align) const
{
    return MonsterEntity::check_sub_alignments(this->sub_align, other_sub_align);
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

    const auto &monrace = this->get_appearance_monrace();
    if (!monrace.symbol_char_is_any_of(R"(/|\()[]="$,.!?&`#%<>+~)")) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        return true;
    }

    return monrace.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) || this->is_asleep();
}

bool MonsterEntity::is_valid() const
{
    return MonraceList::is_valid(this->r_idx);
}

MonsterRaceId MonsterEntity::get_real_monrace_id() const
{
    const auto &monrace = this->get_monrace();
    if (this->mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
        return this->r_idx;
    }

    return monrace.kind_flags.has(MonsterKindType::UNIQUE) ? MonsterRaceId::CHAMELEON_K : MonsterRaceId::CHAMELEON;
}

/*!
 * @brief モンスターの真の種族定義を返す (CHAMAELEONフラグ専用)
 * @return 真のモンスター種族参照
 */
MonsterRaceInfo &MonsterEntity::get_real_monrace() const
{
    return monraces_info[this->get_real_monrace_id()];
}

MonsterRaceInfo &MonsterEntity::get_appearance_monrace() const
{
    return monraces_info[this->ap_r_idx];
}

MonsterRaceInfo &MonsterEntity::get_monrace() const
{
    return monraces_info[this->r_idx];
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
    const auto &monrace = is_apperance ? this->get_appearance_monrace() : this->get_monrace();
    return monrace.has_living_flag();
}

/*!
 * @brief モンスターが自爆するか否か
 * @return 自爆するならtrue
 */
bool MonsterEntity::is_explodable() const
{
    const auto &monrace = this->get_monrace();
    return monrace.is_explodable();
}

/*!
 * @brief モンスターに召喚主がいるか
 * @return 召喚主がいるならtrue
 */
bool MonsterEntity::has_parent() const
{
    return this->parent_m_idx > 0;
}

/*!
 * @brief モンスターを撃破した際の述語メッセージを返す
 * @return 撃破されたモンスターの述語
 */
std::string MonsterEntity::get_died_message() const
{
    const auto &monrace = this->get_monrace();
    return monrace.get_died_message();
}

/*!
 * @brief モンスターにダメージを与えた際の述語メッセージを返す
 * @return ダメージを受けたモンスターの述語
 */
std::optional<std::string> MonsterEntity::get_pain_message(std::string_view monster_name, int damage) const
{
    auto &monrace = this->get_monrace();
    return MonsterPainDescriber(monrace.idx, monrace.symbol_definition.character, monster_name).describe(this->hp, damage, this->ml);
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

std::optional<bool> MonsterEntity::order_pet_whistle(const MonsterEntity &other) const
{
    const auto is_ordered_name = this->order_pet_named(other);
    if (is_ordered_name) {
        return *is_ordered_name;
    }

    const auto &monrace1 = this->get_monrace();
    const auto &monrace2 = other.get_monrace();
    const auto is_ordered_race = monrace1.order_pet(monrace2);
    if (is_ordered_race) {
        return *is_ordered_race;
    }

    return this->order_pet_hp(other);
}

std::optional<bool> MonsterEntity::order_pet_dismission(const MonsterEntity &other) const
{
    const auto is_ordered_name = this->order_pet_named(other);
    if (is_ordered_name) {
        return *is_ordered_name;
    }

    if (!this->has_parent() && other.has_parent()) {
        return true;
    }

    if (this->has_parent() && !other.has_parent()) {
        return false;
    }

    const auto &monrace1 = this->get_monrace();
    const auto &monrace2 = other.get_monrace();
    const auto is_ordered_race = monrace1.order_pet(monrace2);
    if (is_ordered_race) {
        return *is_ordered_race;
    }

    return this->order_pet_hp(other);
}

/*!
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param force_fixed_speed 速度を固定にする(個体差を適用しない)か否か
 */
void MonsterEntity::set_individual_speed(bool force_fixed_speed)
{
    const auto &monrace = this->get_monrace();
    auto speed = monrace.speed;
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && !force_fixed_speed) {
        /* Allow some small variation per monster */
        int i = speed_to_energy(monrace.speed) / (one_in_(4) ? 3 : 10);
        if (i) {
            speed += static_cast<uint8_t>(rand_spread(0, i));
        }
    }

    if (speed > STANDARD_SPEED + 99) {
        speed = STANDARD_SPEED + 99;
    }

    this->mspeed = speed;
}

/*!
 * @brief モンスターを敵に回す
 */
void MonsterEntity::set_hostile()
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    this->mflag2.reset({ MonsterConstantFlagType::PET, MonsterConstantFlagType::FRIENDLY });
}

/*
 * 捕獲・死亡の際にカメレオンの変身を元に戻す
 */
void MonsterEntity::reset_chameleon_polymorph()
{
    auto real_monrace_id = this->get_real_monrace_id();
    this->r_idx = real_monrace_id;
    this->ap_r_idx = real_monrace_id;
}

std::string MonsterEntity::get_pronoun_of_summoned_kin() const
{
    return this->get_monrace().get_pronoun_of_summoned_kin();
}

void MonsterEntity::make_lore_treasure(int num_item, int num_gold) const
{
    auto &monrace = this->get_monrace();
    if (!this->is_original_ap()) {
        return;
    }

    monrace.make_lore_treasure(num_item, num_gold);
    if (LoreTracker::get_instance().is_tracking(this->r_idx)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }
}

std::optional<bool> MonsterEntity::order_pet_named(const MonsterEntity &other) const
{
    if (this->is_named() && !other.is_named()) {
        return true;
    }

    if (!this->is_named() && other.is_named()) {
        return false;
    }

    return std::nullopt;
}

std::optional<bool> MonsterEntity::order_pet_hp(const MonsterEntity &other) const
{
    if (this->hp > other.hp) {
        return true;
    }

    if (this->hp < other.hp) {
        return false;
    }

    return std::nullopt;
}

/*!
 * @brief モンスターの目標地点をセットする / Set the target of counter attack
 * @param y 目標y座標
 * @param x 目標x座標
 */
void MonsterEntity::set_target(POSITION y, POSITION x)
{
    this->target_y = y;
    this->target_x = x;
}

/*!
 * @brief モンスターの目標地点をリセットする / Reset the target of counter attack
 */
void MonsterEntity::reset_target()
{
    this->set_target(0, 0);
}

/*!
 * @brief モンスターを友好的にする
 */
void MonsterEntity::set_friendly()
{
    this->mflag2.set(MonsterConstantFlagType::FRIENDLY);
}
