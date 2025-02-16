#include "system/player-type-definition.h"
#include "floor/geometry.h"
#include "monster/monster-util.h"
#include "system/angband-exceptions.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "world/world.h"

/*!
 * @brief プレイヤー構造体実体 / Static player info record
 */
PlayerType p_body;

/*!
 * @brief プレイヤー構造体へのグローバル参照ポインタ / Pointer to the player info
 */
PlayerType *p_ptr = &p_body;

PlayerType::PlayerType()
    : timed_effects(std::make_shared<TimedEffects>())
{
}

/*!
 * @brief モンスターに乗る
 * @param m_idx 乗るモンスターのID（0で降りる）
 */
void PlayerType::ride_monster(MONSTER_IDX m_idx)
{
    if (is_monster(this->riding)) {
        this->current_floor_ptr->m_list[this->riding].mflag2.reset(MonsterConstantFlagType::RIDING);
    }

    this->riding = m_idx;

    if (is_monster(m_idx)) {
        this->current_floor_ptr->m_list[m_idx].mflag2.set(MonsterConstantFlagType::RIDING);
    }
}

std::shared_ptr<TimedEffects> PlayerType::effects() const
{
    return this->timed_effects;
}

/*!
 * @brief 自身の状態が全快で、かつフロアに影響を与えないかを検証する
 * @return 上記の通りか
 * @todo 時限効果系に分類されるものはいずれTimedEffectsクラスのメソッドとして繰り込みたい
 */
bool PlayerType::is_fully_healthy() const
{
    auto effects = this->effects();
    auto is_fully_healthy = this->chp == this->mhp;
    is_fully_healthy &= this->csp >= this->msp;
    is_fully_healthy &= !effects->blindness().is_blind();
    is_fully_healthy &= !effects->confusion().is_confused();
    is_fully_healthy &= !effects->poison().is_poisoned();
    is_fully_healthy &= !effects->fear().is_fearful();
    is_fully_healthy &= !effects->stun().is_stunned();
    is_fully_healthy &= !effects->cut().is_cut();
    is_fully_healthy &= !effects->deceleration().is_slow();
    is_fully_healthy &= !effects->paralysis().is_paralyzed();
    is_fully_healthy &= !effects->hallucination().is_hallucinated();
    is_fully_healthy &= !this->word_recall;
    is_fully_healthy &= !this->alter_reality;
    return is_fully_healthy;
}

/*
 * @brief ランダムに1つアビリティスコアを減少させる
 * @return アビリティスコア減少メッセージ
 * @todo stat_curにのみ依存するのでアビリティスコアを表すクラスへ移設する
 */
std::string PlayerType::decrease_ability_random()
{
    constexpr std::array<std::pair<int, std::string_view>, 6> candidates = { {
        { A_STR, _("強く", "strong") },
        { A_INT, _("聡明で", "bright") },
        { A_WIS, _("賢明で", "wise") },
        { A_DEX, _("器用で", "agile") },
        { A_CON, _("健康で", "hale") },
        { A_CHR, _("美しく", "beautiful") },
    } };

    const auto &[k, act] = rand_choice(candidates);
    this->stat_cur[k] = (this->stat_cur[k] * 3) / 4;
    if (this->stat_cur[k] < 3) {
        this->stat_cur[k] = 3;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    return format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), act.data());
}

/*
 * @brief 全てのアビリティスコアを減少させる
 * @return アビリティスコア減少メッセージ
 * @todo stat_curにのみ依存するのでアビリティスコアを表すクラスへ移設する
 */
std::string PlayerType::decrease_ability_all()
{
    for (auto i = 0; i < A_MAX; i++) {
        this->stat_cur[i] = (this->stat_cur[i] * 7) / 8;
        if (this->stat_cur[i] < 3) {
            this->stat_cur[i] = 3;
        }
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    return _("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be...");
}

/*!
 * @brief 現在地の瞬時値を返す
 * @details プレイヤーが移動する前後の文脈で使用すると不整合を起こすので注意
 */
Pos2D PlayerType::get_position() const
{
    return Pos2D(this->y, this->x);
}

Pos2D PlayerType::get_old_position() const
{
    return Pos2D(this->oldpy, this->oldpx);
}

/*!
 * @brief 現在地の隣 (瞬時値)または現在地を返す
 * @param dir 隣を表す方向番号
 * @details プレイヤーが移動する前後の文脈で使用すると不整合を起こすので注意
 * 方向番号による位置取りは以下の通り. 0と5は現在地.
 * 789 ...
 * 456 .@.
 * 123 ...
 */
Pos2D PlayerType::get_neighbor(int dir) const
{
    return this->get_position() + Direction(dir).vec();
}

/*!
 * @brief 現在地の隣 (瞬時値)または現在地を返す
 * @param dir 隣を表す方向
 * @attention プレイヤーが移動する前後の文脈で使用すると不整合を起こすので注意
 */
Pos2D PlayerType::get_neighbor(const Direction &dir) const
{
    return this->get_position() + dir.vec();
}

bool PlayerType::is_located_at_running_destination() const
{
    return (this->y == this->run_py) && (this->x == this->run_px);
}

bool PlayerType::is_located_at(const Pos2D &pos) const
{
    return (this->y == pos.y) && (this->x == pos.x);
}

/*!
 * @brief プレイヤーを指定座標に配置する
 * @param pos 配置先座標
 * @return 配置に成功したらTRUE
 */
bool PlayerType::try_set_position(const Pos2D &pos)
{
    if (this->current_floor_ptr->get_grid(pos).has_monster()) {
        return false;
    }

    this->y = pos.y;
    this->x = pos.x;
    return true;
}

void PlayerType::set_position(const Pos2D &pos)
{
    this->y = pos.y;
    this->x = pos.x;
}

bool PlayerType::in_saved_floor() const
{
    return this->floor_id != 0;
}

/*!
 * @brief プレイヤーの体力ランクを計算する
 *
 * プレイヤーの体力ランク（最大レベル時のHPの期待値に対する実際のHPの値の割合）を計算する。
 *
 * @return 体力ランク[%]
 */
int PlayerType::calc_life_rating() const
{
    const auto actual_hp = this->player_hp[PY_MAX_LEVEL - 1];

    // ダイスによる上昇回数は52回（初期3回+LV50までの49回）なので
    // 期待値計算のため2で割っても端数は出ない
    constexpr auto roll_num = 3 + PY_MAX_LEVEL - 1;
    const auto expected_hp = this->hit_dice.maxroll() + this->hit_dice.floored_expected_value_multiplied_by(roll_num);

    return actual_hp * 100 / expected_hp;
}

bool PlayerType::try_resist_eldritch_horror() const
{
    return evaluate_percent(this->skill_sav) || one_in_(2);
}
