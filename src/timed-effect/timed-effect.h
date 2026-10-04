#pragma once

#include "system/angband-exceptions.h"

/*!
 * @brief プレイヤーの時限効果 (残り継続ターン数) を保持する
 * @tparam Tag 効果ごとに別の型とするためのタグ型。定義は不要 (不完全型でよい)
 * @details ランク等の固有処理を持つ効果はこのクラスを継承して差分のみ実装する。
 *          多態的な使用は想定していない (デストラクタは非仮想)。
 */
template <class Tag>
class TimedEffect {
public:
    TimedEffect() = default;
    ~TimedEffect() = default;
    TimedEffect(const TimedEffect &) = delete;
    TimedEffect(TimedEffect &&) = delete;
    TimedEffect &operator=(const TimedEffect &) = delete;
    TimedEffect &operator=(TimedEffect &&) = delete;

    short current() const
    {
        return this->value;
    }

    bool is_active() const
    {
        return this->value > 0;
    }

    void set(short v)
    {
        if (v < 0) {
            THROW_EXCEPTION(std::invalid_argument, "Negative value can't be set in the player's timed effect!");
        }

        this->value = v;
    }

    void reset()
    {
        this->set(0);
    }

private:
    short value = 0;
};
