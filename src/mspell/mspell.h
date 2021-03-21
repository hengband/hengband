#pragma once

//! モンスターが魔法を使った際の結果。
struct MonsterSpellResult {
private:
    explicit MonsterSpellResult(const bool valid)
        : valid(valid)
    {
    }

public:
    bool valid; //!< 通常は true。何か変なこと(無効な魔法IDなど)が起こったら false
    bool learnable{ false }; //!< ラーニングを試みるか

    MonsterSpellResult() = delete;

    static MonsterSpellResult make_valid()
    {
        return MonsterSpellResult(true);
    }

    static MonsterSpellResult make_invalid()
    {
        return MonsterSpellResult(false);
    }
};
