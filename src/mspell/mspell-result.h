#pragma once
#include "system/angband.h"

//! モンスターが魔法を使った際の結果。
struct MonsterSpellResult {
private:
    explicit MonsterSpellResult(const bool valid, const HIT_POINT dam = 0)
        : valid(valid)
        , dam(dam)
    {
    }

public:
    bool valid; //!< 通常は true。何か変なこと(無効な魔法IDなど)が起こったら false
    bool learnable{ false }; //!< ラーニングを試みるか
    HIT_POINT dam{}; //! ダメージ量(ものまね用)

    static MonsterSpellResult make_valid(HIT_POINT dam = 0)
    {
        return MonsterSpellResult(true, dam);
    }

    static MonsterSpellResult make_invalid()
    {
        return MonsterSpellResult(false);
    }
};
