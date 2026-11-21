#include "spell/magic-aptitude.h"
#include "system/system-variables.h"
#include <algorithm>
#include <cstdlib>
#include <limits>

namespace {
//! 職業別の消費MP の上限 (魔法の基準値と同じ)
constexpr auto MAX_DERIVED_MANA_COST = std::numeric_limits<short>::max() / 16;

/*!
 * @brief 符号つきの割り算を、0 から遠い側へ四捨五入する
 * @param numerator 割られる数
 * @param denominator 割る数 (正)
 * @return 四捨五入した商。得意・不得意で動きが対称になる
 */
int round_half_away_from_zero(int numerator, int denominator)
{
    const auto quotient = (std::abs(numerator) * 2 + denominator) / (denominator * 2);
    return (numerator < 0) ? -quotient : quotient;
}
}

/*!
 * @brief 魔法の基準値と得意さの段階から、職業別の値を求める
 * @param base 魔法の基準値 (SpellDefinitions の値)
 * @param steps 段階 1 つあたりの動き
 * @param aptitude 得意さの段階 (正が得意、負が不得意、0 は基準値のまま)
 * @return 職業別の値。消費MP の倍率が 1% 未満になるか、消費MP が上限を超えるときは値なし
 * @details 習得レベルは 基準 − 基準 × (50 − 基準) / 50 × (習得レベルの動き × 段階)% を 1〜50 に収める。
 * 基準が 25 のとき最も大きく動き、1 や 50 に近いほど動きが小さい。
 * 消費MP は 基準 × (100 − 消費MP の動き × 段階)% を四捨五入して 1 以上に、難易度は 基準 − 難易度の動き × 段階 を 0〜100 に収める。
 * 初回経験値倍率は基準のまま。基準値が無い (習得できない) 魔法は、習得できない魔法のまま返す。
 */
tl::optional<magic_type> derive_magic_by_aptitude(const magic_type &base, const AptitudeSteps &steps, int aptitude)
{
    if (!base.is_learnable()) {
        return magic_type{};
    }

    const auto mana_percent = 100 - steps.mana_cost_percent * aptitude;
    if (mana_percent < 1) {
        return tl::nullopt;
    }
    const auto mana_cost = std::max(1, (base.smana * mana_percent + 50) / 100);
    if (mana_cost > MAX_DERIVED_MANA_COST) {
        return tl::nullopt;
    }

    const int base_level = base.get_learn_level();
    const auto level_move = round_half_away_from_zero(base_level * (PY_MAX_LEVEL - base_level) * steps.learn_level_percent * aptitude, PY_MAX_LEVEL * 100);
    const auto learn_level = std::clamp(base_level - level_move, 1, PY_MAX_LEVEL);
    const auto difficulty = std::clamp(base.sfail - steps.difficulty * aptitude, 0, 100);

    magic_type derived;
    derived.set_learn_level(static_cast<PLAYER_LEVEL>(learn_level));
    derived.smana = mana_cost;
    derived.sfail = difficulty;
    derived.sexp = base.sexp;
    return derived;
}
