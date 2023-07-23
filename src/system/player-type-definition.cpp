#include "system/player-type-definition.h"
#include "market/arena-info-table.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-deceleration.h"
#include "timed-effect/player-fear.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-poison.h"
#include "timed-effect/player-stun.h"
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

bool PlayerType::is_true_winner() const
{
    return (w_ptr->total_winner > 0) && (this->arena_number > MAX_ARENA_MONS + 2);
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
    is_fully_healthy &= !effects->blindness()->is_blind();
    is_fully_healthy &= !effects->confusion()->is_confused();
    is_fully_healthy &= !effects->poison()->is_poisoned();
    is_fully_healthy &= !effects->fear()->is_fearful();
    is_fully_healthy &= !effects->stun()->is_stunned();
    is_fully_healthy &= !effects->cut()->is_cut();
    is_fully_healthy &= !effects->deceleration()->is_slow();
    is_fully_healthy &= !effects->paralysis()->is_paralyzed();
    is_fully_healthy &= !effects->hallucination()->is_hallucinated();
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
